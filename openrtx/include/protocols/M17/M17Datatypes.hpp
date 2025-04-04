/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

enum M17DataMode
{
    M17_DATAMODE_PACKET = 0,
    M17_DATAMODE_STREAM = 1
};

enum M17DataType
{
    M17_DATATYPE_DATA       = 1,
    M17_DATATYPE_VOICE      = 2,
    M17_DATATYPE_VOICE_DATA = 3
};

enum M17EncyptionType
{
    M17_ENCRYPTION_NONE      = 0,
    M17_ENCRYPTION_AES       = 1,
    M17_ENCRYPTION_SCRAMBLER = 2,
    M17_ENCRYPTION_OTHER     = 3,
};

enum M17MetaType
{
    M17_META_TEXT          = 0,
    M17_META_GNSS          = 1,
    M17_META_EXTD_CALLSIGN = 2,
};

enum M17ScramblingType
{
    M17_SCRAMBLING_8BIT     = 0,
    M17_SCRAMBLING_16BIT    = 1,
    M17_SCRAMBLING_24BIT    = 2,
};


/**
 * Data structure for M17 GNSS metadata field.
 */
typedef struct __attribute__((packed))
{
    uint8_t  data_src;          //< Data source
    uint8_t  station_type;      //< Station type
    uint8_t  lat_deg;           //< Latitude, whole number
    uint16_t lat_dec;           //< Latitude, decimal part multiplied by 65535
    uint8_t  lon_deg;           //< Longitude, whole number
    uint16_t lon_dec;           //< Longitude, decimal part multiplied by 65535
    uint8_t  lat_sign  : 1;     //< Latitude N/S: 0 = north, 1 = south
    uint8_t  lon_sign  : 1;     //< Longitude E/W: 0 = east, 1 = west
    uint8_t  alt_valid : 1;     //< Altitude data valid
    uint8_t  spd_valid : 1;     //< Speed data valid
    uint8_t  _unused   : 4;
    uint16_t altitude;          //< Altitude above sea level in feet + 1500
    uint16_t bearing;           //< Bearing in degrees, whole number
    uint8_t  speed;             //< Speed in mph, whole number
}
gnssData_t;

/**
 * Data structure for M17 extended callsign metadata field.
 */
typedef struct __attribute__((packed))
{
    call_t   call1;
    call_t   call2;
    uint16_t _unused;
}
extdCall_t;

/**
 * Data structure for M17 LSF metadata field.
 */
typedef union
{
    extdCall_t extended_call_sign;
    gnssData_t gnss_data;
    uint8_t    raw_data[14];
}
meta_t;

/**
 * This structure provides bit field definitions for the "TYPE" field
 * contained in an M17 Link Setup Frame.
 */
typedef union
{
    struct __attribute__((packed))
    {
        uint16_t dataMode   : 1;    //< Packet/stream indicator: 0 = packet, 1 = stream
        uint16_t dataType   : 2;    //< Data type indicator
        uint16_t encType    : 2;    //< Encryption type
        uint16_t encSubType : 2;    //< Encryption subtype
        uint16_t CAN        : 4;    //< Channel Access Number
        uint16_t            : 5;    //< Reserved, padding to 16 bit
    } fields;

    uint16_t value;
}
streamType_t;

}  // namespace M17

#endif  // M17_DATATYPES_H
