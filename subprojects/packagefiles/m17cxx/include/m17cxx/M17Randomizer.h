// Copyright 2020 Mobilinkd LLC.

#pragma once

#include <array>
#include <experimental/array>
#include <cstdint>
#include <cstddef>

namespace mobilinkd
{

namespace detail
{

// M17 randomization matrix.
extern const std::array<uint8_t, 46> DC;

}

template <size_t N = 368>
struct M17Randomizer
{
    std::array<int8_t, N> dc_;

    M17Randomizer()
    {
        size_t i = 0;
        for (auto b : detail::DC)
        {
            for (size_t j = 0; j != 8; ++j)
            {
                dc_[i++] = (b >> (7 - j)) & 1 ? -1 : 1;
            }
        }
    }

    // Randomize and derandomize are the same operation.
    void operator()(std::array<int8_t, N>& frame)
    {
        for (size_t i = 0; i != N; ++i)
        {
            frame[i] *= dc_[i];
        }
    }

    void randomize(std::array<int8_t, N>& frame)
    {
        for (size_t i = 0; i != N; ++i)
        {
            frame[i] ^= (dc_[i] == -1);
        }
    }

};

template <size_t N = 46>
struct M17ByteRandomizer
{
    // Randomize and derandomize are the same operation.
    void operator()(std::array<uint8_t, N>& frame)
    {
        for (size_t i = 0; i != N; ++i)
        {
            for (size_t j = 8; j != 0; --j)
            {
                uint8_t mask = 1 << (j - 1);
                frame[i] = (frame[i] & ~mask) | ((frame[i] & mask) ^ (detail::DC[i] & mask));
            }
        }
    }
};


} // mobilinkd
