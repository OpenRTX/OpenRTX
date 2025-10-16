/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstring>
#include "protocols/M17/M17Golay.hpp"
#include "protocols/M17/M17Callsign.hpp"
#include "protocols/M17/M17LinkSetupFrame.hpp"
#include "core/utils.h"

using namespace M17;

M17LinkSetupFrame::M17LinkSetupFrame()
{
    clear();
}

M17LinkSetupFrame::~M17LinkSetupFrame()
{

}

void M17LinkSetupFrame::clear()
{
    memset(&data, 0x00, sizeof(data));
    data.dst.fill(0xFF);
}

void M17LinkSetupFrame::setSource(const Callsign& callsign)
{
    data.src = callsign;
}

Callsign M17LinkSetupFrame::getSource()
{
    return Callsign(data.src);
}

void M17LinkSetupFrame::setDestination(const Callsign& callsign)
{
    data.dst = callsign;
}

Callsign M17LinkSetupFrame::getDestination()
{
    return Callsign(data.dst);
}

streamType_t M17LinkSetupFrame::getType()
{
    // NOTE: M17 fields are big-endian, we need to swap bytes
    streamType_t type = data.type;
    type.value = __builtin_bswap16(type.value);
    return type;
}

void M17LinkSetupFrame::setType(streamType_t type)
{
    // NOTE: M17 fields are big-endian, we need to swap bytes
    type.value = __builtin_bswap16(type.value);
    data.type  = type;
}

meta_t& M17LinkSetupFrame::metadata()
{
    return data.meta;
}

void M17LinkSetupFrame::updateCrc()
{
    // Compute CRC over the first 28 bytes, then store it in big endian format.
    uint16_t crc = crc16(&data, 28);
    data.crc     = __builtin_bswap16(crc);
}

bool M17LinkSetupFrame::valid() const
{
    uint16_t crc = crc16(&data, 28);
    if(data.crc == __builtin_bswap16(crc)) return true;

    return false;
}

const uint8_t * M17LinkSetupFrame::getData()
{
    return reinterpret_cast < const uint8_t * >(&data);
}

void M17LinkSetupFrame::generateLichSegment(lich_t &segment,
                                            const uint8_t segmentNum)
{
    /*
     * The M17 protocol specification prescribes that the content of the
     * link setup frame is continuously transmitted alongside data frames
     * by partitioning it in 6 chunks of five bites each and cyclically
     * transmitting these chunks.
     * With a bit of pointer math, we extract the data for each of the
     * chunks by casting the a pointer lsf_t data structure to uint8_t
     * and adjusting it to make it point to the start of each block of
     * five bytes, as specified by the segmentNum parameter.
     */

    // Set up pointer to the beginning of the specified 5-byte chunk
    uint8_t num    = segmentNum % 6;
    uint8_t *chunk = reinterpret_cast< uint8_t* >(&data) + (num * 5);

    // Partition chunk data in 12-bit blocks for Golay(24,12) encoding.
    std::array< uint16_t, 4 > blocks;
    blocks[0] = chunk[0] << 4 | ((chunk[1] >> 4) & 0x0F);
    blocks[1] = ((chunk[1] & 0x0F) << 8) | chunk[2];
    blocks[2] = chunk[3] << 4 | ((chunk[4] >> 4) & 0x0F);
    blocks[3] = ((chunk[4] & 0x0F) << 8) | (num << 5);

    // Encode each block and assemble the final data block.
    // NOTE: shift and bitswap required to genereate big-endian data.
    for(size_t i = 0; i < blocks.size(); i++)
    {
        uint32_t encoded = golay24_encode(blocks[i]);
        encoded          = __builtin_bswap32(encoded << 8);
        memcpy(&segment[3*i], &encoded, 3);
    }
}

void M17LinkSetupFrame::setGnssData(const gps_t *position,
                                    const M17GNSSStationType stationType)
{
    if(position->fix_type < FIX_TYPE_2D)
        return;

    streamType_t streamType = getType();
    streamType.fields.encType = M17_ENCRYPTION_NONE;
    streamType.fields.encSubType = M17_META_GNSS;
    setType(streamType);

    data.meta.gnss_data.data_src = M17_GNSS_SOURCE_OPENRTX;
    data.meta.gnss_data.station_type = stationType;

    data.meta.gnss_data.coords_valid = 1;
    data.meta.gnss_data.alt_valid = 1;
    data.meta.gnss_data.velocity_valid = 1;
    data.meta.gnss_data.radius_valid = 0;

    data.meta.gnss_data.radius = position->hdop;

    data.meta.gnss_data.bearing_1 = (position->tmg_true >> 8) & 0x01; // MSB (1 bit)
    data.meta.gnss_data.bearing_2 = position->tmg_true & 0xFF; // Lower 8 bits

    // Encode the coordinates in Q1.24 format, swap the byte order to big
    // endian as required by M17 specification
    int32_t lat_encoded = coordToFixedPoint(position->latitude, 90);
    int32_t lon_encoded = coordToFixedPoint(position->longitude, 180);
    for(uint8_t i = 0; i < 3; i++) {
        data.meta.gnss_data.latitude_bytes[i] = *((uint8_t*)&lat_encoded + 2 - i);
        data.meta.gnss_data.longitude_bytes[i] = *((uint8_t*)&lon_encoded + 2 - i);
    }

    uint16_t speed = (uint16_t)position->speed * 2;
    data.meta.gnss_data.speed_1 = speed >> 4; // MBS
    data.meta.gnss_data.speed_2 = speed & 0x0F; // Lower 4 bits

    // Structure numeric fields that need offset and steps
    uint16_t alt = (uint16_t)1000 + position->altitude * 2;
    data.meta.gnss_data.altitude = __builtin_bswap16(alt);
}

uint16_t M17LinkSetupFrame::crc16(const void *data, const size_t len) const
{
    const uint8_t *ptr = reinterpret_cast< const uint8_t *>(data);
    uint16_t crc = 0xFFFF;

    for(size_t i = 0; i < len; i++)
    {
        crc ^= (ptr[i] << 8);

        for(uint8_t j = 0; j < 8; j++)
        {
            if(crc & 0x8000)
                crc = (crc << 1) ^ 0x5935;
            else
                crc = (crc << 1);
        }
    }

    return crc;
}
