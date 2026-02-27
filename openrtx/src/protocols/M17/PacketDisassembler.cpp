/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/PacketDisassembler.hpp"
#include "core/crc.h"
#include <cstring>

namespace M17
{

PacketDisassembler::PacketDisassembler()
{
    reset();
}

void PacketDisassembler::reset()
{
    memset(buffer, 0, sizeof(buffer));
    totalLen = 0;
    dataLen = 0;
    nextCounter = 0;
}

DisassemblerResult PacketDisassembler::pushFrame(const PacketFrame &frame)
{
    uint8_t counter = frame.getCounter();
    bool eof = frame.isEof();

    if (!eof) {
        // Intermediate frame: counter must match expected sequence
        if (counter != nextCounter)
            return DisassemblerResult::ERR_SEQUENCE;

        if (totalLen + PacketFrame::DATA_SIZE > MAX_PACKET_DATA)
            return DisassemblerResult::ERR_OVERFLOW;

        memcpy(&buffer[totalLen], frame.data(), PacketFrame::DATA_SIZE);
        totalLen += PacketFrame::DATA_SIZE;
        nextCounter++;

        return DisassemblerResult::IN_PROGRESS;
    }

    // EOF frame: counter holds the number of valid bytes in this chunk
    uint8_t lastBytes = counter;
    if (lastBytes == 0 || lastBytes > PacketFrame::DATA_SIZE)
        lastBytes = PacketFrame::DATA_SIZE;

    if (totalLen + lastBytes > MAX_PACKET_DATA)
        return DisassemblerResult::ERR_OVERFLOW;

    memcpy(&buffer[totalLen], frame.data(), lastBytes);
    totalLen += lastBytes;

    // Need at least 3 bytes: 1 byte of data + 2 bytes CRC
    if (totalLen < 3)
        return DisassemblerResult::ERR_CRC;

    // CRC is over all bytes except the trailing 2-byte CRC itself,
    // stored big-endian at the end of the packet.
    uint16_t computed = crc_m17(buffer, totalLen - 2);
    uint16_t stored = (static_cast<uint16_t>(buffer[totalLen - 2]) << 8)
                    | static_cast<uint16_t>(buffer[totalLen - 1]);

    if (computed != stored)
        return DisassemblerResult::ERR_CRC;

    dataLen = totalLen - 2;
    return DisassemblerResult::COMPLETE;
}

} // namespace M17
