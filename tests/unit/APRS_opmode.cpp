/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * Contract tests for the APRS operating mode's packet interface, the part
 * that can be checked without a radio, an audio device or the rtx thread.
 */

#include <catch2/catch_test_macros.hpp>

#include <cerrno>

#include "rtx/OpMode_APRS.hpp"
#include "rtx/rtx.h"

TEST_CASE("OpMode_APRS: the mode identifier is APRS", "[aprs][opmode]")
{
    OpMode_APRS mode;
    REQUIRE(mode.getID() == OPMODE_APRS);
}

TEST_CASE("OpMode_APRS disable: hands back queued receive descriptors",
          "[aprs][opmode]")
{
    OpMode_APRS mode;
    uint8_t bufA[APRS_PACLEN], bufB[APRS_PACLEN];
    struct pktDesc a = { PKT_STATUS_SUBMITTED, bufA, sizeof(bufA), 0 };
    struct pktDesc b = { PKT_STATUS_SUBMITTED, bufB, sizeof(bufB), 0 };

    mode.enable();
    REQUIRE(mode.addPacketRx(&a) == 0);
    REQUIRE(mode.addPacketRx(&b) == 0);

    /* Leaving the mode must not strand them: the next mode never sees them,
     * so only a handed-back descriptor can be resubmitted. */
    mode.disable();

    CHECK(a.status == PKT_STATUS_ERROR);
    CHECK(a.res == -ECANCELED);
    CHECK(b.status == PKT_STATUS_ERROR);
    CHECK(b.res == -ECANCELED);

    /* The queue is empty again */
    a.status = PKT_STATUS_SUBMITTED;
    b.status = PKT_STATUS_SUBMITTED;
    REQUIRE(mode.addPacketRx(&a) == 0);
    REQUIRE(mode.addPacketRx(&b) == 0);
}

TEST_CASE("OpMode_APRS enable: hands back descriptors queued while off",
          "[aprs][opmode]")
{
    OpMode_APRS mode;
    uint8_t buf[APRS_PACLEN];
    struct pktDesc desc = { PKT_STATUS_SUBMITTED, buf, sizeof(buf), 0 };

    REQUIRE(mode.addPacketRx(&desc) == 0);
    mode.enable();

    CHECK(desc.status == PKT_STATUS_ERROR);
    CHECK(desc.res == -ECANCELED);
    mode.disable();
}
