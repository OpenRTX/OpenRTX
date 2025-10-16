/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_CODE_PUNCTURING_H
#define M17_CODE_PUNCTURING_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <array>
#include "M17Utils.hpp"

namespace M17
{

/**
 * Puncture matrix for linx setup frame.
 */
static constexpr std::array< uint8_t, 61 > LSF_PUNCTURE =
{
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1,
    0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1,
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1
};

/**
 *  Puncture matrix for audio frames.
 */
static constexpr std::array< uint8_t, 12 > DATA_PUNCTURE =
{
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 0
};


/**
 * Apply a given puncturing scheme to a byte array.
 *
 * \param input: input byte array.
 * \param output: output byte array, containing puntured data.
 * \param puncture: puncturing matrix, stored as an array of 8 bit values.
 * \return resulting bit count after punturing.
 */
template < size_t IN, size_t OUT, size_t P >
size_t puncture(const std::array< uint8_t, IN  >& input,
                      std::array< uint8_t, OUT >& output,
                const std::array< uint8_t, P   >& puncture)
{
    size_t outIndex   = 0;
    size_t punctIndex = 0;
    size_t bit_count  = 0;

    for(size_t i = 0; i < 8*IN && outIndex < 8*OUT; i++)
    {
        if(puncture[punctIndex++])
        {
            setBit(output, outIndex++, getBit(input, i));
            bit_count++;
        }

        if(punctIndex == P) punctIndex = 0;
    }

    return bit_count;
}

/**
 * Apply the inverse puncturing scheme to a byte array, thus expanding its bits.
 *
 * \param input: input byte array, containing puntured data.
 * \param output: output byte array, containing depuntured data.
 * \param puncture: puncturing matrix, stored as an array of 8 bit values.
 * \return number of zero bits added to the input sequence for depunturing.
 */
template < size_t IN, size_t OUT, size_t P >
size_t depuncture(const std::array< uint8_t, IN  >& input,
                        std::array< uint8_t, OUT >& output,
                  const std::array< uint8_t, P   >& puncture)
{
    size_t inpIndex   = 0;
    size_t punctIndex = 0;
    size_t bit_count  = 0;

    for(size_t i = 0; i < 8*OUT && inpIndex < 8*IN; i++)
    {
        if(puncture[punctIndex++])
        {
            setBit(output, i, getBit(input, inpIndex++));
        }
        else
        {
            setBit(output, i, 0);
            bit_count++;
        }

        if(punctIndex == P) punctIndex = 0;
    }

    return bit_count;
}

}      // namespace M17

#endif // M17_CODE_PUNCTURING_H
