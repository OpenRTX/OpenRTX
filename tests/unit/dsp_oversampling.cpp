/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include "core/dsp.h"

TEST_CASE("Oversampling decimation with factor 1 passes through",
          "[dsp][oversampling]")
{
    struct decimatorState blk;
    dsp_resetState(blk);

    int16_t sample = 1000;
    REQUIRE(dsp_decimator(&blk, &sample, 1) == true);
    REQUIRE(sample == 1000);
}

TEST_CASE("Oversampling decimation with factor 4 accumulates and averages",
          "[dsp][oversampling]")
{
    struct decimatorState blk;
    dsp_resetState(blk);

    int16_t sample;

    sample = 100;
    REQUIRE(dsp_decimator(&blk, &sample, 4) == false);

    sample = 200;
    REQUIRE(dsp_decimator(&blk, &sample, 4) == false);

    sample = 300;
    REQUIRE(dsp_decimator(&blk, &sample, 4) == false);

    sample = 400;
    REQUIRE(dsp_decimator(&blk, &sample, 4) == true);
    // Average of 100, 200, 300, 400 = 250
    REQUIRE(sample == 250);
}

TEST_CASE("Oversampling decimation with factor 8 does not overflow",
          "[dsp][oversampling]")
{
    struct decimatorState blk;
    dsp_resetState(blk);

    // Feed 8 samples at ADC full-scale (12-bit: 4095); sum = 8 * 4095 = 32760
    // The uint32_t accumulator must handle this without overflow.
    int16_t sample;
    for (int i = 0; i < 7; i++) {
        sample = 4095;
        REQUIRE(dsp_decimator(&blk, &sample, 8) == false);
    }

    sample = 4095;
    REQUIRE(dsp_decimator(&blk, &sample, 8) == true);
    REQUIRE(sample == 4095);
}

TEST_CASE("Oversampling decimation resets between frames",
          "[dsp][oversampling]")
{
    struct decimatorState blk;
    dsp_resetState(blk);

    int16_t sample;

    // First pair
    sample = 100;
    REQUIRE(dsp_decimator(&blk, &sample, 2) == false);
    sample = 200;
    REQUIRE(dsp_decimator(&blk, &sample, 2) == true);
    REQUIRE(sample == 150);

    // Second pair should start fresh
    sample = 500;
    REQUIRE(dsp_decimator(&blk, &sample, 2) == false);
    sample = 700;
    REQUIRE(dsp_decimator(&blk, &sample, 2) == true);
    REQUIRE(sample == 600);
}

TEST_CASE("Oversampling decimation with factor 1 and zero returns immediately",
          "[dsp][oversampling]")
{
    struct decimatorState blk;
    dsp_resetState(blk);

    int16_t sample = 0;
    REQUIRE(dsp_decimator(&blk, &sample, 1) == true);
    REQUIRE(sample == 0);
}
