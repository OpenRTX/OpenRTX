/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/PacketAssembler.hpp"
#include "core/crc.h"
#include <cstring>

namespace M17
{

PacketAssembler::PacketAssembler()
    : src(nullptr), totalLen(0), offset(0), counter(0), numFrames(0)
{
}

bool PacketAssembler::init(uint8_t *buffer, size_t dataLen)
{
    if (buffer == nullptr || dataLen == 0)
        return false;

    totalLen = dataLen + 2;
    if (totalLen > MAX_PACKET_DATA)
        return false;

    src = buffer;

    // Compute CRC over application data, append big-endian.
    uint16_t crc = crc_m17(buffer, dataLen);
    buffer[dataLen] = static_cast<uint8_t>(crc >> 8);
    buffer[dataLen + 1] = static_cast<uint8_t>(crc & 0xFF);

    numFrames = (totalLen + PacketFrame::DATA_SIZE - 1)
              / PacketFrame::DATA_SIZE;
    offset = 0;
    counter = 0;

    return true;
}

bool PacketAssembler::nextFrame(PacketFrame &frame)
{
    if (offset >= totalLen)
        return false;

    size_t remaining = totalLen - offset;
    frame.clear();

    if (remaining > PacketFrame::DATA_SIZE) {
        memcpy(frame.data(), &src[offset], PacketFrame::DATA_SIZE);
        frame.setCounter(counter);
        offset += PacketFrame::DATA_SIZE;
        counter++;
    } else {
        memcpy(frame.data(), &src[offset], remaining);
        frame.setEof(true);
        frame.setCounter(static_cast<uint8_t>(remaining));
        offset = totalLen;
    }

    return true;
}

} // namespace M17
