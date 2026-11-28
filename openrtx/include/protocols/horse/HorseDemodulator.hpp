/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_DEMODULATOR_H
#define HORSE_DEMODULATOR_H

#include "core/iir.hpp"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <array>
#include "core/dsp.h"
#include <cmath>
#include "core/audio_path.h"
#include "core/audio_stream.h"
#include "HorseDatatypes.hpp"
#include "HorseConstants.hpp"
#include "HorseUtils.hpp"
#include "HorseSoft.hpp"
#include "protocols/M17/Correlator.hpp"
#include "protocols/M17/Synchronizer.hpp"
#include "protocols/M17/ClockRecovery.hpp"
#include "protocols/M17/DevEstimator.hpp"

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace horse
{

/** Why the demodulator left LOCKED / SYNC_UPDATE (host diagnosis). */
enum class HorseUnlockReason : uint8_t {
    None = 0,
    MissedSyncCoast, /**< missedSyncs > missUnlock (coast expiry) */
    EotSeen,         /**< EOT sync matched while locked */
    NoTagTimeout,    /**< dropWithoutTag and LOCK_NO_TAG_FRAMES */
};

class HorseDemodulator
{
public:
    HorseDemodulator();
    ~HorseDemodulator();

    void init();
    void terminate();
    void startBasebandSampling();
    void stopBasebandSampling();

    const frame_t &getFrame();
    bool update(bool invertPhase = false);
    bool isLocked();
    static std::array<int8_t, SYNCWORD_SYMBOLS> acquisitionSync();
    void resetImmediate();
    bool feedSample(int16_t sample, bool invertPhase);
    bool takeFrame(frame_t &out);
    /**
     * \brief Copy 384 uint16 soft bits for the last takeFrame() .
     *        Mapping (see HorseSoft.hpp): 0 = strong 0, 32767 = erasure,
     *        65535 = strong 1. Per 4-FSK symbol, bit0 is the sign bit
     *        (positive sample -> 0) and bit1 is inner/outer
     *        (|y| vs (2/3)|A|). Confidence is |distance to threshold|
     *        scaled by the current outer-deviation estimate.
     */
    void takeSoftBits(uint16_t out[FRAME_BITS]) const;
    void takeSymbolSamples(int16_t out[FRAME_SYMBOLS]) const;
    /* Skip the upstream DC-block (negative left-shift UB). Still applies RRC. */
    void setSkipDcBlock(bool skip);
    void setAcquireHamming(uint8_t hd);
    void setCorrPeakMin(int32_t peak);
    void setDropWithoutTag(bool enable);
    void noteValidTag();
    void setMissUnlock(uint8_t n);
    void setClockTracking(bool enable);
    int32_t lastLockCorrAbs() const;
    bool lockAuthenticated() const
    {
        return haveValidTag;
    }
    HorseUnlockReason lastUnlockReason() const
    {
        return unlockReason;
    }
    uint8_t lastUnlockMissedSyncs() const
    {
        return unlockMissedSyncs;
    }
    uint8_t debugMissedSyncs() const
    {
        return missedSyncs;
    }
    uint32_t debugSamplingPoint() const
    {
        return samplingPoint;
    }
    uint16_t debugFrameIndex() const
    {
        return frameIndex;
    }
    int8_t debugLastClockDelta() const
    {
        return lastClockDelta;
    }
    int8_t debugLastAppliedClock() const
    {
        return lastAppliedLatch;
    }
    uint8_t debugClockHold() const
    {
        return clockHold;
    }
    bool debugLastSyncOk() const
    {
        return lastSyncOk;
    }
    int32_t lastOuterAbs() const
    {
        return lastOuter;
    }

private:
    void quantize(int16_t sample);
    void reset();
    bool acquireSync(const syncw_t &word);
    bool acquireSyncConvPhase(const syncw_t &word);
    bool tryAcquireLsf();
    bool tryAcquireVoice();
    int32_t syncNccQ12(int32_t conv,
                       const std::array<int8_t, SYNCWORD_SYMBOLS> &sym);
    void unlockedState();
    void syncedState();
    void lockedState(int16_t sample);
    void syncUpdateState();

    static constexpr size_t RX_SAMPLE_RATE = 24000;
    static constexpr size_t SAMPLES_PER_SYMBOL = RX_SAMPLE_RATE / SYMBOL_RATE;
    static constexpr size_t FRAME_SAMPLES = FRAME_SYMBOLS * SAMPLES_PER_SYMBOL;
    static constexpr size_t SAMPLE_BUF_SIZE = FRAME_SAMPLES / 2;
    static constexpr size_t SYNCWORD_SAMPLES = SAMPLES_PER_SYMBOL
                                             * SYNCWORD_SYMBOLS;

    enum class DemodState { INIT, UNLOCKED, SYNCED, LOCKED, SYNC_UPDATE };

    static constexpr std::array<float, 3> sfNum = { 4.24433681e-05f,
                                                    8.48867363e-05f,
                                                    4.24433681e-05f };
    static constexpr std::array<float, 3> sfDen = { 1.0f, -1.98148851f,
                                                    0.98165828f };

    DemodState demodState;
    std::unique_ptr<int16_t[]> baseband_buffer;
    streamId basebandId;
    pathId basebandPath;
    std::unique_ptr<frame_t> demodFrame;
    std::unique_ptr<frame_t> readyFrame;
    std::unique_ptr<uint16_t[]> demodSoft;
    std::unique_ptr<uint16_t[]> readySoft;
    std::unique_ptr<uint16_t[]> lastSoft;
    std::unique_ptr<int16_t[]> demodSamp;
    std::unique_ptr<int16_t[]> readySamp;
    std::unique_ptr<int16_t[]> lastSamp;
    bool newFrame;
    bool resetClockRec;
    bool updateSampPoint;
    uint16_t frameIndex;
    uint32_t sampleIndex;
    uint32_t samplingPoint;
    uint32_t sampleCount;
    uint8_t missedSyncs;
    uint8_t missUnlock;
    bool lastSyncOk;
    uint8_t clockHold;
    int8_t clockAccum;
    uint8_t clockAgree;
    int8_t lastClockDelta;
    int8_t lastAppliedClock;
    int8_t lastAppliedLatch;
    int32_t lastOuter;
    bool clockTracking;
    uint32_t initCount;
    float corrThreshold;
    bool skipDcBlock;
    uint8_t acquireHammingMax;
    int32_t corrPeakMin;
    bool dropWithoutTag;
    uint8_t framesWithoutTag;
    bool haveValidTag;
    int32_t lastLockCorr;
    HorseUnlockReason unlockReason;
    uint8_t unlockMissedSyncs;
    struct dcBlock dcBlock;

    Correlator<SYNCWORD_SYMBOLS, SAMPLES_PER_SYMBOL> correlator;
    Synchronizer<SYNCWORD_SYMBOLS, SAMPLES_PER_SYMBOL> lsfSync{ syncwordSymbols(
        LSF_SYNC_WORD) };
    Synchronizer<SYNCWORD_SYMBOLS, SAMPLES_PER_SYMBOL> voiceSync{
        syncwordSymbols(VOICE_SYNC_WORD)
    };
    Synchronizer<SYNCWORD_SYMBOLS, SAMPLES_PER_SYMBOL> eotSync{ syncwordSymbols(
        EOT_SYNC_WORD) };
    DevEstimator devEstimator;
    ClockRecovery<SAMPLES_PER_SYMBOL> clockRec;
    Iir<3> sampleFilter;
};

} // namespace horse

#endif // HORSE_DEMODULATOR_H
