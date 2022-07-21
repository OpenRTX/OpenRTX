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

#include <cstring>
#include <M17/M17Golay.hpp>
#include <M17/M17Callsign.hpp>
#include <M17/M17LinkSetupFrame.hpp>

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

void M17LinkSetupFrame::setSource(const std::string& callsign)
{
    encode_callsign(callsign, data.src);
}

std::string M17LinkSetupFrame::getSource()
{
    return decode_callsign(data.src);
}

void M17LinkSetupFrame::setDestination(const std::string& callsign)
{
    encode_callsign(callsign, data.dst);
}

std::string M17LinkSetupFrame::getDestination()
{
    return decode_callsign(data.dst);
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

lich_t M17LinkSetupFrame::generateLichSegment(const uint8_t segmentNum)
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
    lich_t result;
    for(size_t i = 0; i < blocks.size(); i++)
    {
        uint32_t encoded = golay24_encode(blocks[i]);
        encoded          = __builtin_bswap32(encoded << 8);
        memcpy(&result[3*i], &encoded, 3);
    }

    return result;
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
