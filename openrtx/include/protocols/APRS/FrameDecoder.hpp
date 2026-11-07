/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FRAME_DECODER_H
#define FRAME_DECODER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <memory>
#include "core/crc.h"
#include "protocols/APRS/constants.h"

namespace APRS
{

struct frame {
    uint8_t data[APRS_PACLEN];
    uint8_t len;
};

class FrameDecoder
{
public:
    /**
     * Construct a Decoder object
     */
    FrameDecoder();

    /**
     * Destroy a Decoder object
     */
    ~FrameDecoder();

    /**
     * Allocate buffers and initialize decoder.
     */
    void init();

    /**
     * Shutdown decoder and deallocate data buffers.
     */
    void terminate();

    /**
     * Reset the decoder state.
     */
    void reset();

    /**
     * Perform HDLC decoding
     *
     * Performs stateful HDLC decoding: NRZI decoding, bit-unstuffing,
     * CRC checking, etc.
     *
     * @param bit A uint8_t bit from the slicer
     * @return True if a frame is ready, false otherwise
     */
    bool decode(uint8_t bit);

    /**
     * Gets the ready frame and sets newFrame to false
     */
    struct frame &getFrame();

private:
    /**
     * Performs NRZI decoding of a bit
     *
     * Performs non-return-to-zero, inverted decoding and stores in input bit
     * for the next comparison
     *
     * @param bit A uint8_t bit from the slicer
     *
     * @return A uint8_t NRZI decoded bit
     */
    uint8_t nrziDecode(const uint8_t bit);

    /**
     * Counts incoming bits and sends signals for flags and errors.
     *
     * @param bit A uint8_t bit from the NRZI decoder
     * @return A Signal indicating HDLC flags and errors
     */
    enum class Signal;
    Signal countBits(const uint8_t bit);

    /**
     * Closes out the current frame
     *
     * Pulls off the frame check sequence (CRC) from the end of the frame and
     * compares it to the calculated CRC. If it matches it swaps readyFrame
     * and demodFrame, and sets the newFrame flag.
     */
    void closeFrame();

    /**
     * Adds bits to the current frame
     *
     * Adds bits to workingByte (reversing them) and if workingByte is full,
     * adds it to demodFrame.
     */
    void addToFrame(uint8_t);

    /**
     * Resets the current frame and returns to WAITING state
     */
    void dumpFrame();

    enum class Signal { FLAG = 'F', ERROR = 'E', NONE = 'N', IGNORE = 'I' };
    enum class State { WAITING = 'W', INFRAME = 'I' };

    uint8_t onesCount = 0;                    ///< count of one bits recv
    uint8_t prevBit = 0;                      ///< previous bit
    uint8_t workingByte = 0;                  ///< byte being built
    uint8_t bitPos = 0;                       ///< position in workingByte
    std::unique_ptr<struct frame> demodFrame; ///< frame being built
    std::unique_ptr<struct frame> readyFrame; ///< fully built frame
    bool newFrame = false;                    ///< new frame available flag
    State state = State::WAITING;             ///< current state of decoder
};

} /* APRS */

#endif
