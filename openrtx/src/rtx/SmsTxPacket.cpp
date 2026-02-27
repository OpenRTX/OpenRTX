/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rtx/SmsTxPacket.hpp"
#include "protocols/M17/PacketAssembler.hpp"
#include <cstring>

size_t prepareSmsPacketData(const char *message, size_t msgLen, uint8_t *buffer,
                            size_t bufferSize)
{
    if (msgLen == 0 || message == nullptr || buffer == nullptr)
        return 0;

    // SMS application format: [0x05][UTF-8 text][0x00]
    const size_t appDataLen = 1 + msgLen + 1;

    // Buffer needs room for app data + 2-byte CRC (written by PacketAssembler)
    if (appDataLen + 2 > bufferSize || appDataLen + 2 > M17::MAX_PACKET_DATA)
        return 0;

    buffer[0] = 0x05;
    memcpy(&buffer[1], message, msgLen);
    buffer[1 + msgLen] = 0x00;

    return appDataLen;
}
