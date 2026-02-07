/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <array>
#include <algorithm>
#include "protocols/M17/Callsign.hpp"
#include "protocols/M17/M17Datatypes.hpp"

using namespace std;

TEST_CASE("Encode callsign AB1CD", "[m17][callsign]")
{
    const char callsign[] = "AB1CD";
    M17::call_t expected = { 0x00, 0x00, 0x00, 0x9f, 0xdd, 0x51 };

    M17::Callsign test = M17::Callsign(callsign);
    M17::call_t actual = test;
    REQUIRE(equal(begin(actual), end(actual), begin(expected), end(expected)));
}

TEST_CASE("Encode empty callsign", "[m17][callsign]")
{
    const char callsign[] = "";
    M17::call_t expected = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    M17::Callsign test = M17::Callsign(callsign);
    M17::call_t actual = test;
    REQUIRE(equal(begin(actual), end(actual), begin(expected), end(expected)));
}

TEST_CASE("Decode callsign AB1CD", "[m17][callsign]")
{
    M17::call_t callsign = { 0x00, 0x00, 0x00, 0x9f, 0xdd, 0x51 };

    M17::Callsign test = M17::Callsign(callsign);
    const char expected[] = "AB1CD";
    const char *actual = test;
    REQUIRE(strcmp(expected, actual) == 0);
}
