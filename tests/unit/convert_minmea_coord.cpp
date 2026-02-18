/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

extern "C" {
#include <minmea.h>
#include "core/gps.h"
}

TEST_CASE("minmea coordinate conversion", "[gps]")
{
    SECTION("Standard positive coordinate")
    {
        struct minmea_float test = { 5333735, 1000 };
        REQUIRE(minmea_tofixedpoint(&test) == 53562250);
    }

    SECTION("Zero coordinate")
    {
        struct minmea_float test = { 0, 1 };
        REQUIRE(minmea_tofixedpoint(&test) == 0);
    }

    SECTION("Negative coordinate")
    {
        struct minmea_float test = { -5333735, 1000 };
        REQUIRE(minmea_tofixedpoint(&test) == -53562250);
    }

    SECTION("Small negative coordinate")
    {
        struct minmea_float test = { -330, 1000 };
        REQUIRE(minmea_tofixedpoint(&test) == -5500);
    }

    SECTION("Negative coordinate with fractional part")
    {
        struct minmea_float test = { -3296, 1000 };
        REQUIRE(minmea_tofixedpoint(&test) == -54933);
    }
}
