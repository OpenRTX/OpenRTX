/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_DATATYPES_H
#define HORSE_DATATYPES_H

#include <cstdint>
#include <array>

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace horse
{

using call_t = std::array<uint8_t, 6>;
using frame_t = std::array<uint8_t, 48>;
using syncw_t = std::array<uint8_t, 2>;
using lsf_raw_t = std::array<uint8_t, 46>;
using voice_raw_t = std::array<uint8_t, 23>;

enum class HorseFrameType : uint8_t {
    LINK_SETUP = 0,
    VOICE = 1,
    EOT = 2,
    UNKNOWN = 3
};

} // namespace horse

#endif // HORSE_DATATYPES_H
