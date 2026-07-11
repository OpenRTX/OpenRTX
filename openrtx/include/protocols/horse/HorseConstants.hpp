/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_CONSTANTS_H
#define HORSE_CONSTANTS_H

#include "HorseDatatypes.hpp"
#include <cstddef>

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace horse
{

static constexpr size_t SYMBOL_RATE       = 4800;
static constexpr size_t FRAME_SYMBOLS     = 192;
static constexpr size_t SYNCWORD_SYMBOLS  = 8;
static constexpr size_t FRAME_BYTES       = FRAME_SYMBOLS / 4;

static constexpr syncw_t LSF_SYNC_WORD    = {0x5A, 0xA7};
static constexpr syncw_t VOICE_SYNC_WORD  = {0x7E, 0x9B};
static constexpr syncw_t EOT_SYNC_WORD    = {0x3C, 0xD8};

static constexpr size_t LSF_CALLSIGN_BYTES   = 6;
static constexpr size_t LSF_EPH_PK_OFFSET    = 12;
static constexpr size_t LSF_FLAGS_OFFSET     = 44;
static constexpr uint8_t  LSF_FLAG_ENCRYPTED = 0x01;
static constexpr uint8_t  LSF_FLAG_SIGNED    = 0x02;
static constexpr uint16_t SIG_FRAME_BASE     = 0x7000;
static constexpr uint16_t SIG_FRAME_COUNT   = 6;
static constexpr size_t VOICE_FRAME_COUNTER_BITS = 16;
static constexpr size_t VOICE_MELPE_BITS         = 96;
static constexpr size_t VOICE_TAG_BITS           = 32;

}  // namespace horse

#endif  // HORSE_CONSTANTS_H
