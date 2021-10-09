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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
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
    };

    uint16_t value;
}
streamType_t;


/**
 * Data structure corresponding to a full M17 Link Setup Frame.
 */
typedef struct
{
    call_t       dst;    //< Destination callsign
    call_t       src;    //< Source callsign
    streamType_t type;   //< Stream type information
    meta_t       meta;   //< Metadata
    uint16_t     crc;    //< CRC
}
__attribute__((packed)) lsf_t;


/**
 * Data structure corresponding to a full M17 data frame.
 */
typedef struct
{
    uint16_t   frameNum;  //< Frame number
    payload_t  payload;   //< Payload data
}
__attribute__((packed)) dataFrame_t;

#endif /* M17_DATATYPES_H */
