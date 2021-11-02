/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <cstdio>
#include <cstdint>
#include <random>
#include "M17/M17Golay.h"

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
        uint32_t cword = golay24_encode(value);
        uint32_t emask = generateErrorMask();

        // Check for correct encoding/decoding in absence of errors
        bool decoding_ok = (golay24_decode(cword) == value);

        // Check for correct encoding/decoding in presence of errors
        uint16_t decoded = golay24_decode(cword ^ emask);
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
