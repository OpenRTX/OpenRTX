/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/HorseFrameEncoder.hpp"
#include "protocols/horse/HorseUtils.hpp"
#include "protocols/horse/HorseVoiceCodec.hpp"
#include "protocols/horse/horse_crypto.h"
#include <cstring>

namespace horse
{

HorseFrameEncoder::HorseFrameEncoder() : voiceFrameNumber(0)
{
}

HorseFrameEncoder::~HorseFrameEncoder()
{
}

void HorseFrameEncoder::reset()
{
    voiceFrameNumber = 0;
}

void HorseFrameEncoder::encodeLsf(const call_t& src, const call_t& dst,
                                  const uint8_t* eph_pk, uint8_t flags,
                                  frame_t& output)
{
    lsf_raw_t payload;
    payload.fill(0);
    std::copy(src.begin(), src.end(), payload.begin());
    std::copy(dst.begin(), dst.end(), payload.begin() + LSF_CALLSIGN_BYTES);
    if (eph_pk != nullptr)
        std::memcpy(payload.data() + LSF_EPH_PK_OFFSET, eph_pk,
                    HORSE_X25519_PUBLICKEY_BYTES);
    payload[LSF_FLAGS_OFFSET] = flags;
    payload[LSF_VERSION_OFFSET] = LSF_PROTOCOL_VERSION;
    std::copy(LSF_SYNC_WORD.begin(), LSF_SYNC_WORD.end(), output.begin());
    std::copy(payload.begin(), payload.end(), output.begin() + 2);
}

uint16_t HorseFrameEncoder::encodeVoiceFrame(const uint8_t* melpe96bits,
                                            const uint8_t* tag32bits,
                                            frame_t& output, bool isLast)
{
    uint16_t fn = voiceFrameNumber & 0x7FFF;
    if (fn > VOICE_FN_MAX)
        fn = VOICE_FN_MAX;
    uint16_t out_fn = encodeVoiceFrameWithFn(melpe96bits, tag32bits, fn, output,
                                             isLast || (fn == VOICE_FN_MAX));
    if (fn < VOICE_FN_MAX)
        voiceFrameNumber = fn + 1;
    else
        voiceFrameNumber = SIG_FRAME_BASE;
    return out_fn;
}

uint16_t HorseFrameEncoder::encodeVoiceFrameWithFn(const uint8_t* melpe96bits,
                                                   const uint8_t* tag32bits,
                                                   uint16_t frame_num,
                                                   frame_t& output, bool isLast,
                                                   size_t payload_len)
{
    uint8_t info[HORSE_VOICE_INFO_BYTES];
    std::memset(info, 0, sizeof(info));
    uint16_t fn = frame_num & 0x7FFF;
    if (isLast)
        fn |= 0x8000;
    info[0] = (fn >> 8) & 0xFF;
    info[1] = fn & 0xFF;
    if (melpe96bits != nullptr && payload_len > 0) {
        if (payload_len > 12)
            payload_len = 12;
        std::memcpy(info + 2, melpe96bits, payload_len);
    }
    if (tag32bits != nullptr)
        std::memcpy(info + 14, tag32bits, 4);
    std::copy(VOICE_SYNC_WORD.begin(), VOICE_SYNC_WORD.end(), output.begin());
    voice_encode(info, output.data() + 2);
    return fn & 0x7FFF;
}

void HorseFrameEncoder::encodeEotFrame(frame_t& output)
{
    std::copy(EOT_SYNC_WORD.begin(), EOT_SYNC_WORD.end(), output.begin());
    std::fill(output.begin() + 2, output.end(), 0);
}

uint16_t HorseFrameEncoder::currentVoiceFrameNumber() const
{
    return voiceFrameNumber & 0x7FFF;
}

}  // namespace horse
