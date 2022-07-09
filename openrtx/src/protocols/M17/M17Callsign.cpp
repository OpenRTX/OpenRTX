/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   Adapted from original code written by Rob Riggs, Mobilinkd LLC        *
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

#include <string>
#include <M17/M17Callsign.hpp>

bool M17::encode_callsign(const std::string& callsign, call_t& encodedCall,
                          bool strict)
{
    encodedCall.fill(0x00);
    if(callsign.size() > 9) return false;

    // Encode the characters to base-40 digits.
    uint64_t encoded = 0;

    for(auto it = callsign.rbegin(); it != callsign.rend(); ++it)
    {
        encoded *= 40;
        if (*it >= 'A' and *it <= 'Z')
        {
            encoded += (*it - 'A') + 1;
        }
        else if (*it >= '0' and *it <= '9')
        {
            encoded += (*it - '0') + 27;
        }
        else if (*it == '-')
        {
            encoded += 37;
        }
        else if (*it == '/')
        {
            encoded += 38;
        }
        else if (*it == '.')
        {
            encoded += 39;
        }
        else if (strict)
        {
            return false;
        }
    }

    auto *ptr = reinterpret_cast< uint8_t *>(&encoded);
    std::copy(ptr, ptr + 6, encodedCall.rbegin());

    return true;
}

std::string M17::decode_callsign(const call_t& encodedCall)
{
    // First of all, check if encoded address is a broadcast one
    bool isBroadcast = true;
    for(auto& elem : encodedCall)
    {
        if(elem != 0xFF)
        {
            isBroadcast = false;
            break;
        }
    }

    if(isBroadcast) return "BROADCAST";

    /*
     * Address is not broadcast, decode it.
     * TODO: current implementation works only on little endian architectures.
     */
    static const char charMap[] = "xABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/.";

    uint64_t encoded = 0;
    auto p = reinterpret_cast<uint8_t*>(&encoded);
    std::copy(encodedCall.rbegin(), encodedCall.rend(), p);

    // Decode each base-40 digit and map them to the appriate character.
    std::string result;
    size_t index = 0;

    while(encoded)
    {
        result[index++] = charMap[encoded % 40];
        encoded /= 40;
    }

    result[index] = '\0';

    return result;
}
