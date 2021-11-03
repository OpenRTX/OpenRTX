/***************************************************************************
 *   Copyright (C) 2021 by Alain Carlucci                                  *
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

#ifndef M17_LOOKUP_MODULATOR_H
#define M17_LOOKUP_MODULATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <cstddef>
#include <array>

/**
 * Modulator device for M17 protocol.
 *
 * This modulator uses lookup tables to perform 4FSK.
 */
class M17LookupModulator
{
public:

    /**
     * Constructor.
     */
    M17LookupModulator();

    /**
     * Destructor.
     */
    ~M17LookupModulator();

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
     * a given block of data.
     *
     * @param sync: synchronisation word to be prepended to data block.
     * @param data: data block to be transmitted.
     */
    void send(const std::array< uint8_t, 2 >& sync,
              const std::array< uint8_t, 46 >& data);

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
    inline std::array< uint8_t, 4 > byteToSymbols(uint8_t value)
    {
        std::array< uint8_t, 4 > symbols;

        symbols[3] = 1 + (value & 0x03);
        symbols[2] = 1 + ((value >> 2) & 0x03);
        symbols[1] = 1 + ((value >> 4) & 0x03);
        symbols[0] = 1 + ((value >> 6) & 0x03);

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

public:
    dataBuffer_t* getOutputBuffer() const {
        return idleBuffer;
    }
};

#endif /* M17_MODULATOR_H */
