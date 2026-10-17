/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INTERLEAVER_H
#define INTERLEAVER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "Utils.hpp"

namespace M17
{

/**
 * Compute the position a bit takes after interleaving, using the quadratic
 * permutation polynomial from M17 protocol specification. Polynomial used is
 * P(x) = 45*x + 92*x^2.
 *
 * \param i: bit position before interleaving.
 * \param numBits: size of the block, in bits.
 * \return bit position after interleaving.
 */
inline size_t interleavedIndex(const size_t i, const size_t numBits)
{
    return ((45 * i) + (92 * i * i)) % numBits;
}

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

    static constexpr size_t NB = N*8;

    for(size_t i = 0; i < NB; i++)
    {
        setBit(interleaved, interleavedIndex(i, NB), getBit(data, i));
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

    static constexpr size_t NB = N*8;

    for(size_t i = 0; i < NB; i++)
    {
        setBit(deinterleaved, i, getBit(data, interleavedIndex(i, NB)));
    }

    std::copy(deinterleaved.begin(), deinterleaved.end(), data.begin());
}

}      // namespace M17

#endif // INTERLEAVER_H
