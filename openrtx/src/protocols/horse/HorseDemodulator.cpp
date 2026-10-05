/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/HorseDemodulator.hpp"
#include "protocols/M17/DSP.hpp"
#include "protocols/M17/Utils.hpp"
#include "protocols/horse/HorseUtils.hpp"
#include <cmath>
#include <cstring>

namespace horse
{

static uint8_t hammingDistance(uint8_t x, uint8_t y)
{
    return __builtin_popcount(x ^ y);
}

HorseDemodulator::HorseDemodulator()
    : demodState(DemodState::INIT)
    , baseband_buffer()
    , basebandId(-1)
    , basebandPath(-1)
    , demodFrame()
    , readyFrame()
    , newFrame(false)
    , resetClockRec(false)
    , updateSampPoint(false)
    , frameIndex(0)
    , sampleIndex(0)
    , samplingPoint(0)
    , sampleCount(0)
    , missedSyncs(0)
    , initCount(0)
    , corrThreshold(0.0f)
    , skipDcBlock(false)
    , acquireHammingMax(HAMMING_ACQUIRE_MAX)
    , corrPeakMin(CORR_PEAK_MIN)
    , dropWithoutTag(false)
    , framesWithoutTag(0)
    , sampleFilter(sfNum, sfDen)
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
    demodFrame = std::make_unique<frame_t>();
    readyFrame = std::make_unique<frame_t>();
    M17::rrc_24k.reset();
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
    basebandId = audioStream_start(basebandPath, baseband_buffer.get(),
                                   2 * SAMPLE_BUF_SIZE, RX_SAMPLE_RATE,
                                   STREAM_INPUT | BUF_CIRC_DOUBLE);
    reset();
}

void HorseDemodulator::stopBasebandSampling()
{
    audioStream_terminate(basebandId);
    audioPath_release(basebandPath);
}

const frame_t &HorseDemodulator::getFrame()
{
    newFrame = false;
    return *readyFrame;
}

bool HorseDemodulator::isLocked()
{
    return (demodState == DemodState::LOCKED)
        || (demodState == DemodState::SYNC_UPDATE);
}

std::array<int8_t, SYNCWORD_SYMBOLS> HorseDemodulator::acquisitionSync()
{
    return syncwordSymbols(LSF_SYNC_WORD);
}

bool HorseDemodulator::update(bool invertPhase)
{
    if (audioPath_getStatus(basebandPath) != PATH_OPEN)
        return false;
    dataBlock_t baseband = inputStream_getData(basebandId);
    if (baseband.data == nullptr)
        return false;
    for (size_t i = 0; i < baseband.len; i++)
        feedSample(baseband.data[i], invertPhase);
    return newFrame;
}

void HorseDemodulator::resetImmediate()
{
    reset();
    demodState = DemodState::UNLOCKED;
    initCount = 0;
}

bool HorseDemodulator::takeFrame(frame_t &out)
{
    if (!newFrame || readyFrame == nullptr)
        return false;
    out = *readyFrame;
    newFrame = false;
    return true;
}

void HorseDemodulator::setSkipDcBlock(bool skip)
{
    skipDcBlock = skip;
}

void HorseDemodulator::setAcquireHamming(uint8_t hd)
{
    acquireHammingMax = hd;
}

void HorseDemodulator::setCorrPeakMin(int32_t peak)
{
    corrPeakMin = peak;
}

void HorseDemodulator::setDropWithoutTag(bool enable)
{
    dropWithoutTag = enable;
}

void HorseDemodulator::noteValidTag()
{
    framesWithoutTag = 0;
}

bool HorseDemodulator::feedSample(int16_t sample, bool invertPhase)
{
    if (!skipDcBlock)
        sample = dsp_dcBlockFilter(&dcBlock, sample);
    float elem = static_cast<float>(sample);
    if (invertPhase)
        elem = 0.0f - elem;
    sample = static_cast<int16_t>(M17::rrc_24k(elem));
    if ((sampleIndex == 0) && resetClockRec) {
        clockRec.reset();
        resetClockRec = false;
        updateSampPoint = false;
    }
    int diff = static_cast<int>(samplingPoint) - static_cast<int>(sampleIndex);
    if (updateSampPoint
        && (std::abs(diff) == static_cast<int>(SAMPLES_PER_SYMBOL / 2))) {
        clockRec.update();
        samplingPoint = clockRec.samplingPoint();
        updateSampPoint = false;
    }
    clockRec.sample(sample);
    correlator.sample(sample);
    corrThreshold = sampleFilter(static_cast<float>(std::abs(sample)));
    switch (demodState) {
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
    horse::setSymbol(*demodFrame, frameIndex, symbol);
    frameIndex += 1;
}

void HorseDemodulator::reset()
{
    sampleIndex = 0;
    frameIndex = 0;
    sampleCount = 0;
    newFrame = false;
    missedSyncs = 0;
    framesWithoutTag = 0;
    demodState = DemodState::INIT;
    initCount = RX_SAMPLE_RATE / 50;
    dsp_resetState(dcBlock);
}

static bool syncwordMatch(const frame_t &frame, const syncw_t &word)
{
    uint8_t hd = hammingDistance(frame[0], word[0])
               + hammingDistance(frame[1], word[1]);
    return hd <= HAMMING_SYNC_MAX;
}

bool HorseDemodulator::acquireSync(const syncw_t &word)
{
    uint8_t bestHd = 0xFF;
    frame_t bestFrame{};
    std::pair<int32_t, int32_t> bestDev{ 0, 0 };

    /*
     * Walk the correlator buffer in the same order as Correlator::convolve():
     * start at index()+phase+1, then +SAMPLES_PER_SYMBOL. phase=4 matches
     * the convolution taps used for the peak.
     */
    for (uint32_t phase = 0; phase < SAMPLES_PER_SYMBOL; phase++) {
        size_t pos = (correlator.index() + 1 + phase) % SYNCWORD_SAMPLES;
        int16_t taps[SYNCWORD_SYMBOLS];
        int16_t peakAbs = 1;
        for (size_t s = 0; s < SYNCWORD_SYMBOLS; s++) {
            taps[s] = correlator.data()[pos];
            int16_t a = static_cast<int16_t>(std::abs(taps[s]));
            if (a > peakAbs)
                peakAbs = a;
            pos = (pos + SAMPLES_PER_SYMBOL) % SYNCWORD_SAMPLES;
        }
        frameIndex = 0;
        int16_t outerPos = peakAbs;
        int16_t outerNeg = static_cast<int16_t>(-peakAbs);
        for (size_t s = 0; s < SYNCWORD_SYMBOLS; s++) {
            int8_t sy = quantizeLevel(taps[s], outerPos, outerNeg);
            setSymbol(*demodFrame, frameIndex, sy);
            frameIndex += 1;
        }
        uint8_t hd = hammingDistance((*demodFrame)[0], word[0])
                   + hammingDistance((*demodFrame)[1], word[1]);
        if (hd < bestHd) {
            bestHd = hd;
            bestFrame = *demodFrame;
            bestDev = { outerPos, outerNeg };
        }
    }

    if (bestHd > acquireHammingMax)
        return false;

    *demodFrame = bestFrame;
    samplingPoint = sampleIndex;
    frameIndex = SYNCWORD_SYMBOLS;
    framesWithoutTag = 0;
    devEstimator.init(bestDev);
    demodState = DemodState::LOCKED;
    return true;
}

void HorseDemodulator::unlockedState()
{
    int32_t syncThresh = static_cast<int32_t>(corrThreshold * CORR_SYNC_SCALE);
    const auto lsfSym = syncwordSymbols(LSF_SYNC_WORD);
    int32_t cL = correlator.convolve(lsfSym);

    if ((std::abs(cL) > syncThresh) && (std::abs(cL) >= corrPeakMin)
        && acquireSync(LSF_SYNC_WORD))
        return;
}

void HorseDemodulator::syncedState()
{
    if (acquireSync(LSF_SYNC_WORD))
        return;
    demodState = DemodState::UNLOCKED;
}

void HorseDemodulator::lockedState(int16_t sample)
{
    if (sampleIndex != samplingPoint)
        return;
    quantize(sample);
    devEstimator.sample(sample);
    if (frameIndex == FRAME_SYMBOLS) {
        devEstimator.update();
        std::swap(readyFrame, demodFrame);
        frameIndex = 0;
        newFrame = true;
        updateSampPoint = true;
        if (dropWithoutTag) {
            framesWithoutTag += 1;
            if (framesWithoutTag > LOCK_NO_TAG_FRAMES) {
                demodState = DemodState::UNLOCKED;
                return;
            }
        }
        demodState = DemodState::SYNC_UPDATE;
    }
}

void HorseDemodulator::syncUpdateState()
{
    /*
     * After swap(), readyFrame holds the frame that just completed. Check
     * its sync word (LSF, voice or EOT), not the empty demod buffer.
     */
    const frame_t &justRx = *readyFrame;
    bool valid = syncwordMatch(justRx, LSF_SYNC_WORD)
              || syncwordMatch(justRx, VOICE_SYNC_WORD);
    bool eot = syncwordMatch(justRx, EOT_SYNC_WORD);
    if (valid)
        missedSyncs = 0;
    else
        missedSyncs += 1;
    if ((missedSyncs > 4) || eot)
        demodState = DemodState::UNLOCKED;
    else
        demodState = DemodState::LOCKED;
}

constexpr std::array<float, 3> HorseDemodulator::sfNum;
constexpr std::array<float, 3> HorseDemodulator::sfDen;

} // namespace horse
