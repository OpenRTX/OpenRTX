/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

extern "C" {
#include "core/state.h"
#include "core/settings.h"
}

extern bool _ui_checkStandby(long long);
extern state_t state;

static void check_timer_threshold(display_timer_t conf, long long time_sec)
{
    state.settings.brightness_timer = conf;

    long long below = (time_sec - 1) * 1000;
    long long at    = time_sec * 1000;

    INFO("Timer enum: " << conf << " threshold: " << time_sec << "s");
    REQUIRE(_ui_checkStandby(below) == false);
    REQUIRE(_ui_checkStandby(at)    == true);
}

TEST_CASE("Backlight timer thresholds", "[ui]")
{
    SECTION("5 second timer")  { check_timer_threshold(TIMER_5S,  5); }
    SECTION("10 second timer") { check_timer_threshold(TIMER_10S, 10); }
    SECTION("15 second timer") { check_timer_threshold(TIMER_15S, 15); }
    SECTION("20 second timer") { check_timer_threshold(TIMER_20S, 20); }
    SECTION("25 second timer") { check_timer_threshold(TIMER_25S, 25); }
    SECTION("30 second timer") { check_timer_threshold(TIMER_30S, 30); }

    SECTION("1 minute timer")  { check_timer_threshold(TIMER_1M,  1 * 60); }
    SECTION("2 minute timer")  { check_timer_threshold(TIMER_2M,  2 * 60); }
    SECTION("3 minute timer")  { check_timer_threshold(TIMER_3M,  3 * 60); }
    SECTION("4 minute timer")  { check_timer_threshold(TIMER_4M,  4 * 60); }
    SECTION("5 minute timer")  { check_timer_threshold(TIMER_5M,  5 * 60); }

    SECTION("15 minute timer") { check_timer_threshold(TIMER_15M, 15 * 60); }
    SECTION("30 minute timer") { check_timer_threshold(TIMER_30M, 30 * 60); }
    SECTION("45 minute timer") { check_timer_threshold(TIMER_45M, 45 * 60); }

    SECTION("1 hour timer")    { check_timer_threshold(TIMER_1H,  60 * 60); }
}

TEST_CASE("Backlight timer OFF never triggers standby", "[ui]")
{
    state.settings.brightness_timer = TIMER_OFF;
    REQUIRE(_ui_checkStandby(0) == false);
    REQUIRE(_ui_checkStandby(60LL * 60 * 24 * 1000) == false);
}
