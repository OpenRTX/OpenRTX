/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
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

#ifndef M17_UTILS_H
#define M17_UTILS_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstddef>
#include <cstdint>
#include <array>
#include <assert.h>


/**
 * Utility function allowing to retrieve the value of a single bit from an array
 * of bytes. Bits are counted scanning from left to right, thus bit number zero
 * is the leftmost bit of array[0].
 *
 * \param array: byte array.
 * \param pos: bit position inside the array.
 * \return value of the indexed bit, as boolean variable.
 */
template < size_t N >
inline bool getBit(const std::array< uint8_t, N >& array, const size_t pos)
{
    size_t i = pos / 8;
    size_t j = pos % 8;
    return (array[i] >> (7 - j)) & 0x01;
}


/**
 * Utility function allowing to set the value of a single bit from an array
 * of bytes. Bits are counted scanning from left to right, thus bit number zero
 * is the leftmost bit of array[0].
 *
 * \param array: byte array.
 * \param pos: bit position inside the array.
 * \param bit: bit value to be set.
 */
template < size_t N >
inline void setBit(std::array< uint8_t, N >& array, const size_t pos,
                   const bool bit)
{
    size_t i     = pos / 8;
    size_t j     = pos % 8;
    uint8_t mask = 1 << (7 - j);
    array[i] = (array[i] & ~mask) |
               (bit ? mask : 0x00);
}

/**
 * Utility function allowing to set the value of a symbol on an array
 * of bytes. Symbols are packed putting the most significant bit first,
 * symbols are filled from the least significant bit pair to the most
 * significant bit pair.
 *
 * \param array: byte array.
 * \param pos: symbol position inside the array.
 * \param symbol: symbol to be set, either -3, -1, +1, +3.
 */
template < size_t N >
inline void setSymbol(std::array< uint8_t, N >& array, const size_t pos,
                   const int8_t symbol)
{
    switch(symbol) {
        case +3:
            setBit<N> (array, 2 * pos    , 0);
            setBit<N> (array, 2 * pos + 1, 1);
            break;
        case +1:
            setBit<N> (array, 2 * pos    , 0);
            setBit<N> (array, 2 * pos + 1, 0);
            break;
        case -1:
            setBit<N> (array, 2 * pos    , 1);
            setBit<N> (array, 2 * pos + 1, 0);
            break;
        case -3:
            setBit<N> (array, 2 * pos    , 1);
            setBit<N> (array, 2 * pos + 1, 1);
            break;
        default:
            assert("Error: unknown M17 symbol!");
    }
}

#endif /* M17_UTILS_H */
