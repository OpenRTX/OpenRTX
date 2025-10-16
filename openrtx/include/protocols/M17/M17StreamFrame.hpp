/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_STREAM_FRAME_H
#define M17_STREAM_FRAME_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstring>
#include <string>
#include "M17Datatypes.hpp"

namespace M17
{

class M17FrameDecoder;

/**
 * This class describes and handles a generic M17 data frame.
 */
class M17StreamFrame
{
public:

    /**
     * Constructor.
     */
    M17StreamFrame()
    {
        clear();
    }

    /**
     * Destructor.
     */
    ~M17StreamFrame(){ }

    /**
     * Clear the frame content, filling it with zeroes.
     */
    void clear()
    {
        memset(&data, 0x00, sizeof(data));
    }

    /**
     * Set frame sequence number.
     *
     * @param seqNum: frame number, between 0 and 0x7FFF.
     */
    void setFrameNumber(const uint16_t seqNum)
    {
        // NOTE: M17 fields are big-endian, we need to swap bytes
        data.frameNum = __builtin_bswap16(seqNum & FN_MASK);
    }

    /**
     * Get frame sequence number.
     *
     * @return frame number, between 0 and 0x7FFF.
     */
    uint16_t getFrameNumber()
    {
        // NOTE: M17 fields are big-endian, we need to swap bytes
        return __builtin_bswap16(data.frameNum);
    }

    /**
     * Mark this frame as the last one in the transmission, informing the
     * receiver that transmission ends.
     */
    void lastFrame()
    {
        data.frameNum |= EOS_BIT;
    }

    /**
     * Check if this frame is the last one that is, get the value of the EOS
     * bit.
     *
     * @return true if the frame has the EOS bit set.
     */
    bool isLastFrame()
    {
        return ((data.frameNum & EOS_BIT) != 0) ? true : false;
    }

    /**
     * Access frame payload.
     *
     * @return a reference to frame's paylod field, allowing for both read and
     * write access.
     */
    payload_t& payload()
    {
        return data.payload;
    }

    /**
     * Get underlying data.
     *
     * @return a pointer to const uint8_t allowing direct access to frame data.
     */
    const uint8_t *getData()
    {
        return reinterpret_cast < const uint8_t * > (&data);
    }

private:

    struct __attribute__((packed))
    {
        uint16_t   frameNum;  // Frame number
        payload_t  payload;   // Payload data
    }
    data;
                                                   ///< Frame data.
    static constexpr uint16_t EOS_BIT = 0x0080;    ///< End Of Stream bit.
    static constexpr uint16_t FN_MASK = 0x7FFF;    ///< Bitmask for frame number.

    // Frame decoder class needs to access raw frame data
    friend class M17FrameDecoder;
};

}      // namespace M17

#endif // M17_STREAM_FRAME_H
