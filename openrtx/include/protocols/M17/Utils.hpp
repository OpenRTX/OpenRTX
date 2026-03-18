/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_UTILS_H
#define M17_UTILS_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstddef>
#include <cstdint>
#include <array>
#include <assert.h>

namespace M17
{

/**
 * Utility function allowing to retrieve the value of a single bit from an array
 * of bytes. Bits are counted scanning from left to right, thus bit number zero
 * is the leftmost bit of array[0].
 *
 * @param array: byte array.
 * @param pos: bit position inside the array.
 * @return value of the indexed bit, as boolean variable.
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
 * @param array: byte array.
 * @param pos: bit position inside the array.
 * @param bit: bit value to be set.
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
 * Compute the hamming distance between two bytes.
 *
 * @param x: first byte.
 * @param y: second byte.
 * @return hamming distance between x and y.
 */
static inline uint8_t hammingDistance(const uint8_t x, const uint8_t y)
{
    return __builtin_popcount(x ^ y);
}


/**
 * Utility function allowing to set the value of a symbol on an array
 * of bytes. Symbols are packed putting the most significant bit first,
 * symbols are filled from the least significant bit pair to the most
 * significant bit pair.
 *
 * @param array: byte array.
 * @param pos: symbol position inside the array.
 * @param symbol: symbol to be set, either -3, -1, +1, +3.
 */
template < size_t N >
inline void setSymbol(std::array< uint8_t, N >& array, const size_t pos,
                      const int8_t symbol)
{
    switch(symbol)
    {
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

}      // namespace M17

#endif // M17_UTILS_H
