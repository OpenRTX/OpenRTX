/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_CONVOLUTIONAL_ENCODER_H
#define M17_CONVOLUTIONAL_ENCODER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <cstddef>

namespace M17
{

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

}      // namespace M17

#endif // M17_CONVOLUTIONAL_ENCODER_H
