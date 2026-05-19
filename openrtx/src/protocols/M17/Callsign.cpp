/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstring>
#include "protocols/M17/Callsign.hpp"

using namespace M17;

static const char BROADCAST_CALL[] = "ALL";
static const char INVALID_CALL[] = "INVALID";
static const char charMap[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/.";

Callsign::Callsign() : Callsign(INVALID_CALL)
{
}

Callsign::Callsign(const std::string callsign) : Callsign(callsign.c_str())
{
}

Callsign::Callsign(const char *callsign)
{
    std::memset(call, 0, sizeof(call));
    std::strncpy(call, callsign, MAX_CALLSIGN_CHARS);
}

Callsign::Callsign(const call_t &encodedCall)
{
    bool isBroadcast = true;
    bool isInvalid = true;

    for (auto &elem : encodedCall) {
        if (elem != 0xFF)
            isBroadcast = false;

        if (elem != 0x00)
            isInvalid = false;
    }

    std::memset(call, 0, sizeof(call));

    if (isBroadcast) {
        std::strncpy(call, BROADCAST_CALL, sizeof(call));
        return;
    }

    if (isInvalid) {
        std::strncpy(call, INVALID_CALL, sizeof(call));
        return;
    }

    // Convert to little endian format
    uint64_t encoded = 0;
    auto p = reinterpret_cast<uint8_t *>(&encoded);
    std::copy(encodedCall.rbegin(), encodedCall.rend(), p);

    size_t pos = 0;
    while (encoded != 0 && pos < MAX_CALLSIGN_CHARS) {
        call[pos++] = charMap[encoded % 40];
        encoded /= 40;
    }
}

bool Callsign::isSpecial() const
{
    if ((std::strcmp(call, "INFO") == 0) || (std::strcmp(call, "ECHO") == 0)
        || (std::strcmp(call, "ALL") == 0))
        return true;

    return false;
}

Callsign::operator std::string() const
{
    return std::string(call);
}

Callsign::operator const char *() const
{
    return call;
}

Callsign::operator call_t() const
{
    call_t encoded;
    uint64_t tmp = 0;

    if (strcmp(call, BROADCAST_CALL) == 0) {
        encoded.fill(0xFF);
        return encoded;
    }

    if (strcmp(call, INVALID_CALL) == 0) {
        encoded.fill(0x00);
        return encoded;
    }

    for (int i = strlen(call) - 1; i >= 0; i--) {
        tmp *= 40;

        if (call[i] >= 'A' && call[i] <= 'Z') {
            tmp += (call[i] - 'A') + 1;
        } else if (call[i] >= '0' && call[i] <= '9') {
            tmp += (call[i] - '0') + 27;
        } else if (call[i] == '-') {
            tmp += 37;
        } else if (call[i] == '/') {
            tmp += 38;
        } else if (call[i] == '.') {
            tmp += 39;
        }
    }

    // Return encoded callsign in big endian format
    auto *ptr = reinterpret_cast<uint8_t *>(&tmp);
    std::copy(ptr, ptr + 6, encoded.rbegin());

    return encoded;
}

bool Callsign::operator==(const Callsign &other) const
{
    // find slash and possibly truncate if slash is within first 3 chars
    const char *truncatedLocal = call;
    const char *truncatedIncoming = other.call;

    const char *slash = std::strchr(call, '/');
    if (slash && (slash - call) <= 2)
        truncatedLocal = slash + 1;

    slash = std::strchr(other.call, '/');
    if (slash && (slash - other.call) <= 2)
        truncatedIncoming = slash + 1;

    return std::strcmp(truncatedLocal, truncatedIncoming) == 0;
}
