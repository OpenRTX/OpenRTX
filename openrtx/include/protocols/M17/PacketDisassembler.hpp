/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_PACKETDISASSEMBLER_H
#define M17_PACKETDISASSEMBLER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "PacketFrame.hpp"
#include "PacketAssembler.hpp"
#include <cstddef>
#include <cstdint>

namespace M17
{

/**
 * Result codes returned by PacketDisassembler::pushFrame().
 */
enum class DisassemblerResult {
    IN_PROGRESS,  ///< Frame accepted, waiting for more
    COMPLETE,     ///< Final frame accepted, CRC valid — packet ready
    ERR_CRC,      ///< Final frame received but CRC check failed
    ERR_OVERFLOW, ///< Frame would exceed buffer capacity
    ERR_SEQUENCE  ///< Frame counter does not match expected sequence
};

/**
 * Stateful reassembler for M17 Data Link Layer packet mode.
 *
 * Receives PacketFrames one at a time (as they arrive from the decoder),
 * validates sequential counters, accumulates payload data, and verifies
 * the trailing CRC on the final frame.  Protocol-ID-agnostic — works for
 * any M17 packet type.
 */
class PacketDisassembler
{
public:
    PacketDisassembler();

    /**
     * Reset the disassembler to accept a new packet from frame 0.
     */
    void reset();

    /**
     * Feed the next received PacketFrame into the reassembly buffer.
     *
     * @param frame: the PacketFrame from the decoder.
     * @return result code indicating reassembly status.
     */
    DisassemblerResult pushFrame(const PacketFrame &frame);

    /**
     * Get a pointer to the reassembled packet data (valid after COMPLETE).
     * The returned data does NOT include the trailing 2-byte CRC.
     *
     * @return pointer to the internal buffer.
     */
    const uint8_t *data() const
    {
        return buffer;
    }

    /**
     * Get the length of the reassembled packet data (excluding CRC).
     * Valid after pushFrame() returns COMPLETE.
     *
     * @return number of application-data bytes.
     */
    size_t length() const
    {
        return dataLen;
    }

private:
    uint8_t buffer[MAX_PACKET_DATA];
    size_t totalLen; ///< Bytes accumulated so far (including CRC area)
    size_t dataLen;  ///< Application data length (totalLen - 2) after CRC check
    uint8_t nextCounter; ///< Expected frame sequence number
};

} // namespace M17

#endif /* M17_PACKETDISASSEMBLER_H */
