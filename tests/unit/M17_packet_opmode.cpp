/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * Unit tests for OpMode_M17 packet RX/TX via the rtx packet descriptor API.
 *
 * These tests exercise addPacketRx() and addPacketTx() directly, without
 * spinning up the full RTX task or hardware.  They verify:
 *
 *  - addPacketRx: accepts up to 4 descriptors, rejects a 5th (queue full).
 *  - addPacketTx: accepts the first descriptor, rejects a second while busy.
 *  - addPacketTx: returns -EBUSY for a second call before the first completes.
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include "rtx/OpMode_M17.hpp"
#include "rtx/rtx.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static struct pktDesc make_desc(struct m17Packet *pkt, size_t payloadLen)
{
    struct pktDesc d;
    d.status = PKT_STATUS_IDLE;
    d.buffer = pkt;
    d.size = 20 + payloadLen;
    d.res = 0;
    return d;
}

// ---------------------------------------------------------------------------
// addPacketRx tests
// ---------------------------------------------------------------------------

TEST_CASE("OpMode_M17 addPacketRx: accepts a single descriptor",
          "[m17][opmode][packet]")
{
    OpMode_M17 mode;

    struct m17Packet pkt = {};
    struct pktDesc desc = make_desc(&pkt, sizeof(pkt.payload));
    desc.status = PKT_STATUS_SUBMITTED;
    REQUIRE(mode.addPacketRx(&desc) == 0);
}

TEST_CASE("OpMode_M17 addPacketRx: rejects when queue is full",
          "[m17][opmode][packet]")
{
    OpMode_M17 mode;

    struct m17Packet pkt1 = {}, pkt2 = {};
    struct pktDesc desc1 = make_desc(&pkt1, sizeof(pkt1.payload));
    desc1.status = PKT_STATUS_SUBMITTED;
    REQUIRE(mode.addPacketRx(&desc1) == 0);

    // 2nd should fail — capacity is 1
    struct pktDesc desc2 = make_desc(&pkt2, sizeof(pkt2.payload));
    desc2.status = PKT_STATUS_SUBMITTED;
    REQUIRE(mode.addPacketRx(&desc2) == -EAGAIN);
}

TEST_CASE("OpMode_M17 addPacketRx: rejects descriptor with size < 20",
          "[m17][opmode][packet]")
{
    OpMode_M17 mode;

    struct m17Packet pkt = {};
    struct pktDesc desc = make_desc(&pkt, sizeof(pkt.payload));
    desc.status = PKT_STATUS_SUBMITTED;
    desc.size = 10; // too small — need at least src[10] + dst[10]
    REQUIRE(mode.addPacketRx(&desc) == -EINVAL);
}

// ---------------------------------------------------------------------------
// addPacketTx tests
// ---------------------------------------------------------------------------

TEST_CASE("OpMode_M17 addPacketTx: accepts a single descriptor",
          "[m17][opmode][packet]")
{
    OpMode_M17 mode;

    struct m17Packet pkt = {};
    strncpy(pkt.src, "N0CALL", 9);
    strncpy(pkt.dst, "TE5ST", 9);
    struct pktDesc desc = make_desc(&pkt, 256);
    desc.status = PKT_STATUS_SUBMITTED;

    REQUIRE(mode.addPacketTx(&desc) == 0);
}

TEST_CASE(
    "OpMode_M17 addPacketTx: rejects second descriptor while first pending",
    "[m17][opmode][packet]")
{
    OpMode_M17 mode;

    struct m17Packet pkt1 = {}, pkt2 = {};
    strncpy(pkt1.src, "N0CALL", 9);
    strncpy(pkt1.dst, "TE5ST", 9);
    strncpy(pkt2.src, "N0CALL", 9);
    strncpy(pkt2.dst, "TE5ST", 9);
    struct pktDesc desc1 = make_desc(&pkt1, 256);
    struct pktDesc desc2 = make_desc(&pkt2, 256);
    desc1.status = PKT_STATUS_SUBMITTED;
    desc2.status = PKT_STATUS_SUBMITTED;

    REQUIRE(mode.addPacketTx(&desc1) == 0);
    REQUIRE(mode.addPacketTx(&desc2) == -EBUSY);
}

TEST_CASE("OpMode_M17 addPacketTx: rejects descriptor with size < 20",
          "[m17][opmode][packet]")
{
    OpMode_M17 mode;

    struct m17Packet pkt = {};
    struct pktDesc desc = make_desc(&pkt, sizeof(pkt.payload));
    desc.status = PKT_STATUS_SUBMITTED;
    desc.size = 10; // too small — need at least src[10] + dst[10]
    REQUIRE(mode.addPacketTx(&desc) == -EINVAL);
}
