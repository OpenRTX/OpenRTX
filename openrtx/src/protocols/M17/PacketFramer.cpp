/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/PacketFramer.hpp"
#include "core/crc.h"
#include <cstring>

namespace M17
{

PacketFramer::PacketFramer()
    : src(nullptr), totalLen(0), offset(0), counter(0), numFrames(0)
{
}

bool PacketFramer::init(uint8_t *buffer, size_t len)
{
    if (buffer == nullptr || len == 0)
        return false;

    dataLen = len;
    totalLen = len + 2;
    if (totalLen > MAX_PACKET_DATA)
        return false;

    src = buffer;

    // Compute CRC over application data and store internally.
    crc = crc_m17(buffer, len);

    numFrames = (totalLen + PacketFrame::DATA_SIZE - 1)
              / PacketFrame::DATA_SIZE;
    offset = 0;
    counter = 0;

    return true;
}

bool PacketFramer::nextFrame(PacketFrame &frame)
{
    if (offset >= totalLen)
        return false;

    size_t remaining = totalLen - offset;
    frame.clear();

    size_t chunkLen;
    bool isLast;

    if (remaining > PacketFrame::DATA_SIZE) {
        chunkLen = PacketFrame::DATA_SIZE;
        isLast = false;
    } else {
        chunkLen = remaining;
        isLast = true;
    }

    // Copy application-data bytes that fall within this frame
    size_t dataBytes = 0;
    if (offset < dataLen) {
        dataBytes = dataLen - offset;
        if (dataBytes > chunkLen)
            dataBytes = chunkLen;
        memcpy(frame.data(), &src[offset], dataBytes);
    }

    // Append CRC bytes that fall within this frame
    for (size_t i = dataBytes; i < chunkLen; i++) {
        size_t pos = offset + i;
        if (pos == dataLen)
            frame.data()[i] = static_cast<uint8_t>(crc >> 8);
        else
            frame.data()[i] = static_cast<uint8_t>(crc & 0xFF);
    }

    if (isLast) {
        frame.setEof(true);
        frame.setCounter(static_cast<uint8_t>(chunkLen));
        offset = totalLen;
    } else {
        frame.setCounter(counter);
        offset += PacketFrame::DATA_SIZE;
        counter++;
    }

    return true;
}

} // namespace M17
