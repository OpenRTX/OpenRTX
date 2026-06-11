/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/PacketDeframer.hpp"
#include "core/crc.h"
#include <cstring>

namespace M17
{

PacketDeframer::PacketDeframer() :
    buffer(nullptr), capacity(0), totalLen(0), dataLen(0), nextCounter(0)
{
}

bool PacketDeframer::init(uint8_t *buf, size_t cap)
{
    buffer = nullptr;
    capacity = 0;
    totalLen = 0;
    dataLen = 0;
    nextCounter = 0;

    if (buf == nullptr || cap == 0)
        return false;

    buffer = buf;
    capacity = cap;
    return true;
}

void PacketDeframer::reset()
{
    // Preserve bound buffer/capacity; only reset reassembly state.
    totalLen = 0;
    dataLen = 0;
    nextCounter = 0;
}

DeframerResult PacketDeframer::pushFrame(const PacketFrame &frame)
{
    if (buffer == nullptr)
        return DeframerResult::ERR_OVERFLOW;

    uint8_t counter = frame.getCounter();
    bool eof = frame.isEof();

    if (!eof) {
        // Intermediate frame: counter must match expected sequence
        if (counter != nextCounter)
            return DeframerResult::ERR_SEQUENCE;

        if (totalLen + PacketFrame::DATA_SIZE > capacity)
            return DeframerResult::ERR_OVERFLOW;

        memcpy(&buffer[totalLen], frame.data(), PacketFrame::DATA_SIZE);
        totalLen += PacketFrame::DATA_SIZE;
        nextCounter++;

        return DeframerResult::IN_PROGRESS;
    }

    // EOF frame: counter holds the number of valid bytes in this chunk.
    // A counter of 0 or greater than DATA_SIZE is malformed.
    if (counter == 0 || counter > PacketFrame::DATA_SIZE)
        return DeframerResult::ERR_INVALID_FRAME;

    if (totalLen + counter > capacity)
        return DeframerResult::ERR_OVERFLOW;

    memcpy(&buffer[totalLen], frame.data(), counter);
    totalLen += counter;

    // Need at least 3 bytes: 1 byte of data + 2 bytes CRC
    if (totalLen < 3)
        return DeframerResult::ERR_CRC;

    // CRC is over all bytes except the trailing 2-byte CRC itself,
    // stored big-endian at the end of the packet.
    uint16_t computed = crc_m17(buffer, totalLen - 2);
    uint16_t stored = (buffer[totalLen - 2] << 8) | buffer[totalLen - 1];

    if (computed != stored)
        return DeframerResult::ERR_CRC;

    dataLen = totalLen - 2;
    return DeframerResult::COMPLETE;
}

} // namespace M17
