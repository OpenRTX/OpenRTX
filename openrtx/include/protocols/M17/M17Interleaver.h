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

#ifndef M17_INTERLEAVER_H
#define M17_INTERLEAVER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "M17Utils.h"

/**
 * Interleave a block of data using the quadratic permutation polynomial from
 * M17 protocol specification. Polynomial used is P(x) = 45*x + 92*x^2.
 *
 * \param data: input byte array.
 */
template < size_t N >
void interleave(std::array< uint8_t, N >& data)
{
    std::array< uint8_t, N > interleaved;

    static constexpr size_t F1 = 45;
    static constexpr size_t F2 = 92;
    static constexpr size_t NB = N*8;

    for(size_t i = 0; i < NB; i++)
    {
        size_t index = ((F1 * i) + (F2 * i * i)) % NB;
        setBit(interleaved, index, getBit(data, i));
    }

    std::copy(interleaved.begin(), interleaved.end(), data.begin());
}

/**
 * Perform the deinterleaving operation on a block of data previously interleaved
 * using the quadratic permutation polynomial from M17 protocol specification.
 * Polynomial used is P(x) = 45*x + 92*x^2.
 *
 * \param data: input byte array.
 */
template < size_t N >
void deinterleave(std::array< uint8_t, N >& data)
{
    std::array< uint8_t, N > deinterleaved;

    static constexpr size_t F1 = 45;
    static constexpr size_t F2 = 92;
    static constexpr size_t NB = N*8;

    for(size_t i = 0; i < NB; i++)
    {
        size_t index = ((F1 * i) + (F2 * i * i)) % NB;
        setBit(deinterleaved, i, getBit(data, index));
    }

    std::copy(deinterleaved.begin(), deinterleaved.end(), data.begin());
}

#endif /* M17_INTERLEAVER_H */
