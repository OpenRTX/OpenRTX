/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include <cerrno>
#include <cstring>

#include "aprs_test_frame.h"
#include "core/aprs_msg.h"
#include "core/messages.h"
#include "core/state.h"
#include "protocols/APRS/packet.h"
#include "rtx_packet_stub.h"

/* The source matches received messages against the APRS callsign. */
state_t state;

/* Time as the source sees it, in milliseconds. */
static long long now;

extern "C" long long getTick()
{
    return now;
}

/**
 * Reset the inbox and activate it for APRS, as the UI thread would, with
 * this station's callsign set.
 */
static void setup(const char *callsign)
{
    rtxStub_reset();
    messages_init();
    REQUIRE(messages_registerSource(&aprs_msg_ops) == 0);
    messages_task(OPMODE_APRS);

    // Past every repeat window, so earlier tests leave nothing behind
    now += 24LL * 60 * 60 * 1000;

    memset(state.settings.aprs_callsign, 0,
           sizeof(state.settings.aprs_callsign));
    strncpy(state.settings.aprs_callsign, callsign,
            sizeof(state.settings.aprs_callsign) - 1);
}

/**
 * Build a UI frame from src to APRS carrying info.
 */
static struct aprsTestFrame makeFrame(const char *src, const char *info)
{
    return aprsTest_makeFrame("APRS", 0, src, 0, info);
}

/**
 * Hand a received frame to the source, as the inbox does.
 */
static int receive(struct aprsTestFrame &frame)
{
    struct pktDesc desc = {};

    desc.status = PKT_STATUS_DONE;
    desc.buffer = frame.data;
    desc.size = sizeof(frame.data);
    desc.res = (ssize_t)frame.len;

    return aprs_msg_ops.processRx(&desc);
}

TEST_CASE("aprs_msg: source serves the APRS opmode", "[aprs][messages]")
{
    CHECK(aprs_msg_ops.mode == OPMODE_APRS);
    CHECK(strcmp(aprs_msg_ops.name, "APRS") == 0);
    CHECK(aprs_msg_ops.broadcastAddr == nullptr);
}

TEST_CASE("aprs_msg: a message for this station is filed in the inbox",
          "[aprs][messages]")
{
    setup("N0CALL");

    struct aprsTestFrame frame = makeFrame("W1AW", ":N0CALL   :Hello{01");
    REQUIRE(receive(frame) == 0);
    REQUIRE(messages_count() == 1);

    const struct message *msg = messages_get(0);
    CHECK(strcmp(msg->body, "Hello") == 0);
    CHECK(strcmp(msg->sender, "W1AW") == 0);
    CHECK(strcmp(msg->recipient, "N0CALL") == 0);
    CHECK(msg->mode == OPMODE_APRS);
    CHECK(msg->direction == MSG_DIR_RX);
    CHECK(msg->unread != 0);

    messages_terminate();
}

TEST_CASE("aprs_msg: the addressee is matched as an address",
          "[aprs][messages]")
{
    setup("N0CALL-7");

    SECTION("written in lower case with a leading zero")
    {
        struct aprsTestFrame frame = makeFrame("W1AW", ":n0call-07:Hello");
        REQUIRE(receive(frame) == 0);
        CHECK(messages_count() == 1);
    }

    SECTION("another SSID of the same callsign is another station")
    {
        struct aprsTestFrame frame = makeFrame("W1AW", ":N0CALL-9 :Hello");
        REQUIRE(receive(frame) == 0);
        CHECK(messages_count() == 0);
    }

    SECTION("the bare callsign is SSID 0, another station")
    {
        struct aprsTestFrame frame = makeFrame("W1AW", ":N0CALL   :Hello");
        REQUIRE(receive(frame) == 0);
        CHECK(messages_count() == 0);
    }

    messages_terminate();
}

TEST_CASE("aprs_msg: copies of a message are stored once", "[aprs][messages]")
{
    setup("N0CALL");

    SECTION("a numbered message repeated until acknowledged")
    {
        struct aprsTestFrame frame = makeFrame("W1AW", ":N0CALL   :Hello{01");
        REQUIRE(receive(frame) == 0);
        now += 10 * 60 * 1000;
        REQUIRE(receive(frame) == 0);
        CHECK(messages_count() == 1);
    }

    SECTION("a plain message relayed by a digipeater")
    {
        struct aprsTestFrame frame = makeFrame("W1AW", ":N0CALL   :Hello");
        REQUIRE(receive(frame) == 0);
        now += 5 * 1000;
        REQUIRE(receive(frame) == 0);
        CHECK(messages_count() == 1);
    }

    SECTION("the same text sent again later is a new message")
    {
        struct aprsTestFrame frame = makeFrame("W1AW", ":N0CALL   :Hello");
        REQUIRE(receive(frame) == 0);
        now += 60 * 1000;
        REQUIRE(receive(frame) == 0);
        CHECK(messages_count() == 2);
    }

    SECTION("another message number is another message")
    {
        struct aprsTestFrame first = makeFrame("W1AW", ":N0CALL   :Hello{01");
        struct aprsTestFrame second = makeFrame("W1AW", ":N0CALL   :Hello{02");
        REQUIRE(receive(first) == 0);
        REQUIRE(receive(second) == 0);
        CHECK(messages_count() == 2);
    }

    messages_terminate();
}

TEST_CASE("aprs_msg: other traffic is decoded but not stored",
          "[aprs][messages]")
{
    setup("N0CALL");

    SECTION("a position report")
    {
        struct aprsTestFrame frame = makeFrame("W1AW", "!4903.50N/07201.75W-");
        REQUIRE(receive(frame) == 0);
    }

    SECTION("a message for another station")
    {
        struct aprsTestFrame frame = makeFrame("W1AW", ":K1ABC    :not for us");
        REQUIRE(receive(frame) == 0);
    }

    CHECK(messages_count() == 0);
    messages_terminate();
}

TEST_CASE("aprs_msg: a malformed frame is refused", "[aprs][messages]")
{
    setup("N0CALL");

    struct aprsTestFrame frame = makeFrame("W1AW", ":N0CALL   :Hello");
    frame.len = 10;

    CHECK(receive(frame) == -EINVAL);
    CHECK(messages_count() == 0);
    messages_terminate();
}
