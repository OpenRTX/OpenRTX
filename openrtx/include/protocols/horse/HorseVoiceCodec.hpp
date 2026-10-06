/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_VOICE_CODEC_H
#define HORSE_VOICE_CODEC_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "protocols/horse/HorseConstants.hpp"
#include "protocols/M17/Viterbi.hpp"
#include <array>
#include <cstdint>
#include <cstddef>

namespace horse
{

/**
 * \brief M17 option-C voice codec with decoder state held off the call stack
 *        (same pattern as M17::FrameDecoder's HardViterbi member).
 */
class HorseVoiceCodec
{
public:
    void encode(const uint8_t info[HORSE_VOICE_INFO_BYTES],
                uint8_t coded[HORSE_VOICE_CODED_BYTES]);

    void encode_with_spare(const uint8_t info[HORSE_VOICE_INFO_BYTES],
                           const uint8_t *spare12,
                           uint8_t coded[HORSE_VOICE_CODED_BYTES]);

    void decode(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                uint8_t info[HORSE_VOICE_INFO_BYTES]);

    void extract_spare(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                       uint8_t spare12[HORSE_VOICE_SPARE_BYTES]);

private:
    M17::HardViterbi vit;
    std::array<uint8_t, HORSE_VOICE_CODED_BYTES> frameBuf;
    std::array<uint8_t, HORSE_VOICE_PUNCT_BYTES> punctBuf;
    std::array<uint8_t, HORSE_VOICE_INFO_BYTES> infoBuf;
    std::array<uint8_t, 37> convBuf;
};

/* Free-function wrappers for unit tests / FEC sim (static instance). */
void voice_encode(const uint8_t info[HORSE_VOICE_INFO_BYTES],
                  uint8_t coded[HORSE_VOICE_CODED_BYTES]);
void voice_decode(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                  uint8_t info[HORSE_VOICE_INFO_BYTES]);
void voice_encode_with_spare(const uint8_t info[HORSE_VOICE_INFO_BYTES],
                             const uint8_t *spare12,
                             uint8_t coded[HORSE_VOICE_CODED_BYTES]);
void voice_extract_spare(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                         uint8_t spare12[HORSE_VOICE_SPARE_BYTES]);

} /* namespace horse */

#endif
