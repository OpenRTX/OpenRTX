/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/HorseDemodulator.hpp"
#include "protocols/M17/DSP.hpp"
#include "protocols/M17/Utils.hpp"
#include <cmath>
#include <cstring>

namespace horse
{

static uint8_t hammingDistance(uint8_t x, uint8_t y)
{
    return __builtin_popcount(x ^ y);
}

HorseDemodulator::HorseDemodulator()
    : demodState(DemodState::INIT),
      newFrame(false),
      resetClockRec(false),
      updateSampPoint(false),
      frameIndex(0),
      sampleIndex(0),
      samplingPoint(0),
      sampleCount(0),
      missedSyncs(0),
      initCount(0),
      corrThreshold(0.0f),
      sampleFilter(sfNum, sfDen)
{
    dsp_resetState(dcBlock);
}

HorseDemodulator::~HorseDemodulator()
{
    terminate();
}

void HorseDemodulator::init()
{
    baseband_buffer = std::make_unique<int16_t[]>(2 * SAMPLE_BUF_SIZE);
    demodFrame      = std::make_unique<frame_t>();
    readyFrame      = std::make_unique<frame_t>();
    reset();
}

void HorseDemodulator::terminate()
{
    audioPath_release(basebandPath);
    audioStream_terminate(basebandId);
    baseband_buffer.reset();
    demodFrame.reset();
    readyFrame.reset();
}

void HorseDemodulator::startBasebandSampling()
{
    basebandPath = audioPath_request(SOURCE_RTX, SINK_MCU, PRIO_RX);
    basebandId   = audioStream_start(basebandPath, baseband_buffer.get(),
                                    2 * SAMPLE_BUF_SIZE, RX_SAMPLE_RATE,
                                    STREAM_INPUT | BUF_CIRC_DOUBLE);
    reset();
}

void HorseDemodulator::stopBasebandSampling()
{
    audioStream_terminate(basebandId);
    audioPath_release(basebandPath);
}

const frame_t& HorseDemodulator::getFrame()
{
    newFrame = false;
    return *readyFrame;
}

bool HorseDemodulator::isLocked()
{
    return (demodState == DemodState::LOCKED) || (demodState == DemodState::SYNC_UPDATE);
}

bool HorseDemodulator::update(bool invertPhase)
{
    if (audioPath_getStatus(basebandPath) != PATH_OPEN)
        return false;
    dataBlock_t baseband = inputStream_getData(basebandId);
    if (baseband.data == nullptr)
        return false;
    for (size_t i = 0; i < baseband.len; i++)
    {
        int16_t sample = dsp_dcBlockFilter(&dcBlock, baseband.data[i]);
        float elem     = static_cast<float>(sample);
        if (invertPhase) elem = 0.0f - elem;
        sample = static_cast<int16_t>(M17::rrc_24k(elem));
        if ((sampleIndex == 0) && resetClockRec)
        {
            clockRec.reset();
            resetClockRec   = false;
            updateSampPoint = false;
        }
        int diff = static_cast<int>(samplingPoint) - static_cast<int>(sampleIndex);
        if (updateSampPoint && (std::abs(diff) == static_cast<int>(SAMPLES_PER_SYMBOL / 2)))
        {
            clockRec.update();
            samplingPoint  = clockRec.samplingPoint();
            updateSampPoint = false;
        }
        clockRec.sample(sample);
        correlator.sample(sample);
        corrThreshold = sampleFilter(static_cast<float>(std::abs(sample)));
        switch (demodState)
        {
        case DemodState::INIT:
            if (initCount == 0)
                demodState = DemodState::UNLOCKED;
            else
                initCount -= 1;
            break;
        case DemodState::UNLOCKED:
            unlockedState();
            break;
        case DemodState::SYNCED:
            syncedState();
            break;
        case DemodState::LOCKED:
            lockedState(sample);
            break;
        case DemodState::SYNC_UPDATE:
            syncUpdateState();
            break;
        }
        sampleCount += 1;
        sampleIndex = (sampleIndex + 1) % SAMPLES_PER_SYMBOL;
    }
    return newFrame;
}

void HorseDemodulator::quantize(int16_t sample)
{
    auto outerDeviation = devEstimator.outerDeviation();
    int8_t symbol;
    if (sample > (2 * outerDeviation.first) / 3)
        symbol = +3;
    else if (sample < (2 * outerDeviation.second) / 3)
        symbol = -3;
    else if (sample > 0)
        symbol = +1;
    else
        symbol = -1;
    M17::setSymbol(*demodFrame, frameIndex, symbol);
    frameIndex += 1;
}

void HorseDemodulator::reset()
{
    sampleIndex  = 0;
    frameIndex   = 0;
    sampleCount  = 0;
    newFrame     = false;
    demodState   = DemodState::INIT;
    initCount    = RX_SAMPLE_RATE / 50;
    dsp_resetState(dcBlock);
}

void HorseDemodulator::unlockedState()
{
    int32_t syncThresh = static_cast<int32_t>(corrThreshold * 33.0f);
    int8_t syncStatus  = streamSync.update(correlator, syncThresh, -syncThresh);
    if (syncStatus != 0)
        demodState = DemodState::SYNCED;
}

void HorseDemodulator::syncedState()
{
    samplingPoint     = streamSync.samplingIndex();
    auto deviation    = correlator.maxDeviation(samplingPoint);
    frameIndex        = 0;
    devEstimator.init(deviation);
    for (size_t i = 0; i < SYNCWORD_SAMPLES; i++)
    {
        size_t pos = (correlator.index() + i) % SYNCWORD_SAMPLES;
        if ((pos % SAMPLES_PER_SYMBOL) == samplingPoint)
        {
            int16_t val = correlator.data()[pos];
            quantize(val);
        }
    }
    uint8_t lsfHd = hammingDistance((*demodFrame)[0], LSF_SYNC_WORD[0]) +
                  hammingDistance((*demodFrame)[1], LSF_SYNC_WORD[1]);
    uint8_t voiceHd = hammingDistance((*demodFrame)[0], VOICE_SYNC_WORD[0]) +
                      hammingDistance((*demodFrame)[1], VOICE_SYNC_WORD[1]);
    uint8_t eotHd = hammingDistance((*demodFrame)[0], EOT_SYNC_WORD[0]) +
                    hammingDistance((*demodFrame)[1], EOT_SYNC_WORD[1]);
    if (lsfHd <= 2 || voiceHd <= 2 || eotHd <= 2)
        demodState = DemodState::LOCKED;
    else
        demodState = DemodState::UNLOCKED;
}

void HorseDemodulator::lockedState(int16_t sample)
{
    if (sampleIndex != samplingPoint)
        return;
    quantize(sample);
    devEstimator.sample(sample);
    if (frameIndex == FRAME_SYMBOLS)
    {
        devEstimator.update();
        std::swap(readyFrame, demodFrame);
        frameIndex      = 0;
        newFrame        = true;
        updateSampPoint = true;
        demodState      = DemodState::SYNC_UPDATE;
    }
}

void HorseDemodulator::syncUpdateState()
{
    uint8_t lsfHd = hammingDistance((*demodFrame)[0], LSF_SYNC_WORD[0]) +
                    hammingDistance((*demodFrame)[1], LSF_SYNC_WORD[1]);
    uint8_t voiceHd = hammingDistance((*demodFrame)[0], VOICE_SYNC_WORD[0]) +
                      hammingDistance((*demodFrame)[1], VOICE_SYNC_WORD[1]);
    uint8_t eotHd = hammingDistance((*demodFrame)[0], EOT_SYNC_WORD[0]) +
                    hammingDistance((*demodFrame)[1], EOT_SYNC_WORD[1]);
    if (lsfHd <= 2 || voiceHd <= 2)
        missedSyncs = 0;
    else
        missedSyncs += 1;
    if ((missedSyncs > 4) || (eotHd <= 2))
        demodState = DemodState::UNLOCKED;
    else
        demodState = DemodState::LOCKED;
}

constexpr std::array<float, 3> HorseDemodulator::sfNum;
constexpr std::array<float, 3> HorseDemodulator::sfDen;

}  // namespace horse
