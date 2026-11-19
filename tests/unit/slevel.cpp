/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include "core/utils.h"

TEST_CASE("S-level thresholds invert the S-meter scale", "[utils]")
{
    // Each S-level starts at its threshold, the dB below is the level under
    for (uint8_t s = 1; s <= SLEVEL_MAX; s++) {
        rssi_t rssi = sLevelToRssi(s);

        REQUIRE(rssiToSlevel(rssi) == s);
        REQUIRE(rssiToSlevel(rssi - 1) == s - 1);
    }

    REQUIRE(sLevelToRssi(9) == -73);
    REQUIRE(sLevelToRssi(SLEVEL_MAX) == -53);
    REQUIRE(sLevelToRssi(0) == -127);

    // Levels saved by older versions, up to 15, are S9 + 20 dB
    REQUIRE(sLevelToRssi(15) == sLevelToRssi(SLEVEL_MAX));
}
