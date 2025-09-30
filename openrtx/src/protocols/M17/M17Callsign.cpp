/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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
#include "protocols/M17/M17Callsign.hpp"

using namespace M17;

Callsign::Callsign(std::string callsign){
    call = callsign;
};

Callsign::Callsign(char* callsign){
    call = callsign;
};

Callsign::Callsign(const call_t encodedCall){
    call = decode_callsign(encodedCall);
};

bool Callsign::encode_callsign(const std::string& callsign, call_t& encodedCall,
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

std::string Callsign::decode_callsign(const call_t& encodedCall)
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

Callsign::operator std::string()
{
    return this->call;
}


Callsign::operator call_t()
{
    call_t encodedCall;
    encode_callsign(this->call, encodedCall);
    return encodedCall;
}

/**
 * NOTE! since only incomingCs is checked for special values, the second arg must be the incoming station
 */
bool compareCallsigns(const std::string& localCs,
                                  const std::string& incomingCs)
{
    if((incomingCs == "ALL") || (incomingCs == "INFO") || (incomingCs == "ECHO"))
        return true;

    std::string truncatedLocal(localCs);
    std::string truncatedIncoming(incomingCs);

    int slashPos = localCs.find_first_of('/');
    if(slashPos <= 2)
        truncatedLocal = localCs.substr(slashPos + 1);

    slashPos = incomingCs.find_first_of('/');
    if(slashPos <= 2)
        truncatedIncoming = incomingCs.substr(slashPos + 1);

    if(truncatedLocal == truncatedIncoming)
        return true;

    return false;
}

// NOTE! Since this uses compareCallsigns internally, always have the right side of the equality check be the incoming callsign
bool Callsign::operator==(const Callsign& other)
{
    return compareCallsigns(this->call, other.call);
}
