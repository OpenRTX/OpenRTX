/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef M17_MODULATOR_H
#define M17_MODULATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <array>

/**
 * Modulator device for M17 protocol.
 */
class M17Modulator
{
public:

    /**
     * Constructor.
     */
    M17Modulator();

    /**
     * Destructor.
     */
    ~M17Modulator();

    /**
     * Allocate buffers for baseband audio generation and initialise modulator.
     */
    void init();

    /**
     * Shutdown modulator and deallocate data buffers.
     */
    void terminate();

    /**
     * Generate and transmit the baseband signal obtained by 4FSK modulation of
     * a given block of data. When called for the first time, this function
     * starts baseband transmission.
     *
     * @param sync: synchronisation word to be prepended to data block.
     * @param data: data block to be transmitted.
     * @param isLast: flag signalling that current block is the last one being
     * transmitted.
     */
    void send(const std::array< uint8_t, 2 >& sync,
              const std::array< uint8_t, 46 >& data,
              const bool isLast = false);

private:

    /**
     * Generate baseband stream from symbol stream.
     */
    void generateBaseband();

    /**
     * Emit the baseband stream towards the output stage, platform dependent.
     */
    void emitBaseband();

    /**
     * Utility function to encode a given byte of data into 4FSK symbols. Each
     * byte is encoded in four symbols.
     *
     * @param value: value to be encoded in 4FSK symbols.
     * @return std::array containing the four symbols obtained by 4FSK encoding.
     */
    inline std::array< int8_t, 4 > byteToSymbols(uint8_t value)
    {
        static constexpr int8_t LUT[] = { +1, +3, -1, -3};
        std::array< int8_t, 4 > symbols;

        symbols[3] = LUT[value & 0x03];
        value >>= 2;
        symbols[2] = LUT[value & 0x03];
        value >>= 2;
        symbols[1] = LUT[value & 0x03];
        value >>= 2;
        symbols[0] = LUT[value & 0x03];

        return symbols;
    }

    static constexpr size_t M17_RTX_SAMPLE_RATE = 48000;
    static constexpr size_t M17_FRAME_SAMPLES   = 1920;
    static constexpr size_t M17_FRAME_SYMBOLS   = 192;

    using dataBuffer_t = std::array< int16_t, M17_FRAME_SAMPLES >;

    std::array< int16_t, M17_FRAME_SYMBOLS > symbols;
    int16_t      *baseband_buffer;    ///< Buffer for baseband audio handling.
    dataBuffer_t *activeBuffer;       ///< Half baseband buffer, in transmission.
    dataBuffer_t *idleBuffer;         ///< Half baseband buffer, free for processing.
    bool         txRunning;           ///< Transmission running.
    bool         stopTx;              ///< Stop transmission request.
};

#endif /* M17_MODULATOR_H */
