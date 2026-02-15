/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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

enum M17GNSSSource {
    M17_GNSS_SOURCE_M17CLIENT = 0,
    M17_GNSS_SOURCE_OPENRTX = 1
};

enum M17GNSSStationType {
    M17_GNSS_STATION_FIXED = 0,
    M17_GNSS_STATION_MOBILE = 1,
    M17_GNSS_STATION_HANDHELD = 2
};

/**
 * Data structure for M17 GNSS metadata field.
 * The fields are individually set to big endian.
 */
typedef struct __attribute__((packed))
{
    uint8_t     station_type    : 4;  //< Station type
    uint8_t     data_src        : 4;  //< Data source
    uint8_t     bearing_1       : 1;  //< MSB of bearing
    uint8_t     radius          : 3;  //< estimate of lateral uncertainty, based on HDOP value
    uint8_t     radius_valid    : 1;  //< radius data valid
    uint8_t     velocity_valid  : 1;  //< speed and bearing valid
    uint8_t     alt_valid       : 1;  //< Altitude data valid
    uint8_t     coords_valid    : 1;  //< Coordinate data valid
    uint8_t     bearing_2       : 8;  //< Lower 8 bits of bearing
    int8_t  latitude_bytes[3];        //< Latitude, twos complement, positive north
    int8_t  longitude_bytes[3];       //< Longitude, twos complement, positive north
    uint16_t    altitude        : 16; //< Numeric altitude in 0.5m steps offset by 500m
    uint16_t    speed_1         : 8,  //< Numeric speed in 0.5 km/h steps; MSB
                                : 4,
                speed_2         : 4;  //< LSB
    uint8_t                     : 8;
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
