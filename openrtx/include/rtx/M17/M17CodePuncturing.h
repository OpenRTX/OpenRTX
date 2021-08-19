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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef M17CODEPUNCTURING_H
#define M17CODEPUNCTURING_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <experimental/array>
#include "Utils.h"


/**
 * Puncture matrix for linx setup frame.
 */
static constexpr auto LSF_puncture = std::experimental::make_array< uint8_t >
(
    1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1
);


/**
 *  Puncture matrix for audio frames.
 */
static constexpr auto Audio_puncture = std::experimental::make_array< uint8_t >
(
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 0
);


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


#endif /* M17CODEPUNCTURING_H */
