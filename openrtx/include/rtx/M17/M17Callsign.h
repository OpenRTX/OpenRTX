/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#ifndef M17CALLSIGN_H
#define M17CALLSIGN_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include "M17Datatypes.h"

/**
 * Encode a callsign in base-40 format, starting with the right-most character.
 * The final value is written out in "big-endian" form, with the most-significant
 * value first, leading to 0-padding of callsigns shorter than nine characters.
 *
 * \param callsign the callsign to encode.
 * \param encodedCall call_t data structure where to put the encoded data.
 * \param strict a flag (disabled by default) which indicates whether invalid
 * characters are allowed and assigned a value of 0 or not allowed, making the
 * function return an error.
 * @return true if the callsign was successfully encoded, false on error.
 */
static bool encode_callsign(const std::string& callsign, call_t& encodedCall,
                     bool strict = false)
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

/**
 * Decode a base-40 encoded callsign to its text representation. This decodes
 * a 6-byte big-endian value into a string of up to 9 characters.
 *
 * \param encodedCall base-40 encoded callsign.
 * \return a string containing the decoded text.
 */
static std::string decode_callsign(const call_t& encodedCall)
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

#endif /* M17CALLSIGN_H */
