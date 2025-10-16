/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_CONSTANTS_H
#define M17_CONSTANTS_H

#include "protocols/M17/M17Datatypes.hpp"
#include <cstdint>
#include <array>

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace M17
{

static constexpr size_t M17_SYMBOL_RATE      = 4800;
static constexpr size_t M17_FRAME_SYMBOLS    = 192;
static constexpr size_t M17_SYNCWORD_SYMBOLS = 8;
static constexpr size_t M17_FRAME_BYTES      = M17_FRAME_SYMBOLS / 4;
static constexpr size_t NUM_LSF_CHUNKS       = 6;

static constexpr syncw_t LSF_SYNC_WORD    = {0x55, 0xF7};  // LSF sync word
static constexpr syncw_t BERT_SYNC_WORD   = {0xDF, 0x55};  // BERT data sync word
static constexpr syncw_t STREAM_SYNC_WORD = {0xFF, 0x5D};  // Stream data sync word
static constexpr syncw_t PACKET_SYNC_WORD = {0x75, 0xFF};  // Packet data sync word
static constexpr syncw_t EOT_SYNC_WORD    = {0x55, 0x5D};  // End of transmission sync word

}      // namespace M17

#endif // M17_CONSTANTS_H
