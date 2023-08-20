/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

namespace M17
{

using call_t    = std::array< uint8_t, 6 >;    // Data type for encoded callsign
using payload_t = std::array< uint8_t, 16 >;   // Data type for frame payload field
using lich_t    = std::array< uint8_t, 12 >;   // Data type for Golay(24,12) encoded LICH data
using frame_t   = std::array< uint8_t, 48 >;   // Data type for a full M17 data frame, including sync word
using syncw_t   = std::array< uint8_t, 2  >;   // Data type for a sync word

enum m17_lsf_packet_stream_indicator
{
    M17_LSF_PACKET_MODE = 0,
    M17_LSF_STREAM_MODE = 1
};

enum m17_lsf_data_type
{
    M17_LSF_DATATYPE_RESERVED       = 0,
    M17_LSF_DATATYPE_DATA           = 1,
    M17_LSF_DATATYPE_VOICE          = 2,
    M17_LSF_DATATYPE_VOICE_AND_DATA = 3
};

enum m17_lsf_encryption_type
{
    M17_LSF_ENCRYPTION_NONE      = 0,
    M17_LSF_ENCRYPTION_AES       = 1,
    M17_LSF_ENCRYPTION_SCRAMBLER = 2,
    M17_LSF_ENCRYPTION_OTHER     = 3,
};

enum m17_lsf_null_encryption_subtype
{
    M17_LSF_NULL_ENCRYPTION_TEXT              = 0,
    M17_LSF_NULL_ENCRYPTION_GNSS              = 1,
    M17_LSF_NULL_ENCRYPTION_EXTENDED_CALLSIGN = 2,
    M17_LSF_NULL_ENCRYPTION_RESERVED          = 3,
};

enum m17_lsf_scrambling_subtype
{
    M17_LSF_SCRAMBLING_8BIT     = 0,
    M17_LSF_SCRAMBLING_16BIT    = 1,
    M17_LSF_SCRAMBLING_24BIT    = 2,
    M17_LSF_SCRAMBLING_RESERVED = 3,
};

union __attribute__((__packed__))
{
    m17_lsf_null_encryption_subtype null_subtype : 2;
    m17_lsf_scrambling_subtype scramble_subtype  : 2;
} encryption_subtype;

typedef struct
{
    call_t call1;
    call_t call2;
    uint16_t unused;
} extended_call_sign_t;

typedef struct __attribute__((packed))
{
    uint8_t data_source;
    uint8_t station_type;
    uint8_t whole_degree_latitude;
    uint16_t decimal_degree_latitude;
    uint8_t whole_degree_longitude;
    uint16_t decimal_degree_longitude;
    uint8_t latitude_sign       : 1;
    uint8_t longitude_sign      : 1;
    uint8_t altitude_valid      : 1;
    uint8_t speed_bearing_valid : 1;
    uint8_t unused              : 4;
    uint16_t altitude;
    uint16_t bearing;
    uint8_t speed;
} gnss_data_t;

typedef union
{
    extended_call_sign_t extended_call_sign;
    gnss_data_t gnss_data;
    std::array<uint8_t, 14> data;
} meta_t;

/**
 * This structure provides bit field definitions for the "TYPE" field
 * contained in an M17 Link Setup Frame.
 */
typedef union
{
    struct __attribute__((packed))
    {
        m17_lsf_packet_stream_indicator stream : 1;  //< Packet/stream indicator: 0 = packet, 1 = stream
        m17_lsf_data_type dataType             : 2;  //< Data type indicator
        m17_lsf_encryption_type encType        : 2;  //< Encryption type
        uint8_t encryption_subtype             : 2;  //< Encryption subtype
        uint16_t CAN                           : 4;  //< Channel Access Number
        uint16_t                               : 5;  //< Reserved, padding to 16 bit
    } fields;

    uint16_t value;
} streamType_t;

}  // namespace M17

#endif  // M17_DATATYPES_H
