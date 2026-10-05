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
     */
    void encodeLsf(const call_t& src, const call_t& dst, const uint8_t* eph_pk,
                   uint8_t flags, frame_t out[LSF_OPENING_FRAMES]);

    uint16_t encodeVoiceFrame(const uint8_t* melpe96bits, const uint8_t* tag32bits,
                              frame_t& output, bool isLast = false);

    uint16_t encodeVoiceFrameWithFn(const uint8_t* melpe96bits,
                                    const uint8_t* tag32bits, uint16_t frame_num,
                                    frame_t& output, bool isLast = false,
                                    size_t payload_len = 12);

    void encodeEotFrame(frame_t& output);

    uint16_t currentVoiceFrameNumber() const;

private:
    uint16_t voiceFrameNumber;
};

}  // namespace horse

#endif  // HORSE_FRAME_ENCODER_H
