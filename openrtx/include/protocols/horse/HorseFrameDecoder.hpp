/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_FRAME_DECODER_H
#define HORSE_FRAME_DECODER_H

#include "HorseDatatypes.hpp"
#include "HorseConstants.hpp"
#include <cstdint>

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace horse
{

class HorseFrameDecoder
{
public:
    HorseFrameDecoder();
    ~HorseFrameDecoder();

    void reset();

    HorseFrameType decodeFrame(const frame_t& frame);

    void getLsfCallsigns(call_t& src, call_t& dst);

    bool getLsfCrypto(const frame_t &frame, uint8_t eph_pk[32], uint8_t *flags,
                      uint8_t *version = nullptr);

    void getVoicePayload(const frame_t& frame, uint8_t* melpe96bits,
                        uint8_t* tag32bits, uint16_t* frameNum);

private:
    call_t lsfSrc;
    call_t lsfDst;
    uint16_t lastVoiceFrameNum;
};

}  // namespace horse

#endif  // HORSE_FRAME_DECODER_H
