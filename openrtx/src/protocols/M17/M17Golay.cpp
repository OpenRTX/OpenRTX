/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/M17Golay.hpp"

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
