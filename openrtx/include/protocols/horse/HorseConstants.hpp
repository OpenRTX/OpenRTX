/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_CONSTANTS_H
#define HORSE_CONSTANTS_H

#include "HorseDatatypes.hpp"
#include <cstddef>
#include <cstring>

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace horse
{

static constexpr size_t SYMBOL_RATE = 4800;
static constexpr size_t FRAME_SYMBOLS = 192;
static constexpr size_t SYNCWORD_SYMBOLS = 8;
static constexpr size_t FRAME_BYTES = FRAME_SYMBOLS / 4;

static constexpr syncw_t LSF_SYNC_WORD = { 0x5A, 0xA7 };
static constexpr syncw_t VOICE_SYNC_WORD = { 0x7E, 0x9B };
static constexpr syncw_t EOT_SYNC_WORD = { 0x3C, 0xD8 };

static constexpr size_t LSF_CALLSIGN_BYTES = 6;
static constexpr size_t LSF_EPH_PK_OFFSET = 12;
static constexpr size_t LSF_FLAGS_OFFSET = 44;
static constexpr size_t LSF_VERSION_OFFSET = 45;
static constexpr uint8_t LSF_FLAG_ENCRYPTED = 0x01;
static constexpr uint8_t LSF_FLAG_SIGNED = 0x02;
static constexpr uint8_t LSF_PROTOCOL_VERSION = 1;
static constexpr uint16_t SIG_FRAME_BASE = 0x7000;
static constexpr uint16_t SIG_FRAME_COUNT = 6;
/* Last legal voice FN. 0x7000-0x7FFF are signature frames only. */
static constexpr uint16_t VOICE_FN_MAX = 0x6FFF;

/* Option C voice: 18 info bytes -> M17 DATA_PUNCTURE (34 B) + 12 B spare. */
static constexpr size_t HORSE_VOICE_INFO_BYTES = 18;
static constexpr size_t HORSE_VOICE_CODED_BYTES = 46;
static constexpr size_t HORSE_VOICE_PUNCT_BYTES = 34;
static constexpr size_t HORSE_VOICE_SPARE_BYTES = 12;
static constexpr size_t HORSE_VOICE_SPARE_BITS = HORSE_VOICE_SPARE_BYTES * 8;
static constexpr size_t HORSE_VOICE_CODED_BITS = HORSE_VOICE_CODED_BYTES * 8;
static constexpr size_t HORSE_FRAG_CYCLE = 10;
static constexpr size_t HORSE_FRAG_BYTES = HORSE_VOICE_SPARE_BYTES;
static constexpr size_t HORSE_FRAG_MAJORITY = 3;
static constexpr size_t HORSE_FRAG_LSF_SLOTS = 4;
static constexpr size_t HORSE_FRAG_SIG_SLOTS = 6;

/* Opening LSF: three M17-coded 18-byte chunks covering 46 B + CRC16. */
static constexpr size_t LSF_OPENING_FRAMES = 3;
static constexpr size_t LSF_RAW_BYTES = 46;
static constexpr size_t LSF_WITH_CRC_BYTES = 48;
static constexpr size_t LSF_CHUNK_BYTES = HORSE_VOICE_INFO_BYTES;

/**
 * \brief Map a voice or signature FN to the fragment cycle slot (0..9).
 * @return slot index, or HORSE_FRAG_CYCLE if FN is not in the cycle.
 */
static inline size_t horse_frag_slot(uint16_t fn)
{
    fn = static_cast<uint16_t>(fn & 0x7FFF);
    if (fn <= VOICE_FN_MAX)
        return static_cast<size_t>(fn % HORSE_FRAG_CYCLE);
    if (fn >= SIG_FRAME_BASE && fn < SIG_FRAME_BASE + SIG_FRAME_COUNT)
        return HORSE_FRAG_LSF_SLOTS
             + static_cast<size_t>(fn - SIG_FRAME_BASE);
    return HORSE_FRAG_CYCLE;
}

static inline bool voice_fn_in_session(uint16_t fn)
{
    return fn <= VOICE_FN_MAX;
}

/* Lost frames are allowed as a gap; repeats and backward FN are not. */
static inline bool voice_fn_newer(bool have_prev, uint16_t prev, uint16_t next)
{
    if (!voice_fn_in_session(next))
        return false;
    if (!have_prev)
        return true;
    return next > prev;
}
/*
 * Signature transport is final: 5 frames of 12 bytes and 1 frame of 4
 * bytes (64 total). Remaining payload bytes in the last frame are zero.
 */
static constexpr size_t SIG_CHUNK_BYTES = 12;
static constexpr size_t SIG_BYTES = 64;

static inline size_t sig_chunk_bytes(unsigned chunk)
{
    const size_t off = static_cast<size_t>(chunk) * SIG_CHUNK_BYTES;
    if (off >= SIG_BYTES)
        return 0;
    const size_t left = SIG_BYTES - off;
    return (left > SIG_CHUNK_BYTES) ? SIG_CHUNK_BYTES : left;
}

static inline bool horse_sig_store_chunk(uint8_t *sig, unsigned chunk,
                                         const uint8_t *src)
{
    const size_t n = sig_chunk_bytes(chunk);
    if (sig == nullptr || src == nullptr || n == 0)
        return false;
    std::memcpy(sig + static_cast<size_t>(chunk) * SIG_CHUNK_BYTES, src, n);
    return true;
}
static constexpr size_t VOICE_FRAME_COUNTER_BITS = 16;
static constexpr size_t VOICE_MELPE_BITS = 96;
static constexpr size_t VOICE_TAG_BITS = 32;

/* Max Hamming distance when matching a 16-bit sync word (two bytes). */
static constexpr uint8_t HAMMING_SYNC_MAX = 2;
/*
 * Acquisition Hamming (LSF only). Hamming 0 has the fewest LSF false
 * locks on open-FM Gaussian noise and still passes clean/impaired
 * loopback (see horse_loopback table). Late entry on voice/EOT is not
 * supported.
 */
static constexpr uint8_t HAMMING_ACQUIRE_MAX = 0;

/*
 * Floor on LSF acquire: |corr| / (||samples|| * ||sync||) in Q12
 * (4096 == 1.0). Real LSF ncc under noise overlaps Hamming-0 false
 * locks (p5 at sigma 12500 is 3985; false-lock max is ~4056), so the
 * extra floor is 0. Hamming-0 acquire remains.
 */
static constexpr int32_t CORR_PEAK_MIN = 0;

/*
 * After lock, this many completed frames without noteValidTag() drops
 * the lock so a real LSF can be acquired. Sized for LSF + 6 signature
 * frames; the first tagged voice must arrive in that window.
 */
static constexpr uint8_t LOCK_NO_TAG_FRAMES = 8;
/*
 * After lock, this many completed frames without a Hamming-2 sync
 * still keep the sampling point (coasting). EOT still unlocks at
 * once. Measured on HorseDemodulator with idle zeros after the last
 * voice: N=2 released after 3 extra frames (120 ms), N=4 after 5
 * (200 ms), N=8 after 9 (360 ms). N=4 covers a two-frame fade without
 * holding the channel for a third of a second after a missed EOT.
 */
static constexpr uint8_t COAST_MISS_UNLOCK = 4;
/*
 * After LSF acquire, hold the sampling point this many completed
 * frames before the timing detector may move it. Covers the firmware
 * DC-block IIR settling: a one-sample SP jump on the first voice
 * pushed sync Hamming distance past 2 (HD=4) with DC on; the same
 * jump with skipDcBlock recovered voice at HD=1.
 */
static constexpr uint8_t CLOCK_HOLD_FRAMES = 3;
/*
 * Require this many consecutive same-sign TED steps (each limited to
 * ±1) on frames with a Hamming-2 sync before applying one sample of
 * correction. Rejects single noisy frames.
 */
static constexpr uint8_t CLOCK_AGREE_FRAMES = 3;

/*
 * Correlator peak must exceed corrThreshold * this scale. M17 uses 33 for
 * all-outer Barker syncwords (energy 72). Horse LSF/EOT energy is 40 and
 * voice is 48, so 33 * 40/72 ~= 18.
 */
static constexpr float CORR_SYNC_SCALE = 18.0f;

} // namespace horse

#endif // HORSE_CONSTANTS_H
