/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <vector>

#include "core/cw_keyer.h"

static constexpr uint8_t WPM = 20;
static constexpr long long DIT = 60; // ms at 20 WPM
static constexpr long long DAH = 3 * DIT;

/**
 * A stretch of key state: down or up, and its length in ms.
 */
struct Segment {
    bool down;
    long long length;

    bool operator==(const Segment &other) const
    {
        return (down == other.down) && (length == other.length);
    }
};

/**
 * Run the keyer every millisecond and collect the key states.
 *
 * @param paddles: paddles closed at a given time, in ms.
 * @param duration: run time, in ms.
 * @return key states, the last one cut at the end of the run.
 */
static std::vector<Segment> run(std::function<uint8_t(long long)> paddles,
                                long long duration)
{
    struct cwKeyer keyer;
    std::vector<Segment> segments;

    cwKeyer_init(&keyer, WPM);
    for (long long t = 0; t < duration; t++) {
        bool down = cwKeyer_update(&keyer, paddles(t), t);
        if (segments.empty() || (segments.back().down != down))
            segments.push_back({ down, 0 });
        segments.back().length++;
    }

    return segments;
}

TEST_CASE("CW keyer stays up without paddles", "[cw]")
{
    auto keys = run([](long long) { return 0; }, 500);
    REQUIRE(keys == std::vector<Segment>{ { false, 500 } });
}

TEST_CASE("CW keyer repeats dits and dahs while held", "[cw]")
{
    SECTION("Dits")
    {
        auto keys = run([](long long) { return CW_PADDLE_DIT; }, 4 * DIT);
        REQUIRE(
            keys
            == std::vector<Segment>{
                { true, DIT }, { false, DIT }, { true, DIT }, { false, DIT } });
    }

    SECTION("Dahs")
    {
        auto keys = run([](long long) { return CW_PADDLE_DAH; },
                        2 * (DAH + DIT));
        REQUIRE(
            keys
            == std::vector<Segment>{
                { true, DAH }, { false, DIT }, { true, DAH }, { false, DIT } });
    }
}

TEST_CASE("CW keyer finishes an element after a short press", "[cw]")
{
    auto keys = run([](long long t) { return (t < 5) ? CW_PADDLE_DAH : 0; },
                    400);
    REQUIRE(keys
            == std::vector<Segment>{ { true, DAH }, { false, 400 - DAH } });
}

TEST_CASE("CW keyer alternates on a squeeze, starting with a dit", "[cw]")
{
    auto keys = run([](long long) { return CW_PADDLE_DIT | CW_PADDLE_DAH; },
                    2 * (DIT + DAH + 2 * DIT));
    REQUIRE(keys
            == std::vector<Segment>{ { true, DIT },
                                     { false, DIT },
                                     { true, DAH },
                                     { false, DIT },
                                     { true, DIT },
                                     { false, DIT },
                                     { true, DAH },
                                     { false, DIT } });
}

TEST_CASE("CW keyer remembers the opposite paddle", "[cw]")
{
    // Dah tapped during the first dit of a held dit paddle: "dit dah dit"
    auto keys = run(
        [](long long t) {
            uint8_t paddles = (t < 2 * DIT + DAH + 2 * DIT) ? CW_PADDLE_DIT : 0;
            if ((t >= 10) && (t < 20))
                paddles |= CW_PADDLE_DAH;
            return paddles;
        },
        1000);
    REQUIRE(keys
            == std::vector<Segment>{ { true, DIT },
                                     { false, DIT },
                                     { true, DAH },
                                     { false, DIT },
                                     { true, DIT },
                                     { false, 1000 - 3 * DIT - DAH - DIT } });
}

TEST_CASE("CW keyer applies a new speed from the next element", "[cw]")
{
    struct cwKeyer keyer;
    std::vector<long long> downs;
    bool last = false;
    long long start = 0;

    cwKeyer_init(&keyer, WPM);
    for (long long t = 0; t < 1000; t++) {
        if (t == 10)
            cwKeyer_setSpeed(&keyer, 2 * WPM);

        bool down = cwKeyer_update(&keyer, CW_PADDLE_DIT, t);
        if (down && !last)
            start = t;
        if (!down && last)
            downs.push_back(t - start);
        last = down;
    }

    REQUIRE(downs.size() > 2);
    REQUIRE(downs[0] == DIT);
    REQUIRE(downs[1] == DIT / 2);
}
