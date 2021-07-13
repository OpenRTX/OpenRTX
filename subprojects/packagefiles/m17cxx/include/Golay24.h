// Copyright 2020 Rob Riggs <rob@mobilinkd.com>
// All rights reserved.

#pragma once

#include <array>
#include <algorithm>
#include <utility>
//#include <iostream>
//#include <iomanip>

namespace mobilinkd {

// Parts are adapted from:
// http://aqdi.com/articles/using-the-golay-error-detection-and-correction-code-3/

namespace Golay24
{

namespace detail
{

// Need a constexpr sort.
// https://stackoverflow.com/a/40030044/854133
template<class T>
constexpr void swap(T& l, T& r)
{
    T tmp = std::move(l);
    l = std::move(r);
    r = std::move(tmp);
}

template <typename T, size_t N>
struct array
{
    constexpr T& operator[](size_t i)
    {
        return arr[i];
    }

    constexpr const T& operator[](size_t i) const
    {
        return arr[i];
    }

    constexpr const T* begin() const
    {
        return arr;
    }
    constexpr const T* end() const
    {
        return arr + N;
    }

    T arr[N];
};

template <typename T, size_t N>
constexpr void sort_impl(array<T, N> &array, size_t left, size_t right)
{
    if (left < right)
    {
        size_t m = left;

        for (size_t i = left + 1; i<right; i++)
            if (array[i]<array[left])
                swap(array[++m], array[i]);

        swap(array[left], array[m]);

        sort_impl(array, left, m);
        sort_impl(array, m + 1, right);
    }
}

template <typename T, size_t N>
constexpr array<T, N> sort(array<T, N> array)
{
    auto sorted = array;
    sort_impl(sorted, 0, N);
    return sorted;
}

} // detail

// static constexpr uint16_t POLY = 0xAE3;
constexpr uint16_t POLY = 0xC75;

struct __attribute__((packed)) SyndromeMapEntry
{
    constexpr const SyndromeMapEntry& operator=(const SyndromeMapEntry& s) const
    {
        return s;
    }

    uint32_t a{0};
    uint16_t b{0};
};



/**
 * Calculate the syndrome of a [23,12] Golay codeword.
 *
 * @return the 11-bit syndrome of the codeword in bits [22:12].
 */
constexpr uint32_t syndrome(uint32_t codeword)
{
    codeword &= 0xffffffl;
    for (size_t i = 0; i != 12; ++i)
    {
        if (codeword & 1)
            codeword ^= POLY;
        codeword >>= 1;
    }
    return (codeword << 12);
}

constexpr bool parity(uint32_t codeword)
{
    return __builtin_popcount(codeword) & 1;
}

constexpr SyndromeMapEntry makeSyndromeMapEntry(uint64_t val)
{
    return SyndromeMapEntry{uint32_t(val >> 16), uint16_t(val & 0xFFFF)};
}

constexpr uint64_t makeSME(uint64_t syndrome, uint32_t bits)
{
    return (syndrome << 24) | (bits & 0xFFFFFF);
}

constexpr size_t LUT_SIZE = 2048;

constexpr std::array<SyndromeMapEntry, LUT_SIZE> make_lut()
{
    constexpr size_t VECLEN=23;
    detail::array<uint64_t, LUT_SIZE> result{};

    size_t index = 0;
    result[index++] = makeSME(syndrome(0), 0);

    for (size_t i = 0; i != VECLEN; ++i)
    {
        auto v = (1 << i);
        result[index++] = makeSME(syndrome(v), v);
    }

    for (size_t i = 0; i != VECLEN - 1; ++i)
    {
        for (size_t j = i + 1; j != VECLEN; ++j)
        {
            auto v = (1 << i) | (1 << j);
            result[index++] = makeSME(syndrome(v), v);
        }
    }

    for (size_t i = 0; i != VECLEN - 2; ++i)
    {
        for (size_t j = i + 1; j != VECLEN - 1; ++j)
        {
            for (size_t k = j + 1; k != VECLEN; ++k)
            {
                auto v = (1 << i) | (1 << j) | (1 << k);
                result[index++] = makeSME(syndrome(v), v);
            }
        }
    }

    result = detail::sort(result);

    constexpr std::array<SyndromeMapEntry, LUT_SIZE> tmp;
    for (size_t i = 0; i != LUT_SIZE; ++i)
    {
        tmp[i] = makeSyndromeMapEntry(result[i]);
    }

    return tmp;
}

constexpr auto LUT = make_lut();

/**
 * Calculate [23,12] Golay codeword.
 *
 * @return checkbits(11)|data(12).
 */
constexpr uint32_t encode23(uint16_t data)
{
    // data &= 0xfff;
    uint32_t codeword = data;
    for (size_t i = 0; i != 12; ++i)
    {
        if (codeword & 1)
            codeword ^= POLY;
        codeword >>= 1;
    }
    return codeword | (data << 11);
}

constexpr uint32_t encode24(uint16_t data)
{
    auto codeword = encode23(data);
    return ((codeword << 1) | parity(codeword));
}

static bool decode(uint32_t input, uint32_t& output)
{
    auto syndrm = syndrome(input >> 1);
    auto it = std::lower_bound(LUT.begin(), LUT.end(), syndrm,
        [](const SyndromeMapEntry& sme, uint32_t val){
            return (sme.a >> 8) < val;
        });

    if ((it->a >> 8) == syndrm)
    {
        // Build the correction from the compressed entry.
        auto correction = ((((it->a & 0xFF) << 16) | it->b) << 1);
        // Apply the correction to the input.
        output = input ^ correction;
        // Only test parity for 3-bit errors.
        return __builtin_popcount(syndrm) < 3 || !parity(output);
    }

    return false;
}

} // Golay24

} // mobilinkd
