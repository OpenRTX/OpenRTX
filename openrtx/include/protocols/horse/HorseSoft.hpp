/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_SOFT_H
#define HORSE_SOFT_H

#include "HorseConstants.hpp"
#include "protocols/M17/Decorrelator.hpp"
#include <cstdint>
#include <cstring>
#include <algorithm>

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace horse
{

/**
 * Soft bit: 0 = strong 0, 32767 = erasure, 65535 = strong 1.
 *
 * 4-FSK dibit (MSB first): 00=+1, 01=+3, 10=-1, 11=-3.
 * Thresholds: sign at 0, inner/outer at (2/3)*|outer|.
 *
 * b0 (MSB): sign of the sample (positive -> 0).
 * b1 (LSB): outer if |sample| > (2/3)|A|, else inner.
 * Confidence is |distance-to-threshold| scaled by |A|, saturating.
 */
inline uint16_t pack_soft_bit(int32_t dist_toward_zero, int32_t scale)
{
    if (scale < 1)
        scale = 1;
    int64_t v = (static_cast<int64_t>(dist_toward_zero) * 32767) / scale;
    if (v > 32767)
        v = 32767;
    if (v < -32768)
        v = -32768;
    return static_cast<uint16_t>(32767 - v);
}

inline uint16_t invert_soft(uint16_t s)
{
    return static_cast<uint16_t>(0xFFFFu - s);
}

inline void symbol_soft(int16_t sample, int16_t outerPos, int16_t outerNeg,
                        uint16_t &msb, uint16_t &lsb)
{
    int32_t ap = outerPos > 0 ? outerPos : 1;
    int32_t an = outerNeg < 0 ? -outerNeg : 1;
    int32_t a = (ap + an) / 2;
    if (a < 1)
        a = 1;
    /*
     * M17::SoftViterbi: 0 = strong 0, 32767 = erasure, 65535 = strong 1.
     * MSB decision boundary is y=0 (positive constellation -> bit 0).
     * LSB decision boundary is |y|=(2/3)|A| (inner -> bit 0, outer -> 1).
     * Each value is monotonic in signed distance to its boundary, scaled
     * by |A|. No inner/outer hard classification before packing.
     */
    msb = pack_soft_bit(sample, a);
    int32_t absy = sample >= 0 ? sample : -sample;
    int32_t thresh = sample >= 0 ? (2 * ap) / 3 : (2 * an) / 3;
    lsb = pack_soft_bit(thresh - absy, a);
}

inline bool soft_bit_hard(uint16_t s)
{
    return s >= 32768u;
}

inline void soft_from_hard_payload(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                                   uint16_t out[HORSE_VOICE_CODED_BITS])
{
    for (size_t i = 0; i < HORSE_VOICE_CODED_BITS; i++) {
        bool b = (coded[i / 8] >> (7 - (i % 8))) & 1u;
        out[i] = b ? 0xFFFFu : 0u;
    }
}

inline void soft_decorrelate(uint16_t bits[HORSE_VOICE_CODED_BITS])
{
    for (size_t i = 0; i < HORSE_VOICE_CODED_BYTES; i++) {
        uint8_t seq = M17::sequence[i];
        for (size_t j = 0; j < 8; j++) {
            if ((seq >> (7 - j)) & 1u)
                bits[i * 8 + j] = invert_soft(bits[i * 8 + j]);
        }
    }
}

inline void soft_deinterleave(uint16_t bits[HORSE_VOICE_CODED_BITS],
                              uint16_t tmp[HORSE_VOICE_CODED_BITS])
{
    static constexpr size_t F1 = 45;
    static constexpr size_t F2 = 92;
    const size_t nb = HORSE_VOICE_CODED_BITS;
    for (size_t i = 0; i < nb; i++) {
        size_t index = ((F1 * i) + (F2 * i * i)) % nb;
        tmp[i] = bits[index];
    }
    std::memcpy(bits, tmp, HORSE_VOICE_CODED_BITS * sizeof(uint16_t));
}

} // namespace horse

#endif
