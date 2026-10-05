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
inline bool getBit(const std::array<uint8_t, N>& array, size_t pos)
{
    size_t i = pos / 8;
    size_t j = pos % 8;
    return (array[i] >> (7 - j)) & 0x01;
}

template <size_t N>
inline void setBit(std::array<uint8_t, N>& array, size_t pos, bool bit)
{
    size_t i     = pos / 8;
    size_t j     = pos % 8;
    uint8_t mask = 1 << (7 - j);
    array[i] = (array[i] & ~mask) | (bit ? mask : 0x00);
}

inline std::array<int8_t, 4> byteToSymbols(uint8_t value)
{
    static constexpr int8_t LUT[] = {+1, +3, -1, -3};
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
    return {a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3]};
}

}  // namespace horse

#endif  // HORSE_UTILS_H
