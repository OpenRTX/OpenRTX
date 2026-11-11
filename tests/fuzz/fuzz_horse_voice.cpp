/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Input: 46 coded bytes; exercises Horse option-C voice decode.
 */

#include "protocols/horse/HorseVoiceCodec.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint8_t coded[horse::HORSE_VOICE_CODED_BYTES];
    uint8_t info[horse::HORSE_VOICE_INFO_BYTES];
    uint8_t spare[horse::HORSE_VOICE_SPARE_BYTES];

    std::memset(coded, 0, sizeof coded);
    if (size > 0)
        std::memcpy(coded, data, size < sizeof coded ? size : sizeof coded);
    horse::voice_decode(coded, info);
    horse::voice_extract_spare(coded, spare);
    horse::voice_encode(info, coded);
    return 0;
}
