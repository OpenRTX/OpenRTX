/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * Contract tests for OpMode_APRS::addPacketTx().
 *
 * Submission is the boundary between whatever wants to transmit and the
 * operating mode that does it, and it is the part that can be checked
 * without a radio, an audio device, or the RTX task. What matters here is
 * that a caller is told the truth: a malformed request is refused rather
 * than transmitted as garbage, and a second request while one is in flight
 * is refused rather than silently replacing the first — the single slot is
 * claimed with a compare-and-exchange precisely so a UI thread and the RTX
 * thread cannot both believe they own it.
 */

#include <catch2/catch_test_macros.hpp>

#include "protocols/APRS/packet.h"
#include "rtx/OpMode_APRS.hpp"
#include "rtx/rtx.h"

#include <cerrno>
#include <cstring>

namespace
{

/** A descriptor carrying a well-formed frame, ready to submit. */
struct pktDesc makeDesc(uint8_t *buf, size_t cap)
{
    const char info[] = ":W1AW     :hello";
    size_t len = aprsFrameBuild(buf, cap, APRS_TOCALL, "N0CALL-7",
                                APRS_DEFAULT_PATH, info, strlen(info));
    REQUIRE(len > 0);

    struct pktDesc desc;
    desc.status = PKT_STATUS_SUBMITTED;
    desc.buffer = buf;
    desc.size = len;
    desc.res = 0;
    return desc;
}

} // namespace

TEST_CASE("OpMode_APRS addPacketTx: a well-formed frame is accepted",
          "[aprs][opmode]")
{
    OpMode_APRS mode;
    uint8_t buf[APRS_PACLEN];
    struct pktDesc desc = makeDesc(buf, sizeof(buf));

    REQUIRE(mode.addPacketTx(&desc) == 0);
}

TEST_CASE("OpMode_APRS addPacketTx: only one transmission is in flight",
          "[aprs][opmode]")
{
    OpMode_APRS mode;
    uint8_t bufA[APRS_PACLEN];
    uint8_t bufB[APRS_PACLEN];
    struct pktDesc first = makeDesc(bufA, sizeof(bufA));
    struct pktDesc second = makeDesc(bufB, sizeof(bufB));

    REQUIRE(mode.addPacketTx(&first) == 0);

    /* Refused, not queued and not substituted. */
    REQUIRE(mode.addPacketTx(&second) == -EBUSY);
}

TEST_CASE("OpMode_APRS addPacketTx: malformed requests are refused",
          "[aprs][opmode]")
{
    OpMode_APRS mode;
    uint8_t buf[APRS_PACLEN];
    struct pktDesc desc = makeDesc(buf, sizeof(buf));

    SECTION("no descriptor at all")
    {
        REQUIRE(mode.addPacketTx(nullptr) == -EINVAL);
    }

    SECTION("no buffer")
    {
        desc.buffer = nullptr;
        REQUIRE(mode.addPacketTx(&desc) == -EINVAL);
    }

    SECTION("an empty frame")
    {
        desc.size = 0;
        REQUIRE(mode.addPacketTx(&desc) == -EINVAL);
    }

    SECTION("a frame longer than AX.25 allows")
    {
        desc.size = APRS_PACLEN + 1;
        REQUIRE(mode.addPacketTx(&desc) == -EINVAL);
    }

    /* A refused request must not have claimed the slot. */
    uint8_t good[APRS_PACLEN];
    struct pktDesc valid = makeDesc(good, sizeof(good));
    REQUIRE(mode.addPacketTx(&valid) == 0);
}

TEST_CASE("OpMode_APRS: the mode identifier is APRS", "[aprs][opmode]")
{
    OpMode_APRS mode;
    REQUIRE(mode.getID() == OPMODE_APRS);
}
