/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_UTILS_H
#define HORSE_UTILS_H

#include <cstddef>
#include <cstdint>
#include <array>
#include <cassert>
#include "HorseDatatypes.hpp"

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace horse
{

template <size_t N>
inline bool getBit(const std::array<uint8_t, N> &array, size_t pos)
{
    size_t i = pos / 8;
    size_t j = pos % 8;
    return (array[i] >> (7 - j)) & 0x01;
}

template <size_t N>
inline void setBit(std::array<uint8_t, N> &array, size_t pos, bool bit)
{
    size_t i = pos / 8;
    size_t j = pos % 8;
    uint8_t mask = 1 << (7 - j);
    array[i] = (array[i] & ~mask) | (bit ? mask : 0x00);
}

inline std::array<int8_t, 4> byteToSymbols(uint8_t value)
{
    static constexpr int8_t LUT[] = { +1, +3, -1, -3 };
    std::array<int8_t, 4> symbols;
    symbols[3] = LUT[value & 0x03];
    value >>= 2;
    symbols[2] = LUT[value & 0x03];
    value >>= 2;
    symbols[1] = LUT[value & 0x03];
    value >>= 2;
    symbols[0] = LUT[value & 0x03];
    return symbols;
}

inline std::array<int8_t, 8> syncwordSymbols(syncw_t word)
{
    auto a = byteToSymbols(word[0]);
    auto b = byteToSymbols(word[1]);
    return { a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3] };
}

/*
 * Pack one 4FSK symbol into a byte array. Inverse of byteToSymbols():
 * +1 -> 00, +3 -> 01, -1 -> 10, -3 -> 11, MSB dibit first.
 */
template <size_t N>
inline void setSymbol(std::array<uint8_t, N> &array, size_t pos, int8_t symbol)
{
    bool b0 = false;
    bool b1 = false;
    switch (symbol) {
        case +3:
            b0 = false;
            b1 = true;
            break;
        case +1:
            b0 = false;
            b1 = false;
            break;
        case -1:
            b0 = true;
            b1 = false;
            break;
        case -3:
            b0 = true;
            b1 = true;
            break;
        default:
            assert(false && "unknown Horse 4FSK symbol");
            break;
    }
    setBit<N>(array, 2 * pos, b0);
    setBit<N>(array, 2 * pos + 1, b1);
}

inline int8_t quantizeLevel(int16_t sample, int16_t outerPos, int16_t outerNeg)
{
    if (sample > (2 * outerPos) / 3)
        return +3;
    if (sample < (2 * outerNeg) / 3)
        return -3;
    if (sample > 0)
        return +1;
    return -1;
}

} // namespace horse

#endif // HORSE_UTILS_H
