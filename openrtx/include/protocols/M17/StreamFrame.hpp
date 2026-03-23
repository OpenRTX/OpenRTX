/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STREAM_FRAME_H
#define STREAM_FRAME_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstring>
#include <string>
#include "Datatypes.hpp"

namespace M17
{

class FrameDecoder;

/**
 * This class describes and handles a generic M17 data frame.
 */
class StreamFrame
{
public:

    /**
     * Constructor.
     */
    StreamFrame()
    {
        clear();
    }

    /**
     * Destructor.
     */
    ~StreamFrame(){ }

    /**
     * Clear the frame content, filling it with zeroes.
     */
    void clear()
    {
        memset(&frameData, 0x00, sizeof(frameData));
    }

    /**
     * Set frame sequence number.
     *
     * @param seqNum: frame number, between 0 and 0x7FFF.
     */
    void setFrameNumber(const uint16_t seqNum)
    {
        // NOTE: M17 fields are big-endian, we need to swap bytes
        frameData.frameNum = __builtin_bswap16(seqNum & FN_MASK);
    }

    /**
     * Get frame sequence number.
     *
     * @return frame number, between 0 and 0x7FFF.
     */
    uint16_t getFrameNumber()
    {
        // NOTE: M17 fields are big-endian, we need to swap bytes
        return __builtin_bswap16(frameData.frameNum);
    }

    /**
     * Mark this frame as the last one in the transmission, informing the
     * receiver that transmission ends.
     */
    void lastFrame()
    {
        frameData.frameNum |= EOS_BIT;
    }

    /**
     * Check if this frame is the last one that is, get the value of the EOS
     * bit.
     *
     * @return true if the frame has the EOS bit set.
     */
    bool isLastFrame()
    {
        return ((frameData.frameNum & EOS_BIT) != 0) ? true : false;
    }

    /**
     * Get underlying payload storage.
     *
     * @return a pointer to uint8_t allowing direct access to payload.
     */
    uint8_t *data()
    {
        return frameData.payload.data();
    }

    /**
     * Get underlying payload storage (const overload)
     *
     * @return a pointer to uint8_t allowing direct access to payload.
     */
    const uint8_t *data() const
    {
        return frameData.payload.data();
    }

    /**
     * Access payload data by index.
     *
     * @param index: byte position.
     * @return a reference to the byte at the given index.
     */
    uint8_t &operator[](size_t index)
    {
        return frameData.payload[index];
    }

    /**
     * Access payload data by index. (const overload).
     *
     * @param index: byte position.
     * @return the byte value at the given index.
     */
    uint8_t operator[](size_t index) const
    {
        return frameData.payload[index];
    }

private:

    struct __attribute__((packed)) {
        uint16_t   frameNum;  // Frame number
        payload_t  payload;   // Payload data
    } frameData;
                                                   ///< Frame data.
    static constexpr uint16_t EOS_BIT = 0x0080;    ///< End Of Stream bit.
    static constexpr uint16_t FN_MASK = 0x7FFF;    ///< Bitmask for frame number.

    // Frame encoder and decoder classes needs to access raw frame data
    friend class FrameEncoder;
    friend class FrameDecoder;
};

}      // namespace M17

#endif // STREAM_FRAME_H
