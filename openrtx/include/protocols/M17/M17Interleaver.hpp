/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_INTERLEAVER_H
#define M17_INTERLEAVER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "M17Utils.hpp"

namespace M17
{

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

}      // namespace M17

#endif // M17_INTERLEAVER_H
