/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/Golay.hpp"
#include "core/utils.h"
#include <cstdlib>

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


/// Bits in a Golay(24,12) codeword.
static constexpr uint8_t  CODE_BITS  = 24;
/// Least reliable bits flipped in every combination, half the code distance.
static constexpr uint8_t  CHASE_BITS = 4;
/// Margin the best candidate needs over the runner-up, half a confident bit.
static constexpr uint32_t MIN_MARGIN = 0x7FFF;

/**
 * Find the least reliable bits of a received word, ties going to the more
 * significant bit.
 *
 * @param rel: reliability of each bit, by bit number.
 * @param weak: bit numbers of the least reliable bits, least reliable first.
 */
static void leastReliable(const uint16_t (&rel)[CODE_BITS],
                          uint8_t (&weak)[CHASE_BITS])
{
    uint32_t taken = 0;

    for(uint8_t k = 0; k < ARRAY_SIZE(weak); k++)
    {
        int8_t min = -1;

        for(int8_t bit = CODE_BITS - 1; bit >= 0; bit--)
        {
            if((taken & (1UL << bit)) != 0)
                continue;

            if((min < 0) || (rel[bit] < rel[min]))
                min = bit;
        }

        weak[k] = min;
        taken  |= 1UL << min;
    }
}

/**
 * Distance of a candidate codeword from the received soft bits: the summed
 * reliability of the bits it changes.
 *
 * @param diff: bits the candidate changes in the received word.
 * @param rel: reliability of each bit, by bit number.
 * @return the distance.
 */
static uint32_t distance(const uint32_t diff, const uint16_t (&rel)[CODE_BITS])
{
    uint32_t dist = 0;

    for(uint8_t bit = 0; bit < ARRAY_SIZE(rel); bit++)
    {
        if((diff & (1UL << bit)) != 0)
            dist += rel[bit];
    }

    return dist;
}

uint16_t M17::golay24_softDecode(const std::array< uint16_t, 24 >& soft)
{
    // Hard decisions and their reliability, |2 * soft - 0xFFFF|, by bit
    // number, the first soft bit being the most significant
    uint32_t hard = 0;
    uint16_t rel[CODE_BITS];

    for(uint8_t i = 0; i < ARRAY_SIZE(rel); i++)
    {
        uint8_t bit     = CODE_BITS - 1 - i;
        int32_t centred = (2 * soft[i]) - UINT16_MAX;

        if(centred > 0)
            hard |= 1UL << bit;

        rel[bit] = std::abs(centred);
    }

    uint8_t weak[CHASE_BITS];
    leastReliable(rel, weak);

    // Try every combination of flips of the weak bits, keeping the closest
    // candidate and the distance of the runner-up
    uint32_t best     = 0;
    uint32_t bestDist = UINT32_MAX;
    uint32_t nextDist = UINT32_MAX;

    for(uint8_t pattern = 0; pattern < (1 << CHASE_BITS); pattern++)
    {
        uint32_t flips = 0;

        for(uint8_t k = 0; k < CHASE_BITS; k++)
        {
            if((pattern & (1 << k)) != 0)
                flips |= 1UL << weak[k];
        }

        uint32_t errors = Golay24::detectErrors(hard ^ flips);
        if(errors == 0xFFFFFFFF)
            continue;

        uint32_t diff      = flips ^ errors;
        uint32_t candidate = hard ^ diff;
        if((candidate == best) && (bestDist != UINT32_MAX))
            continue;

        uint32_t dist = distance(diff, rel);
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
