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
 * Stateful framer for M17 Data Link Layer packet mode.
 *
 * Accepts a mutable buffer of application data, computes and appends the
 * trailing CRC, then yields one PacketFrame at a time via nextFrame().
 * The caller's buffer does not need extra room for the CRC.
 */
class PacketFramer
{
public:
    PacketFramer();

    /**
     * Prepare the framer for a new packet.
     *
     * Computes the M17 CRC over the first len bytes and stores it
     * internally.  The CRC is appended to the frame stream during
     * nextFrame() so the caller's buffer does not need extra room.
     *
     * The buffer must remain valid and unmodified until all frames have
     * been consumed by nextFrame() (i.e. until nextFrame() returns false).
     *
     * @param buffer: Buffer containing application data (at least len bytes).
     * @param len:    Number of application-data bytes (max 823).
     * @return true if init succeeded, false on invalid input.
     */
    bool init(uint8_t *buffer, size_t len);

    /**
     * Emit the next PacketFrame.
     *
     * @param frame: output PacketFrame to fill.
     * @return true if the frame just written is the EOF (last) frame;
     *         false if it is an intermediate frame and more remain.
     *         Must not be called after returning true; call init() again
     *         to start a new packet.
     */
    bool nextFrame(PacketFrame &frame);

private:
    uint8_t *src;
    size_t dataLen;
    size_t totalLen;
    size_t offset;
    uint8_t counter;
    uint16_t crc;
};

} // namespace M17

#endif /* M17_PACKETFRAMER_H */
