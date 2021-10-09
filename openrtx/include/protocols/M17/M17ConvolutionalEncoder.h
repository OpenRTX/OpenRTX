/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   Adapted from original code written by Rob Riggs, Mobilinkd LLC        *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef M17_CONVOLUTIONAL_ENCODER_H
#define M17_CONVOLUTIONAL_ENCODER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <cstddef>

/**
 * Convolutional encoder tailored on M17 protocol specifications, requiring a
 * coder rate R = 1/2, a constraint length K = 5 and polynomials G1 = 0x19 and
 * G2 = 0x17.
 */
class M17ConvolutionalEncoder
{
public:

    /**
     * Constructor.
     */
    M17ConvolutionalEncoder(){ }

    /**
     * Encode a given block of data using the M17 convolutional encoding scheme.
     * Given the coder rate of 1/2, calling code must ensure that the destination
     * buffer for the convolved data has twice the size of the original data block.
     *
     * \param data: pointer to source data block.
     * \param convolved: pointer to a destination buffer for the convolved data.
     * \param len: length of the source data block.
     */
    void encode(const void *data, void *convolved, const size_t len)
    {
        const uint8_t  *src  = reinterpret_cast< const uint8_t * >(data);
        uint16_t       *dest = reinterpret_cast< uint16_t * >(convolved);

        for(size_t i = 0; i < len; i++)
        {
            dest[i] = convolveByte(src[i]);
        }
    }

    /**
     * Flush the convolutional encoder, returning the remaining encoded data.
     *
     * \return the convolution encoding of the remaining content of the encoder
     * memory.
     */
    uint16_t flush()
    {
        return convolveByte(0x00);
    }

    /**
     * Reset the convolutional encoder memory to zero.
     */
    void reset()
    {
        memory = 0;
    }

private:

    /**
     * Compute the convolutional encoding of a byte, using the M17 encoding
     * scheme.
     *
     * \param value: byte to be convolved.
     * \return result of the convolutional encoding process.
     */
    uint16_t convolveByte(uint8_t value)
    {
        uint16_t result = 0;

        for(uint8_t i = 0; i < 8; i++)
        {
            memory  = (memory << 1) | ((value & 0x80) >> 7);
            memory &= 0x1F;
            result  = (result << 1) | (__builtin_popcount(memory & 0x19) & 0x01);
            result  = (result << 1) | (__builtin_popcount(memory & 0x17) & 0x01);
            value <<= 1;
        }

        return __builtin_bswap16(result);
    }

    uint8_t memory = 0;    ///< Convolutional encoder memory.
};

#endif /* M17_CONVOLUTIONAL_ENCODER_H */
