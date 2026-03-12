/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_DECODER_H
#define APRS_DECODER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
//#include <cstddef>
#include <memory>
#include "core/crc.h"
#include "protocols/APRS/constants.h"
#include "protocols/APRS/frame.h"

namespace APRS
{

class Decoder
{
public:
    /**
     * @brief Constructs a Decoder object
     */
    Decoder();

    /**
     * @brief Destroys a Decoder object
     */
    ~Decoder();

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
     * @brief Performs HDLC decoding
     *
     * Performs stateful HDLC decoding: NRZI decoding, bit-unstuffing,
     * CRC checking, etc.
     *
     * @param bit A uint8_t bit from the slicer
     *
     * @return True if a frame is ready, false otherwise
     */
    bool decode(uint8_t bit);

    /**
     * @brief Gets the ready frame and sets newFrame to false
     */
    struct frameData &getFrame();

private:
    /**
     * @brief Performs NRZI decoding of a bit
     *
     * Performs non-return-to-zero, inverted decoding and stores in input bit
     * for the next comparison
     *
     * @param bit A uint8_t bit from the slicer
     *
     * @return A uint8_t NRZI decoded bit
     */
    uint8_t nrziDecode(const uint8_t bit);

    enum class Signal { FLAG = 'F', ERROR = 'E', NONE = 'N', IGNORE = 'I' };

    /**
     * @brief Counts incoming bits and sends signals for flags and errors.
     *
     * @param bit A uint8_t bit from the NRZI decoder
     *
     * @return A Signal indicating HDLC flags and errors
     */
    Signal countBits(const uint8_t bit);

    /**
     * @brief Closes out the current frame
     *
     * Pulls off the frame check sequence (CRC) from the end of the frame and
     * compares it to the calculated CRC. If it matches it swaps readyFrame
     * and demodFrame, and sets the newFrame flag.
     */
    void closeFrame();

    /**
     * @brief Adds bits to the current frame
     *
     * Adds bits to workingByte (reversing them) and if workingByte is full,
     * adds it to demodFrame.
     */
    void addToFrame(uint8_t);

    /**
     * @brief Resets the current frame and returns to WAITING state
     */
    void dumpFrame();

    uint8_t onesCount = 0;                        ///< count of one bits recv
    uint8_t prevBit = 0;                          ///< previous bit
    uint8_t workingByte = 0;                      ///< byte being built
    uint8_t bitPos = 0;                           ///< position in workingByte
    std::unique_ptr<struct frameData> demodFrame; ///< frame being built
    std::unique_ptr<struct frameData> readyFrame; ///< fully built frame
    bool newFrame;                ///< flag for new frame available
    enum class State { WAITING = 'W', INFRAME = 'I' };
    State state = State::WAITING; ///< current state of decoder
};

} /* APRS */

#endif
