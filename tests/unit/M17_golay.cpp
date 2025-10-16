/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>
#include <cstdint>
#include <random>
#include "protocols/M17/M17Golay.hpp"

using namespace std;

default_random_engine rng;

/**
 * Generate a mask with a random number of bit errors in random positions.
 */
uint32_t generateErrorMask()
{
    uint32_t errorMask = 0;
    uniform_int_distribution< uint8_t > numErrs(0, 5);
    uniform_int_distribution< uint8_t > errPos(0, 23);

    for(uint8_t i = 0; i < numErrs(rng); i++)
    {
        errorMask |= 1 << errPos(rng);
    }

    return errorMask;
}

int main()
{
    uniform_int_distribution< uint16_t > rndValue(0, 2047);

    for(uint32_t i = 0; i < 10000; i++)
    {
        uint16_t value = rndValue(rng);
        uint32_t cword = M17::golay24_encode(value);
        uint32_t emask = generateErrorMask();

        // Check for correct encoding/decoding in absence of errors
        bool decoding_ok = (M17::golay24_decode(cword) == value);

        // Check for correct encoding/decoding in presence of errors
        uint16_t decoded = M17::golay24_decode(cword ^ emask);
        bool correcting_ok = false;

        // For four or more bit errors, decode should return 0xFFFF (uncorrectable error)
        if((__builtin_popcount(emask) >= 4) && (decoded == 0xFFFF))
        {
            correcting_ok = true;
        }

        // Less than four errors should be corrected.
        if(decoded == value) correcting_ok = true;

        int input_error_count = __builtin_popcount(emask);

        printf("Value %04x, emask %08x errs %d -> d %s, c %s\n",
               value,
               emask,
               input_error_count,
               decoding_ok   ? "OK" : "FAIL",
               correcting_ok ? "OK" : "FAIL");

        if(input_error_count <= 4 && (!decoding_ok || !correcting_ok))
            return -1;
    }

    return 0;
}
