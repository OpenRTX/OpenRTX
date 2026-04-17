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

PacketDeframer::PacketDeframer()
{
    reset();
}

void PacketDeframer::reset()
{
    memset(buffer, 0, sizeof(buffer));
    totalLen = 0;
    dataLen = 0;
    nextCounter = 0;
}

DeframerResult PacketDeframer::pushFrame(const PacketFrame &frame)
{
    uint8_t counter = frame.getCounter();
    bool eof = frame.isEof();

    if (!eof) {
        // Intermediate frame: counter must match expected sequence
        if (counter != nextCounter)
            return DeframerResult::ERR_SEQUENCE;

        if (totalLen + PacketFrame::DATA_SIZE > MAX_PACKET_DATA)
            return DeframerResult::ERR_OVERFLOW;

        memcpy(&buffer[totalLen], frame.data(), PacketFrame::DATA_SIZE);
        totalLen += PacketFrame::DATA_SIZE;
        nextCounter++;

        return DeframerResult::IN_PROGRESS;
    }

    // EOF frame: counter holds the number of valid bytes in this chunk
    uint8_t lastBytes = counter;
    if (lastBytes == 0 || lastBytes > PacketFrame::DATA_SIZE)
        lastBytes = PacketFrame::DATA_SIZE;

    if (totalLen + lastBytes > MAX_PACKET_DATA)
        return DeframerResult::ERR_OVERFLOW;

    memcpy(&buffer[totalLen], frame.data(), lastBytes);
    totalLen += lastBytes;

    // Need at least 3 bytes: 1 byte of data + 2 bytes CRC
    if (totalLen < 3)
        return DeframerResult::ERR_CRC;

    // CRC is over all bytes except the trailing 2-byte CRC itself,
    // stored big-endian at the end of the packet.
    uint16_t computed = crc_m17(buffer, totalLen - 2);
    uint16_t stored = (static_cast<uint16_t>(buffer[totalLen - 2]) << 8)
                    | static_cast<uint16_t>(buffer[totalLen - 1]);

    if (computed != stored)
        return DeframerResult::ERR_CRC;

    dataLen = totalLen - 2;
    return DeframerResult::COMPLETE;
}

} // namespace M17
