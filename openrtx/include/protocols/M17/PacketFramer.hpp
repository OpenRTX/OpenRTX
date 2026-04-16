/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_PACKETFRAMER_H
#define M17_PACKETFRAMER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "PacketFrame.hpp"
#include <cstddef>
#include <cstdint>

namespace M17
{

/**
 * Maximum number of bytes in an M17 Single Packet (application data + CRC).
 * Per the spec, up to 823 bytes of application data plus 2 bytes of CRC,
 * yielding at most 33 frames of 25 bytes each.
 */
static constexpr size_t MAX_PACKET_DATA = 33 * 25;

/**
 * Stateful framer for M17 Data Link Layer packet mode.
 *
 * Accepts a mutable buffer of application data, computes and appends the
 * trailing CRC, then yields one PacketFrame at a time via nextFrame().
 * The buffer must have room for at least dataLen + 2 bytes (CRC).
 */
class PacketFramer
{
public:
    PacketFramer();

    /**
     * Prepare the framer for a new packet.
     *
     * Computes the M17 CRC over the first dataLen bytes and stores it
     * internally.  The CRC is appended to the frame stream during
     * nextFrame() so the caller's buffer does not need extra room.
     *
     * @param buffer:  Buffer containing application data (at least dataLen
     *                 bytes).
     * @param dataLen: Number of application-data bytes (max 823).
     * @return true if init succeeded, false on invalid input.
     */
    bool init(uint8_t *buffer, size_t len);

    /**
     * Emit the next PacketFrame.
     *
     * @param frame: output PacketFrame to fill.
     * @return true if a frame was produced, false if all frames have been
     *         emitted (call init() again to start a new packet).
     */
    bool nextFrame(PacketFrame &frame);

private:
    uint8_t *src;
    size_t dataLen;
    size_t totalLen;
    size_t offset;
    uint8_t counter;
    size_t numFrames;
    uint16_t crc;
};

} // namespace M17

#endif /* M17_PACKETFRAMER_H */
