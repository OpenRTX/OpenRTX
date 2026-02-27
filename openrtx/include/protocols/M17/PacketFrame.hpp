/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_PACKETFRAME_H
#define M17_PACKETFRAME_H

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
 * Element access uses operator[] (like std::array); metadata is manipulated
 * exclusively through setEof()/isEof()/setCounter()/getCounter().
 */
class PacketFrame
{
public:
    static constexpr size_t DATA_SIZE = 25;
    static constexpr size_t FRAME_SIZE = DATA_SIZE + 1; // 25 data + 1 metadata

    /**
     * Constructor.
     */
    PacketFrame()
    {
        clear();
    }

    /**
     * Clear the frame content, filling it with zeroes.
     */
    void clear()
    {
        frameData.payloadData.fill(0);
        frameData.metadata = 0;
    }

    /**
     * Access a payload byte by index.
     *
     * @param index: byte position in the 25-byte data portion.
     * @return a reference to the byte at the given index.
     */
    uint8_t &operator[](size_t index)
    {
        return frameData.payloadData[index];
    }

    /**
     * Access a payload byte by index (const overload).
     *
     * @param index: byte position in the 25-byte data portion.
     * @return the byte value at the given index.
     */
    uint8_t operator[](size_t index) const
    {
        return frameData.payloadData[index];
    }

    /**
     * Set the EOF (last frame) flag.
     *
     * @param eof: true to mark this as the last packet frame.
     */
    void setEof(bool eof)
    {
        if (eof)
            frameData.metadata |= EOF_BIT;
        else
            frameData.metadata &= ~EOF_BIT;
    }

    /**
     * Check if this is the last packet frame.
     *
     * @return true if the EOF bit is set.
     */
    bool isEof() const
    {
        return (frameData.metadata & EOF_BIT) != 0;
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
        frameData.metadata = (frameData.metadata & COUNTER_CLEAR)
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
        return (frameData.metadata >> COUNTER_SHIFT) & COUNTER_MASK;
    }

    /**
     * Get underlying data (all 26 bytes: payload + metadata).
     *
     * @return a pointer to const uint8_t allowing direct access to frame data.
     */
    const uint8_t *data() const
    {
        return reinterpret_cast<const uint8_t *>(&frameData);
    }

private:
    static constexpr uint8_t EOF_BIT = 0x80;
    static constexpr uint8_t COUNTER_MASK = 0x1F;
    static constexpr uint8_t COUNTER_SHIFT = 2;
    static constexpr uint8_t COUNTER_CLEAR = ~(COUNTER_MASK << COUNTER_SHIFT);

    struct __attribute__((packed)) {
        std::array<uint8_t, DATA_SIZE> payloadData;
        uint8_t metadata;
    } frameData;

    friend class M17FrameDecoder;
};

} // namespace M17

#endif // M17_PACKETFRAME_H
