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
    , missUnlock(COAST_MISS_UNLOCK)
    , lastSyncOk(true)
    , clockHold(0)
    , clockAccum(0)
    , clockAgree(0)
    , lastClockDelta(0)
    , clockTracking(true)
    , initCount(0)
    , corrThreshold(0.0f)
    , skipDcBlock(false)
    , acquireHammingMax(HAMMING_ACQUIRE_MAX)
    , corrPeakMin(CORR_PEAK_MIN)
    , dropWithoutTag(false)
    , framesWithoutTag(0)
    , haveValidTag(false)
    , lastLockCorr(0)
    , unlockReason(HorseUnlockReason::None)
    , unlockMissedSyncs(0)
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
    demodSoft = std::make_unique<uint16_t[]>(FRAME_BITS);
    readySoft = std::make_unique<uint16_t[]>(FRAME_BITS);
    lastSoft = std::make_unique<uint16_t[]>(FRAME_BITS);
    std::memset(demodSoft.get(), 0, FRAME_BITS * sizeof(uint16_t));
    std::memset(readySoft.get(), 0, FRAME_BITS * sizeof(uint16_t));
    std::memset(lastSoft.get(), 0, FRAME_BITS * sizeof(uint16_t));
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
    demodSoft.reset();
    readySoft.reset();
    lastSoft.reset();
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
    if (lastSoft && readySoft)
        std::memcpy(lastSoft.get(), readySoft.get(),
                    FRAME_BITS * sizeof(uint16_t));
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
    if (lastSoft && readySoft)
        std::memcpy(lastSoft.get(), readySoft.get(),
                    FRAME_BITS * sizeof(uint16_t));
    newFrame = false;
    return true;
}

void HorseDemodulator::takeSoftBits(uint16_t out[FRAME_BITS]) const
{
    if (lastSoft)
        std::memcpy(out, lastSoft.get(), FRAME_BITS * sizeof(uint16_t));
    else
        std::memset(out, 0, FRAME_BITS * sizeof(uint16_t));
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
    haveValidTag = true;
}

void HorseDemodulator::setMissUnlock(uint8_t n)
{
    missUnlock = n;
}

void HorseDemodulator::setClockTracking(bool enable)
{
    clockTracking = enable;
}

int32_t HorseDemodulator::lastLockCorrAbs() const
{
    return lastLockCorr;
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
        uint8_t next = clockRec.samplingPoint();
        int step = static_cast<int>(next) - static_cast<int>(samplingPoint);
        const int sps = static_cast<int>(SAMPLES_PER_SYMBOL);
        if (step > sps / 2)
            step -= sps;
        if (step < -(sps / 2))
            step += sps;
        if (step > 1)
            step = 1;
        if (step < -1)
            step = -1;
        lastClockDelta = static_cast<int8_t>(step);
        if (!clockTracking) {
            /* Frozen acquire phase (baseline / diagnostic). */
        } else if (clockHold > 0) {
            clockHold -= 1;
            clockAccum = 0;
            clockAgree = 0;
        } else if (!lastSyncOk) {
            clockAccum = 0;
            clockAgree = 0;
        } else if (step == 0) {
            clockAccum = 0;
            clockAgree = 0;
        } else if (clockAgree == 0 || ((step > 0) == (clockAccum > 0))) {
            clockAccum = static_cast<int8_t>(clockAccum + step);
            clockAgree += 1;
            if (clockAgree >= CLOCK_AGREE_FRAMES) {
                const int apply = clockAccum > 0 ? 1 : -1;
                samplingPoint = static_cast<uint32_t>(
                    (static_cast<int>(samplingPoint) + apply + sps) % sps);
                clockAccum = 0;
                clockAgree = 0;
                clockHold = 1;
            }
        } else {
            clockAccum = static_cast<int8_t>(step);
            clockAgree = 1;
        }
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
    int16_t op = static_cast<int16_t>(outerDeviation.first);
    int16_t on = static_cast<int16_t>(outerDeviation.second);
    int8_t symbol;
    if (sample > (2 * outerDeviation.first) / 3)
        symbol = +3;
    else if (sample < (2 * outerDeviation.second) / 3)
        symbol = -3;
    else if (sample > 0)
        symbol = +1;
    else
        symbol = -1;
    if (demodSoft) {
        uint16_t msb = 0, lsb = 0;
        symbol_soft(sample, op, on, msb, lsb);
        demodSoft[2 * frameIndex] = msb;
        demodSoft[2 * frameIndex + 1] = lsb;
    }
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
    haveValidTag = false;
    unlockReason = HorseUnlockReason::None;
    unlockMissedSyncs = 0;
    lastSyncOk = true;
    clockHold = 0;
    clockAccum = 0;
    clockAgree = 0;
    lastClockDelta = 0;
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
    const uint16_t savedFi = frameIndex;
    const frame_t savedFrame = *demodFrame;

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

    if (bestHd > acquireHammingMax) {
        /*
         * A failed search must not leave frameIndex at SYNCWORD_SYMBOLS
         * or the mid-frame demodFrame half-written: that stalled
         * takeFrame at noise ~12000 while known-phase slicing still
         * decoded (got=162, locked=1, frameIndex=184).
         */
        frameIndex = savedFi;
        *demodFrame = savedFrame;
        return false;
    }

    *demodFrame = bestFrame;
    samplingPoint = sampleIndex;
    frameIndex = SYNCWORD_SYMBOLS;
    framesWithoutTag = 0;
    haveValidTag = false;
    missedSyncs = 0;
    lastSyncOk = true;
    resetClockRec = true;
    clockHold = CLOCK_HOLD_FRAMES;
    clockAccum = 0;
    clockAgree = 0;
    devEstimator.init(bestDev);
    demodState = DemodState::LOCKED;
    return true;
}

bool HorseDemodulator::acquireSyncConvPhase(const syncw_t &word)
{
    /*
     * phase = SAMPLES_PER_SYMBOL-1 matches Correlator::convolve()
     * (index()+SAMPLES_PER_SYMBOL). Slice only that alignment so a
     * Hamming-0 match cannot lock a neighbour sample when the floor
     * is 0.
     */
    const uint16_t savedFi = frameIndex;
    const frame_t savedFrame = *demodFrame;
    const uint32_t savedSp = samplingPoint;
    const uint32_t phase = SAMPLES_PER_SYMBOL - 1u;
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
    if (hd > acquireHammingMax) {
        frameIndex = savedFi;
        *demodFrame = savedFrame;
        samplingPoint = savedSp;
        return false;
    }
    samplingPoint = sampleIndex;
    frameIndex = SYNCWORD_SYMBOLS;
    framesWithoutTag = 0;
    haveValidTag = false;
    missedSyncs = 0;
    lastSyncOk = true;
    resetClockRec = true;
    clockHold = CLOCK_HOLD_FRAMES;
    clockAccum = 0;
    clockAgree = 0;
    devEstimator.init({ outerPos, outerNeg });
    demodState = DemodState::LOCKED;
    return true;
}

bool HorseDemodulator::tryAcquireLsf()
{
    int32_t syncThresh = static_cast<int32_t>(corrThreshold * CORR_SYNC_SCALE);
    const auto lsfSym = syncwordSymbols(LSF_SYNC_WORD);
    int32_t cL = correlator.convolve(lsfSym);
    int32_t cLabs = std::abs(cL);
    int32_t ncc = syncNccQ12(cL, lsfSym);

    if ((cLabs > syncThresh) && (ncc >= corrPeakMin)
        && acquireSyncConvPhase(LSF_SYNC_WORD)) {
        lastLockCorr = ncc;
        return true;
    }
    return false;
}

bool HorseDemodulator::tryAcquireVoice()
{
    int32_t syncThresh = static_cast<int32_t>(corrThreshold * CORR_SYNC_SCALE);
    const auto voiceSym = syncwordSymbols(VOICE_SYNC_WORD);
    int32_t cV = correlator.convolve(voiceSym);
    int32_t cVabs = std::abs(cV);
    int32_t ncc = syncNccQ12(cV, voiceSym);

    if ((cVabs > syncThresh) && (ncc >= corrPeakMin)
        && acquireSyncConvPhase(VOICE_SYNC_WORD)) {
        lastLockCorr = ncc;
        return true;
    }
    return false;
}

int32_t
HorseDemodulator::syncNccQ12(int32_t conv,
                             const std::array<int8_t, SYNCWORD_SYMBOLS> &sym)
{
    int64_t e2 = 0;
    int64_t p2 = 0;
    size_t pos = correlator.index() + SAMPLES_PER_SYMBOL;
    for (size_t s = 0; s < SYNCWORD_SYMBOLS; s++) {
        int32_t v = correlator.data()[pos % SYNCWORD_SAMPLES];
        e2 += static_cast<int64_t>(v) * static_cast<int64_t>(v);
        p2 += static_cast<int64_t>(sym[s]) * static_cast<int64_t>(sym[s]);
        pos += SAMPLES_PER_SYMBOL;
    }
    if (e2 <= 0 || p2 <= 0)
        return 0;
    float ncc = std::fabs(static_cast<float>(conv))
              / (std::sqrt(static_cast<float>(e2))
                 * std::sqrt(static_cast<float>(p2)));
    if (ncc > 1.0f)
        ncc = 1.0f;
    return static_cast<int32_t>(ncc * 4096.0f + 0.5f);
}

void HorseDemodulator::unlockedState()
{
    if (tryAcquireLsf())
        return;
    (void)tryAcquireVoice();
}

void HorseDemodulator::syncedState()
{
    if (acquireSync(LSF_SYNC_WORD))
        return;
    demodState = DemodState::UNLOCKED;
}

void HorseDemodulator::lockedState(int16_t sample)
{
    if (!haveValidTag && (framesWithoutTag >= 1) && (missedSyncs > 0)
        && tryAcquireLsf())
        return;
    if (sampleIndex != samplingPoint)
        return;
    quantize(sample);
    devEstimator.sample(sample);
    if (frameIndex == FRAME_SYMBOLS) {
        devEstimator.update();
        std::swap(readyFrame, demodFrame);
        if (readySoft && demodSoft)
            std::swap(readySoft, demodSoft);
        frameIndex = 0;
        newFrame = true;
        updateSampPoint = true;
        framesWithoutTag += 1;
        if (dropWithoutTag && (framesWithoutTag > LOCK_NO_TAG_FRAMES)) {
            unlockReason = HorseUnlockReason::NoTagTimeout;
            unlockMissedSyncs = missedSyncs;
            demodState = DemodState::UNLOCKED;
            return;
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
    if (valid) {
        missedSyncs = 0;
        lastSyncOk = true;
    } else {
        missedSyncs += 1;
        lastSyncOk = false;
    }
    if (eot) {
        unlockReason = HorseUnlockReason::EotSeen;
        unlockMissedSyncs = missedSyncs;
        demodState = DemodState::UNLOCKED;
    } else if (missedSyncs > missUnlock) {
        unlockReason = HorseUnlockReason::MissedSyncCoast;
        unlockMissedSyncs = missedSyncs;
        demodState = DemodState::UNLOCKED;
    } else
        demodState = DemodState::LOCKED;
}

constexpr std::array<float, 3> HorseDemodulator::sfNum;
constexpr std::array<float, 3> HorseDemodulator::sfDen;

} // namespace horse
