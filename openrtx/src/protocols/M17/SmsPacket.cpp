/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/SmsPacket.hpp"
#include "protocols/M17/PacketFrame.hpp"
#include <cstring>

namespace M17
{

size_t sms_format_packet(const char *message, size_t msgLen, uint8_t *buffer,
                         size_t bufferSize)
{
    if (msgLen == 0 || message == nullptr || buffer == nullptr)
        return 0;

    /* SMS application format: [0x05][UTF-8 text][0x00] */
    const size_t appDataLen = 1 + msgLen + 1;

    /* Buffer needs room for app data; PacketFramer will append the 2-byte
     * CRC during framing, so the total on-air payload is appDataLen + 2
     * bytes. */
    if (appDataLen > bufferSize || appDataLen + 2 > M17_MAX_PACKET_DATA)
        return 0;

    buffer[0] = 0x05;
    memcpy(&buffer[1], message, msgLen);
    buffer[1 + msgLen] = 0x00;

    return appDataLen;
}

const char *sms_packet_text(const uint8_t *data, size_t len, size_t *textLen)
{
    if (data == nullptr || len < 2 || textLen == nullptr)
        return nullptr;

    /* Check M17 SMS protocol ID byte */
    if (data[0] != 0x05)
        return nullptr;

    /* Message text follows the protocol ID byte */
    const char *text = reinterpret_cast<const char *>(&data[1]);
    size_t length = len - 1;

    /* Strip trailing NUL if present */
    if (length > 0 && text[length - 1] == '\0')
        length--;

    *textLen = length;
    return text;
}

} // namespace M17
