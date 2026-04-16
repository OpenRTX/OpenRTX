/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <cstdio>

#include "rtx/SMSQueue.hpp"

// ===========================================================================
// Section 1 – SMS Queue Management (public API)
// ===========================================================================

TEST_CASE("get returns false when the queue is empty", "[m17][sms][queue]")
{
    SMSQueue queue;

    char sender[32] = {};
    char message[822] = {};

    REQUIRE(queue.count() == 0);
    REQUIRE(queue.get(0, sender, sizeof(sender), message, sizeof(message))
            == false);
}

TEST_CASE("get returns false for an index beyond the queue size",
          "[m17][sms][queue]")
{
    SMSQueue queue;
    queue.push("W1AW", "Only message");

    char sender[32] = {};
    char message[822] = {};

    REQUIRE(queue.get(0, sender, sizeof(sender), message, sizeof(message))
            == true);
    REQUIRE(queue.get(1, sender, sizeof(sender), message, sizeof(message))
            == false);
}

TEST_CASE("get copies the correct sender callsign and message text",
          "[m17][sms][queue]")
{
    SMSQueue queue;
    queue.push("KD9NLK", "Hello from M17 SMS");

    char sender[32] = {};
    char message[822] = {};

    REQUIRE(queue.get(0, sender, sizeof(sender), message, sizeof(message))
            == true);
    REQUIRE(strcmp(sender, "KD9NLK") == 0);
    REQUIRE(strcmp(message, "Hello from M17 SMS") == 0);
}

TEST_CASE("erase removes the indexed entry and shifts remaining messages",
          "[m17][sms][queue]")
{
    SMSQueue queue;
    queue.push("AA1AA", "First message");
    queue.push("BB2BB", "Second message");

    REQUIRE(queue.count() == 2);

    queue.erase(0);

    REQUIRE(queue.count() == 1);

    char sender[32] = {};
    char message[822] = {};

    REQUIRE(queue.get(0, sender, sizeof(sender), message, sizeof(message))
            == true);
    REQUIRE(strcmp(sender, "BB2BB") == 0);
    REQUIRE(strcmp(message, "Second message") == 0);
}

TEST_CASE("Queue eviction removes the oldest message when at capacity",
          "[m17][sms][queue]")
{
    SMSQueue queue;

    for (int i = 0; i < 10; i++) {
        char sender[16], msg[32];
        snprintf(sender, sizeof(sender), "CALL%d", i);
        snprintf(msg, sizeof(msg), "Message %d", i);
        queue.push(sender, msg);
    }

    REQUIRE(queue.count() == 10);

    // Push an 11th message – oldest (CALL0) should get evicted.
    queue.push("CALL10", "Message 10");

    REQUIRE(queue.count() == 10);

    char sender[32] = {};
    char message[822] = {};

    // CALL0 was evicted; CALL1 is now at index 0.
    REQUIRE(queue.get(0, sender, sizeof(sender), message, sizeof(message))
            == true);
    REQUIRE(strcmp(sender, "CALL1") == 0);
    REQUIRE(strcmp(message, "Message 1") == 0);

    // The 11th injection (CALL10) is at the end of the queue.
    REQUIRE(queue.get(9, sender, sizeof(sender), message, sizeof(message))
            == true);
    REQUIRE(strcmp(sender, "CALL10") == 0);
    REQUIRE(strcmp(message, "Message 10") == 0);
}

TEST_CASE("clear removes all messages and resets count", "[m17][sms][queue]")
{
    SMSQueue queue;
    queue.push("W1AW", "Test msg 1");
    queue.push("W2AW", "Test msg 2");

    REQUIRE(queue.count() == 2);

    queue.clear();

    REQUIRE(queue.count() == 0);

    char sender[32] = {};
    char message[822] = {};
    REQUIRE(queue.get(0, sender, sizeof(sender), message, sizeof(message))
            == false);
}

TEST_CASE("get truncates sender to fit caller buffer", "[m17][sms][queue]")
{
    SMSQueue queue;
    queue.push("LONGCALL9", "Test");

    // Use a small buffer: only 5 bytes (4 chars + NUL)
    char sender[5] = {};
    char message[822] = {};

    REQUIRE(queue.get(0, sender, sizeof(sender), message, sizeof(message))
            == true);
    REQUIRE(strcmp(sender, "LONG") == 0);
}
