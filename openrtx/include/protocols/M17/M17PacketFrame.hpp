/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_PACKET_FRAME_H
#define M17_PACKET_FRAME_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "M17Datatypes.hpp"
#include <cstring>

namespace M17
{

/**
 * This class describes and handles a generic M17 packet frame.
 *
 * The frame is 26 bytes on the wire: 25 bytes of packet data followed by a
 * 1-byte metadata field (EOF flag, 5-bit counter, 2 reserved bits).
 * The payload() accessor exposes only the 25-byte data portion; metadata
 * is manipulated exclusively through setEof()/isEof()/setCounter()/getCounter().
 */
class M17PacketFrame
{
    static constexpr uint8_t EOF_BIT = 0x80;
    static constexpr uint8_t COUNTER_MASK = 0x1F;
    static constexpr uint8_t COUNTER_SHIFT = 2;
    static constexpr uint8_t COUNTER_CLEAR =
        static_cast<uint8_t>(~(COUNTER_MASK << COUNTER_SHIFT));

public:
    static constexpr size_t DATA_SIZE = 25;

    /**
     * Constructor.
     */
    M17PacketFrame()
    {
        clear();
    }

    /**
     * Clear the frame content, filling it with zeroes.
     */
    void clear()
    {
        data.payloadData.fill(0);
        data.metadata = 0;
    }

    /**
     * Access frame payload (data portion only, excluding metadata byte).
     *
     * @return a reference to frame's 25-byte data field, allowing for both
     * read and write access.
     */
    std::array<uint8_t, DATA_SIZE> &payload()
    {
        return data.payloadData;
    }

    /**
     * Access frame payload (const overload).
     *
     * @return a const reference to frame's 25-byte data field.
     */
    const std::array<uint8_t, DATA_SIZE> &payload() const
    {
        return data.payloadData;
    }

    /**
     * Set the EOF (last frame) flag.
     *
     * @param eof: true to mark this as the last packet frame.
     */
    void setEof(bool eof)
    {
        if (eof)
            data.metadata |= EOF_BIT;
        else
            data.metadata &= ~EOF_BIT;
    }

    /**
     * Check if this is the last packet frame.
     *
     * @return true if the EOF bit is set.
     */
    bool isEof() const
    {
        return (data.metadata & EOF_BIT) != 0;
    }

    /**
     * Set the 5-bit counter field (bits 6..2 of byte 25).
     *
     * For intermediate frames (EOF=0) this is the frame sequence number.
     * For the final frame (EOF=1) this is the number of valid data bytes
     * in the last 25-byte chunk (1-25).
     *
     * @param counter: value between 0 and 31.
     */
    void setCounter(uint8_t counter)
    {
        data.metadata = (data.metadata & COUNTER_CLEAR)
                      | ((counter & COUNTER_MASK) << COUNTER_SHIFT);
    }

    /**
     * Get the 5-bit counter field.
     *
     * @return counter value between 0 and 31.  See setCounter() for
     * interpretation based on the EOF flag.
     */
    uint8_t getCounter() const
    {
        return (data.metadata >> COUNTER_SHIFT) & COUNTER_MASK;
    }

    /**
     * Get underlying data (all 26 bytes: payload + metadata).
     *
     * @return a pointer to const uint8_t allowing direct access to frame data.
     */
    const uint8_t *getData() const
    {
        return reinterpret_cast<const uint8_t *>(&data);
    }

private:
    struct __attribute__((packed)) {
        std::array<uint8_t, DATA_SIZE> payloadData;
        uint8_t metadata;
    } data;

    friend class M17FrameDecoder;
};

} // namespace M17

#endif // M17_PACKET_FRAME_H
