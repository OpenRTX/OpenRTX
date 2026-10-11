/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_FRAME_ENCODER_H
#define HORSE_FRAME_ENCODER_H

#include "HorseDatatypes.hpp"
#include "HorseConstants.hpp"
#include <cstdint>
#include <cstddef>

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace horse
{

class HorseFrameEncoder
{
public:
    HorseFrameEncoder();
    ~HorseFrameEncoder();

    void reset();

    /**
     * \brief Encode a three-frame opening LSF (CRC-16 + M17 DATA_PUNCTURE).
     *        Also stores the 48-byte LSF||CRC for voice-frame fragments.
     */
    void encodeLsf(const call_t &src, const call_t &dst, const uint8_t *eph_pk,
                   uint8_t flags, frame_t out[LSF_OPENING_FRAMES]);

    /**
     * \brief Set the 64-byte signature for fragment slots 4..9.
     *        Pass nullptr to clear (encrypt-only: slots 4..9 are zeros).
     */
    void setSignatureFragments(const uint8_t *sig64);

    uint16_t encodeVoiceFrame(const uint8_t *melpe96bits,
                              const uint8_t *tag32bits, frame_t &output,
                              bool isLast = false);

    uint16_t encodeVoiceFrameWithFn(const uint8_t *melpe96bits,
                                    const uint8_t *tag32bits,
                                    uint16_t frame_num, frame_t &output,
                                    bool isLast = false,
                                    size_t payload_len = 12);

    void encodeEotFrame(frame_t &output);

    uint16_t currentVoiceFrameNumber() const;

private:
    void fillFragment(uint16_t fn, uint8_t spare[HORSE_FRAG_BYTES]) const;

    uint16_t voiceFrameNumber;
    uint8_t lsfWithCrc[LSF_WITH_CRC_BYTES];
    uint8_t sigBytes[SIG_BYTES];
    bool haveLsfFrag;
    bool haveSigFrag;
};

} // namespace horse

#endif // HORSE_FRAME_ENCODER_H
