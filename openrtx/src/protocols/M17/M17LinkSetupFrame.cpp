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
#include <M17/M17Callsign.h>
#include <M17/M17LinkSetupFrame.h>

M17LinkSetupFrame::M17LinkSetupFrame()
{
    clear();
}

M17LinkSetupFrame::~M17LinkSetupFrame()
{

}

void M17LinkSetupFrame::clear()
{
    memset(&data, 0x00, sizeof(lsf_t));
    data.dst.fill(0xFF);
}

void M17LinkSetupFrame::setSource(const std::string& callsign)
{
    encode_callsign(callsign, data.src);
}

void M17LinkSetupFrame::setDestination(const std::string& callsign)
{
    encode_callsign(callsign, data.dst);
}

streamType_t M17LinkSetupFrame::getType()
{
    // NOTE: M17 fields are big-endian, we need to swap bytes
    uint16_t *a = reinterpret_cast< uint16_t* >(&data.type);
    uint16_t  b = __builtin_bswap16(*a);
    return *reinterpret_cast< streamType_t* >(&b);
}

void M17LinkSetupFrame::setType(streamType_t type)
{
    // NOTE: M17 fields are big-endian, we need to swap bytes
    uint16_t *a = reinterpret_cast< uint16_t* >(&type);
    uint16_t  b = __builtin_bswap16(*a);
    data.type   = *reinterpret_cast< streamType_t* >(&b);
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

lsf_t& M17LinkSetupFrame::getData()
{
    return data;
}

std::array< uint8_t, sizeof(lsf_t) > M17LinkSetupFrame::toArray()
{
    std::array< uint8_t, sizeof(lsf_t) > frame;
    memcpy(frame.data(), &data, frame.size());
    return frame;
}

uint16_t M17LinkSetupFrame::crc16(const void *data, const size_t len)
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
