/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <limits.h>
#include <inttypes.h>
#include "protocols/M17/M17DSP.hpp"

#define IMPULSE_SIZE 4096

TEST_CASE("RRC filter produces non-zero impulse response", "[m17][rrc]")
{
    // Allocate impulse signal
    int16_t impulse[IMPULSE_SIZE] = { 0 };
    impulse[0] = SHRT_MAX;

    // Apply RRC on impulse signal
    int16_t filtered_impulse[IMPULSE_SIZE] = { 0 };
    bool hasNonZero = false;
    for(size_t i = 0; i < IMPULSE_SIZE; i++)
    {
        float elem = static_cast< float >(impulse[i]);
        filtered_impulse[i] = static_cast< int16_t >(M17::rrc_48k(0.10 * elem));
        if(filtered_impulse[i] != 0)
            hasNonZero = true;
    }

    REQUIRE(hasNonZero);
}
