/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef M17_DATATYPES_H
#define M17_DATATYPES_H

#include <cstdint>
#include <array>

#ifndef __cplusplus
#error This header is C++ only!
#endif


using call_t    = std::array< uint8_t, 6 >;    // Data type for encoded callsign
using meta_t    = std::array< uint8_t, 14 >;   // Data type for LSF metadata field
using payload_t = std::array< uint8_t, 16 >;   // Data type for frame payload field
using lich_t    = std::array< uint8_t, 12 >;   // Data type for Golay(24,12) encoded LICH data

static constexpr std::array<uint8_t, 2> LSF_SYNC_WORD    = {0x55, 0xF7};  // LSF sync word
static constexpr std::array<uint8_t, 2> DATA_SYNC_WORD   = {0xFF, 0x5D};  // Stream data sync word
static constexpr std::array<uint8_t, 2> PACKET_SYNC_WORD = {0x75, 0xFF};  // Packet data sync word


/**
 * This structure provides bit field definitions for the "TYPE" field
 * contained in an M17 Link Setup Frame.
 */
typedef union
{
    struct __attribute__((packed))
    {
        uint16_t stream     : 1;    //< Packet/stream indicator: 0 = packet, 1 = stream
        uint16_t dataType   : 2;    //< Data type indicator
        uint16_t encType    : 2;    //< Encryption type
        uint16_t encSubType : 2;    //< Encryption subtype
        uint16_t CAN        : 4;    //< Channel Access Number
        uint16_t            : 4;    //< Reserved, padding to 16 bit
    }
    fields;

    uint16_t value;
}
streamType_t;

#endif /* M17_DATATYPES_H */
