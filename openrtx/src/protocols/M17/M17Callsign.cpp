/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string>
#include "protocols/M17/M17Callsign.hpp"

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

    if(isBroadcast) return "ALL";

    /*
     * Address is not broadcast, decode it.
     * TODO: current implementation works only on little endian architectures.
     */
    static const char charMap[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/.";

    uint64_t encoded = 0;
    auto p = reinterpret_cast<uint8_t*>(&encoded);
    std::copy(encodedCall.rbegin(), encodedCall.rend(), p);

    // Decode each base-40 digit and map them to the appriate character.
    std::string result;
    size_t index = 0;

    while(encoded)
    {
        result.push_back(charMap[encoded % 40]);
        index++;
        encoded /= 40;
    }

    result[index] = '\0';

    return result;
}
