/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <vector>

extern "C" {
#include "core/input.h"
#include "core/event.h"
}

static keyboard_t fakeKeys = 0;
static long long fakeTick = 0;

extern "C" keyboard_t kbd_getKeys()
{
    return fakeKeys;
}

extern "C" long long getTick()
{
    return fakeTick;
}

/*
 * Advance time by the given amount in 25ms steps, as done by the UI thread,
 * collecting the generated keyboard events.
 */
static std::vector<kbd_msg_t> scanFor(long long ms)
{
    std::vector<kbd_msg_t> events;
    kbd_msg_t msg;

    for (long long t = 0; t < ms; t += 25) {
        fakeTick += 25;
        if (input_scanKeyboard(&msg))
            events.push_back(msg);
    }

    return events;
}

static kbd_msg_t setKeys(keyboard_t keys)
{
    kbd_msg_t msg;

    fakeKeys = keys;
    fakeTick += 25;
    REQUIRE(input_scanKeyboard(&msg));

    return msg;
}

static void releaseAll()
{
    kbd_msg_t msg;

    fakeKeys = 0;
    fakeTick += 25;
    input_scanKeyboard(&msg);
    scanFor(100);
}

TEST_CASE("Keyboard message fits in an event payload", "[input]")
{
    kbd_msg_t msg;
    msg.value = 0;
    msg.long_press = 1;
    msg.keys = (1 << KBD_NUM_KEYS) - 1;
    msg.repeat = 1;

    event_t event;
    event.type = EVENT_KBD;
    event.payload = msg.value;

    kbd_msg_t decoded;
    decoded.value = event.payload;

    REQUIRE(sizeof(event_t) == sizeof(uint32_t));
    REQUIRE(decoded.long_press == 1);
    REQUIRE(decoded.keys == msg.keys);
    REQUIRE(decoded.repeat == 1);
}

TEST_CASE("Short press generates no repeat", "[input]")
{
    releaseAll();

    kbd_msg_t msg = setKeys(KEY_UP);
    REQUIRE(msg.keys == KEY_UP);
    REQUIRE(msg.long_press == 0);
    REQUIRE(msg.repeat == 0);

    REQUIRE(scanFor(300).empty());

    msg = setKeys(0);
    REQUIRE(msg.keys == 0);
    REQUIRE(msg.repeat == 0);
    REQUIRE(scanFor(2000).empty());
}

TEST_CASE("Held key generates long press, then repeats", "[input]")
{
    releaseAll();
    long long pressTs = fakeTick + 25;
    setKeys(KEY_UP);

    std::vector<long long> repeatTs;
    bool longPressSeen = false;
    kbd_msg_t msg;

    // Hold for 2 seconds
    for (int i = 0; i < 80; i++) {
        fakeTick += 25;
        if (!input_scanKeyboard(&msg))
            continue;

        REQUIRE(msg.keys == KEY_UP);
        REQUIRE((msg.long_press ^ msg.repeat) == 1);

        if (msg.long_press) {
            REQUIRE_FALSE(longPressSeen);
            REQUIRE(repeatTs.empty());
            REQUIRE(fakeTick - pressTs == input_longPressTimeout);
            longPressSeen = true;
        } else {
            repeatTs.push_back(fakeTick);
        }
    }

    REQUIRE(longPressSeen);
    REQUIRE(!repeatTs.empty());
    long long firstRepeat = repeatTs[0] - pressTs;
    REQUIRE(firstRepeat == input_longPressTimeout + input_repeatInterval);

    for (size_t i = 1; i < repeatTs.size(); i++)
        REQUIRE(repeatTs[i] - repeatTs[i - 1] == input_repeatInterval);

    // Release stops repeats
    msg = setKeys(0);
    REQUIRE(msg.repeat == 0);
    REQUIRE(scanFor(1000).empty());
}

TEST_CASE("Repeats carry all held keys at a single rate", "[input]")
{
    releaseAll();
    setKeys(KEY_MONI);
    scanFor(300);
    setKeys(KEY_MONI | KEY_8);

    std::vector<kbd_msg_t> events = scanFor(2000);
    size_t repeats = 0;

    for (const auto &ev : events) {
        REQUIRE(ev.keys == (KEY_MONI | KEY_8));
        if (ev.repeat)
            repeats++;
    }

    // Repeats start 800ms after the last keyboard change, every 100ms: two
    // independently held keys must not double the rate.
    REQUIRE(repeats == 13);
}

TEST_CASE("Keyboard change restarts the repeat delay", "[input]")
{
    releaseAll();
    setKeys(KEY_MONI | KEY_8);
    scanFor(1500);

    // Release MONI while still holding 8
    kbd_msg_t msg = setKeys(KEY_8);
    REQUIRE(msg.keys == KEY_8);
    REQUIRE(msg.repeat == 0);

    long long delay = input_longPressTimeout + input_repeatInterval;
    std::vector<kbd_msg_t> events = scanFor(delay - 25);
    for (const auto &ev : events)
        REQUIRE(ev.repeat == 0);

    events = scanFor(25);
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].repeat == 1);
    REQUIRE(events[0].keys == KEY_8);
}
