/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include "rtx/PktBuf.hpp"

static rtxPacket_t make_pkt(uint8_t fill, uint16_t len, uint8_t proto)
{
    rtxPacket_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    memset(pkt.data, fill, len);
    pkt.len      = len;
    pkt.rssi     = -60;
    pkt.protocol = proto;
    return pkt;
}

TEST_CASE("Empty queue returns no packet", "[pktbuf]")
{
    PktBuf q;
    rtxPacket_t out;
    REQUIRE(q.pending() == 0);
    REQUIRE(q.pop(&out) == false);
}

TEST_CASE("Push then pop returns same data", "[pktbuf]")
{
    PktBuf q;
    rtxPacket_t pkt = make_pkt(0xAB, 100, PKT_PROTO_M17);
    pkt.timestamp = 12345;

    REQUIRE(q.push(&pkt) == true);
    REQUIRE(q.pending() == 1);

    rtxPacket_t out;
    REQUIRE(q.pop(&out) == true);
    REQUIRE(q.pending() == 0);

    REQUIRE(out.len == 100);
    REQUIRE(out.rssi == -60);
    REQUIRE(out.timestamp == 12345);
    REQUIRE(out.protocol == PKT_PROTO_M17);
    REQUIRE(out.data[0] == 0xAB);
    REQUIRE(out.data[99] == 0xAB);
    REQUIRE(out.data[100] == 0);
}

TEST_CASE("FIFO ordering is preserved", "[pktbuf]")
{
    PktBuf q;

    for (uint8_t i = 0; i < PKTBUF_SLOTS; i++)
    {
        rtxPacket_t pkt = make_pkt(i, 10, PKT_PROTO_APRS);
        REQUIRE(q.push(&pkt) == true);
    }

    REQUIRE(q.pending() == PKTBUF_SLOTS);

    for (uint8_t i = 0; i < PKTBUF_SLOTS; i++)
    {
        rtxPacket_t out;
        REQUIRE(q.pop(&out) == true);
        REQUIRE(out.data[0] == i);
    }

    REQUIRE(q.pending() == 0);
}

TEST_CASE("Overflow drops oldest packet", "[pktbuf]")
{
    PktBuf q;

    /* Fill the queue with packets 0..PKTBUF_SLOTS-1 */
    for (uint8_t i = 0; i < PKTBUF_SLOTS; i++)
    {
        rtxPacket_t pkt = make_pkt(i, 10, PKT_PROTO_M17);
        REQUIRE(q.push(&pkt) == true);
    }

    /* Push one more — oldest (fill=0) should be dropped */
    rtxPacket_t extra = make_pkt(0xFF, 10, PKT_PROTO_M17);
    REQUIRE(q.push(&extra) == false);
    REQUIRE(q.pending() == PKTBUF_SLOTS);

    /* First pop should return fill=1 (second-oldest) */
    rtxPacket_t out;
    REQUIRE(q.pop(&out) == true);
    REQUIRE(out.data[0] == 1);

    /* Last pop should return the extra 0xFF packet */
    for (size_t i = 0; i < PKTBUF_SLOTS - 2; i++)
        q.pop(&out);

    REQUIRE(q.pop(&out) == true);
    REQUIRE(out.data[0] == 0xFF);
    REQUIRE(q.pending() == 0);
}

TEST_CASE("clear() empties the queue", "[pktbuf]")
{
    PktBuf q;

    for (uint8_t i = 0; i < 3; i++)
    {
        rtxPacket_t pkt = make_pkt(i, 5, PKT_PROTO_M17);
        q.push(&pkt);
    }

    REQUIRE(q.pending() == 3);
    q.clear();
    REQUIRE(q.pending() == 0);

    rtxPacket_t out;
    REQUIRE(q.pop(&out) == false);
}

TEST_CASE("Queue works after wrap-around", "[pktbuf]")
{
    PktBuf q;

    /* Push and pop a few times to advance head/tail past the end */
    for (int round = 0; round < 3; round++)
    {
        for (uint8_t i = 0; i < PKTBUF_SLOTS; i++)
        {
            rtxPacket_t pkt = make_pkt(
                static_cast<uint8_t>(round * PKTBUF_SLOTS + i), 10,
                PKT_PROTO_APRS);
            q.push(&pkt);
        }

        for (uint8_t i = 0; i < PKTBUF_SLOTS; i++)
        {
            rtxPacket_t out;
            REQUIRE(q.pop(&out) == true);
            REQUIRE(out.data[0] ==
                    static_cast<uint8_t>(round * PKTBUF_SLOTS + i));
        }

        REQUIRE(q.pending() == 0);
    }
}
