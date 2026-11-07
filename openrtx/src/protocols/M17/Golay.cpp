/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/Golay.hpp"

using namespace M17;

/*
 * Data encoding matrix for Golay(24,12) code with generator polynomial 0xC75
 */
static constexpr uint16_t encode_matrix[12] =
{
    0x8eb, 0x93e, 0xa97, 0xdc6, 0x367, 0x6cd,
    0xd99, 0x3da, 0x7b4, 0xf68, 0x63b, 0xc75
};


/*
 * Data decoding matrix for Golay(24,12) code with generator polynomial 0xC75
 */
static constexpr uint16_t decode_matrix[12] =
{
    0xc75, 0x49f, 0x93e, 0x6e3, 0xdc6, 0xf13,
    0xab9, 0x1ed, 0x3da, 0x7b4, 0xf68, 0xa4f
};


uint16_t Golay24::calcChecksum(const uint16_t& value)
{
    uint16_t checksum = 0;

    for(uint8_t i = 0; i < 12; i++)
    {
        if(value & (1 << i))
        {
            checksum ^= encode_matrix[i];
        }
    }

    return checksum;
}


/**
 * Detect and correct errors in a Golay(24,12) codeword.
 *
 * @param codeword: input codeword.
 * @return bitmask corresponding to detected bit errors in the codeword, or
 * 0xFFFFFFFF if bit errors are unrecoverable.
 */
uint32_t Golay24::detectErrors(const uint32_t& codeword)
{
    uint16_t data   = codeword >> 12;
    uint16_t parity = codeword & 0xFFF;

    uint16_t syndrome = parity ^ calcChecksum(data);

    if(__builtin_popcount(syndrome) <= 3)
    {
        return syndrome;
    }

    for(uint8_t i = 0; i<12; i++)
    {
        uint16_t e = 1 << i;
        uint16_t coded_error = encode_matrix[i];

        if(__builtin_popcount(syndrome^coded_error) <= 2)
        {
            return (e << 12) | (syndrome ^ coded_error);
        }
    }


    uint16_t inv_syndrome = 0;
    for(uint8_t i = 0; i < 12; i++)
    {
        if(syndrome & (1 << i))
        {
            inv_syndrome ^= decode_matrix[i];
        }
    }

    if(__builtin_popcount(inv_syndrome) <= 3)
    {
        return inv_syndrome << 12;
    }

    for(uint8_t i = 0; i < 12; i++ )
    {
        uint16_t e = 1 << i;
        uint16_t coding_error = decode_matrix[i];
        if(__builtin_popcount(inv_syndrome ^ coding_error) <= 2 )
        {
            return ((inv_syndrome ^ coding_error) << 12) | e;
        }
    }

    return 0xFFFFFFFF;
}


/// Least reliable bits flipped in every combination, half the code distance.
static constexpr uint8_t  CHASE_BITS = 4;
/// Margin the best candidate needs over the runner-up, half a confident bit.
static constexpr uint32_t MIN_MARGIN = 0x7FFF;

uint16_t M17::golay24_softDecode(const std::array< uint16_t, 24 >& soft)
{
    // Hard decisions and their reliability, |2 * soft - 0xFFFF|, by bit number
    uint32_t hard = 0;
    uint16_t rel[24];

    for(uint8_t i = 0; i < 24; i++)
    {
        uint8_t bit = 23 - i;

        if(soft[i] >= 0x8000)
        {
            hard    |= 1UL << bit;
            rel[bit] = 2 * (soft[i] - 0x8000) + 1;
        }
        else
        {
            rel[bit] = 2 * (0x7FFF - soft[i]) + 1;
        }
    }

    // Least reliable bits, ties going to the more significant one
    uint8_t  weak[CHASE_BITS];
    uint32_t taken = 0;

    for(uint8_t k = 0; k < CHASE_BITS; k++)
    {
        int8_t min = -1;

        for(int8_t bit = 23; bit >= 0; bit--)
        {
            if((taken & (1UL << bit)) != 0)
                continue;

            if((min < 0) || (rel[bit] < rel[min]))
                min = bit;
        }

        weak[k] = min;
        taken  |= 1UL << min;
    }

    // Distance of a candidate: reliability of the bits it changes
    uint32_t best     = 0;
    uint32_t bestDist = UINT32_MAX;
    uint32_t nextDist = UINT32_MAX;

    for(uint8_t pattern = 0; pattern < (1 << CHASE_BITS); pattern++)
    {
        uint32_t flips = 0;

        for(uint8_t k = 0; k < CHASE_BITS; k++)
        {
            if(pattern & (1 << k))
                flips |= 1UL << weak[k];
        }

        uint32_t errors = Golay24::detectErrors(hard ^ flips);
        if(errors == 0xFFFFFFFF)
            continue;

        uint32_t diff      = flips ^ errors;
        uint32_t candidate = hard ^ diff;
        if((candidate == best) && (bestDist != UINT32_MAX))
            continue;

        uint32_t dist = 0;
        for(uint8_t bit = 0; bit < 24; bit++)
        {
            if(diff & (1UL << bit))
                dist += rel[bit];
        }

        if(dist < bestDist)
        {
            nextDist = bestDist;
            bestDist = dist;
            best     = candidate;
        }
        else if(dist < nextDist)
        {
            nextDist = dist;
        }
    }

    // Nothing decodable, or a runner-up too close to tell apart
    if(bestDist == UINT32_MAX)
        return 0xFFFF;

    if((nextDist != UINT32_MAX) && ((nextDist - bestDist) <= MIN_MARGIN))
        return 0xFFFF;

    return (best >> 12) & 0x0FFF;
}
