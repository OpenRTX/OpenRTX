/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * LibFuzzer harness for Horse frame decode: opening LSF chunks, voice
 * with spare fragments, and EOT.
 */

#include "protocols/horse/HorseFrameDecoder.hpp"
#include "protocols/horse/HorseConstants.hpp"
#include <cstdint>
#include <cstring>

using namespace horse;

static constexpr size_t FRAME_LEN = 48;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < FRAME_LEN)
        return 0;

    HorseFrameDecoder dec;
    const size_t nframes = size / FRAME_LEN;
    for (size_t i = 0; i < nframes; i++) {
        frame_t frame;
        std::memcpy(frame.data(), data + i * FRAME_LEN, FRAME_LEN);
        HorseFrameType type = dec.decodeFrame(frame);
        if (type == HorseFrameType::LINK_SETUP
            || (type == HorseFrameType::VOICE && dec.lsfReady())) {
            call_t src, dst;
            dec.getLsfCallsigns(src, dst);
            uint8_t eph[32], flags = 0, version = 0;
            (void)dec.getLsfCrypto(eph, &flags, &version);
            (void)src;
            (void)dst;
        }
        if (type == HorseFrameType::VOICE) {
            uint8_t melpe[12];
            uint8_t tag[4];
            uint16_t frameNum = 0;
            dec.getVoicePayload(frame, melpe, tag, &frameNum);
            uint8_t sig[64];
            (void)dec.getSigFragments(sig);
            (void)frameNum;
        }
    }
    return 0;
}
