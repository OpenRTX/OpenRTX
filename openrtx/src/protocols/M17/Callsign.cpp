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
#include <cstring>
#include "protocols/M17/Callsign.hpp"

namespace M17
{

static constexpr const call_t BROADCAST_CALL = { 0xFF, 0xFF, 0xFF,
                                                 0xFF, 0xFF, 0xFF };
static constexpr const char BROADCAST_CALL_STR[] = "ALL";

static constexpr const call_t INVALID_CALL = { 0x00, 0x00, 0x00,
                                               0x00, 0x00, 0x00 };
static constexpr const char INVALID_CALL_STR[] = "INVALID";

Callsign &getBroadcastCallsign()
{
    static Callsign instance(BROADCAST_CALL);
    return instance;
}

Callsign &getInvalidCallsign()
{
    static Callsign instance(INVALID_CALL);
    return instance;
}

} // namespace M17

using namespace M17;

Callsign::Callsign(std::string callsign)
{
    std::strncpy(call, callsign.c_str(), CALLSIGN_MAX_CHARS);
    call[CALLSIGN_MAX_CHARS] = '\0';
}

Callsign::Callsign(const char *callsign)
{
    std::strncpy(call, callsign, CALLSIGN_MAX_CHARS);
    call[CALLSIGN_MAX_CHARS] = '\0';
}

Callsign::Callsign(const call_t encodedCall)
{
    decode_callsign(encodedCall, this->call);
}

bool Callsign::encode_callsign(const std::string &callsign, call_t &encodedCall,
                               bool strict) const
{
    if (callsign.size() > 9)
        return false;
    // Return the special broadcast callsign if "ALL"
    if (callsign.compare(BROADCAST_CALL_STR) == 0) {
        encodedCall = BROADCAST_CALL;
        return true;
    }
    if (callsign.compare(INVALID_CALL_STR) == 0) {
        encodedCall = INVALID_CALL;
        return true;
    }
    encodedCall.fill(0x00);
    // Encode the characters to base-40 digits.
    uint64_t encoded = 0;

    for (auto it = callsign.rbegin(); it != callsign.rend(); ++it) {
        encoded *= 40;
        if (*it >= 'A' and *it <= 'Z') {
            encoded += (*it - 'A') + 1;
        } else if (*it >= '0' and *it <= '9') {
            encoded += (*it - '0') + 27;
        } else if (*it == '-') {
            encoded += 37;
        } else if (*it == '/') {
            encoded += 38;
        } else if (*it == '.') {
            encoded += 39;
        } else if (strict) {
            return false;
        }
    }

    auto *ptr = reinterpret_cast<uint8_t *>(&encoded);
    std::copy(ptr, ptr + 6, encodedCall.rbegin());

    return true;
}

const char *Callsign::decode_callsign(const call_t &encodedCall,
                                      char *out) const
{
    // First of all, check if encoded address is a broadcast one
    bool isBroadcast = true;
    for (auto &elem : encodedCall) {
        if (elem != 0xFF) {
            isBroadcast = false;
            break;
        }
    }

    if (isBroadcast) {
        std::strncpy(out, BROADCAST_CALL_STR, CALLSIGN_MAX_CHARS);
        out[CALLSIGN_MAX_CHARS] = '\0';
        return out;
    }

    // Then, check if encoded address is the invalid one
    bool isInvalid = true;
    for (auto &elem : encodedCall) {
        if (elem != 0x00) {
            isInvalid = false;
            break;
        }
    }

    if (isInvalid) {
        std::strncpy(out, INVALID_CALL_STR, CALLSIGN_MAX_CHARS);
        out[CALLSIGN_MAX_CHARS] = '\0';
        return out;
    }

    /*
     * Address is not broadcast, decode it.
     * TODO: current implementation works only on little endian architectures.
     */
    static const char charMap[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/.";

    uint64_t encoded = 0;
    auto p = reinterpret_cast<uint8_t *>(&encoded);
    std::copy(encodedCall.rbegin(), encodedCall.rend(), p);

    // Decode each base-40 digit and map them to the appriate character.
    size_t pos = 0;
    while (encoded && pos < CALLSIGN_MAX_CHARS) {
        out[pos++] = charMap[encoded % 40];
        encoded /= 40;
    }
    out[pos] = '\0';
    return out;
}

Callsign::operator std::string() const
{
    return std::string(this->call);
}

Callsign::operator const char *() const
{
    return this->call;
}

Callsign::operator call_t() const
{
    call_t encodedCall;
    encode_callsign(std::string(this->call), encodedCall);
    return encodedCall;
}

/**
 * NOTE! since only incomingCs is checked for special values, the second arg must be the incoming station
 */
bool compareCallsigns(const char *localCs, const char *incomingCs)
{
    // treat null pointers as empty strings
    if (!localCs)
        localCs = "";
    if (!incomingCs)
        incomingCs = "";

    if (std::strcmp(incomingCs, BROADCAST_CALL_STR) == 0
        || std::strcmp(incomingCs, "INFO") == 0
        || std::strcmp(incomingCs, "ECHO") == 0)
        return true;

    // find slash and possibly truncate if slash is within first 3 chars
    const char *truncatedLocal = localCs;
    const char *truncatedIncoming = incomingCs;

    const char *slash = std::strchr(localCs, '/');
    if (slash && (slash - localCs) <= 2)
        truncatedLocal = slash + 1;

    slash = std::strchr(incomingCs, '/');
    if (slash && (slash - incomingCs) <= 2)
        truncatedIncoming = slash + 1;

    return std::strcmp(truncatedLocal, truncatedIncoming) == 0;
}

// NOTE! Since this uses compareCallsigns internally, always have the right side of the equality check be the incoming callsign
bool Callsign::operator==(const Callsign &other) const
{
    return compareCallsigns(this->call, other.call);
}
