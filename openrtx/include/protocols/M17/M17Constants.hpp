/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef M17_CONSTANTS_H
#define M17_CONSTANTS_H

#include <M17/M17Datatypes.hpp>
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

static constexpr syncw_t LSF_SYNC_WORD    = {0x55, 0xF7};  // LSF sync word
static constexpr syncw_t BERT_SYNC_WORD   = {0xDF, 0x55};  // BERT data sync word
static constexpr syncw_t STREAM_SYNC_WORD = {0xFF, 0x5D};  // Stream data sync word
static constexpr syncw_t PACKET_SYNC_WORD = {0x75, 0xFF};  // Packet data sync word
static constexpr syncw_t EOT_SYNC_WORD    = {0x55, 0x5D};  // End of transmission sync word

}      // namespace M17

#endif // M17_CONSTANTS_H
