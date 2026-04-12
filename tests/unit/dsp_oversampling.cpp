/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include "core/dsp.h"

TEST_CASE("Oversampling decimation with factor 1 passes through",
          "[dsp][oversampling]")
{
    struct oversamplingBlock blk;
    dsp_resetState(blk);
    dsp_oversamplingSetOversampling(&blk, 1);

    uint16_t sample = 1000;
    REQUIRE(dsp_oversamplingDecimate(&blk, &sample) == true);
    REQUIRE(sample == 1000);
}

TEST_CASE("Oversampling decimation with factor 4 accumulates and averages",
          "[dsp][oversampling]")
{
    struct oversamplingBlock blk;
    dsp_resetState(blk);
    dsp_oversamplingSetOversampling(&blk, 4);

    uint16_t sample;

    sample = 100;
    REQUIRE(dsp_oversamplingDecimate(&blk, &sample) == false);

    sample = 200;
    REQUIRE(dsp_oversamplingDecimate(&blk, &sample) == false);

    sample = 300;
    REQUIRE(dsp_oversamplingDecimate(&blk, &sample) == false);

    sample = 400;
    REQUIRE(dsp_oversamplingDecimate(&blk, &sample) == true);
    // Average of 100, 200, 300, 400 = 250
    REQUIRE(sample == 250);
}

TEST_CASE("Oversampling decimation with factor 8 does not overflow",
          "[dsp][oversampling]")
{
    struct oversamplingBlock blk;
    dsp_resetState(blk);
    dsp_oversamplingSetOversampling(&blk, 8);

    // Feed 8 maximum uint16_t samples; sum = 8 * 65535 = 524280
    // This must not overflow the accumulator.
    uint16_t sample;
    for (int i = 0; i < 7; i++)
    {
        sample = 65535;
        REQUIRE(dsp_oversamplingDecimate(&blk, &sample) == false);
    }

    sample = 65535;
    REQUIRE(dsp_oversamplingDecimate(&blk, &sample) == true);
    REQUIRE(sample == 65535);
}

TEST_CASE("Oversampling decimation resets between frames",
          "[dsp][oversampling]")
{
    struct oversamplingBlock blk;
    dsp_resetState(blk);
    dsp_oversamplingSetOversampling(&blk, 2);

    uint16_t sample;

    // First pair
    sample = 100;
    REQUIRE(dsp_oversamplingDecimate(&blk, &sample) == false);
    sample = 200;
    REQUIRE(dsp_oversamplingDecimate(&blk, &sample) == true);
    REQUIRE(sample == 150);

    // Second pair should start fresh
    sample = 500;
    REQUIRE(dsp_oversamplingDecimate(&blk, &sample) == false);
    sample = 700;
    REQUIRE(dsp_oversamplingDecimate(&blk, &sample) == true);
    REQUIRE(sample == 600);
}
