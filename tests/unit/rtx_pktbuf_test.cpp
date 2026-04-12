/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <pthread.h>
#include "rtx/PktBuf.hpp"

extern "C"
{
#include "rtx/rtx.h"
}

static pthread_mutex_t testMutex = PTHREAD_MUTEX_INITIALIZER;

TEST_CASE("RX queue is empty after init", "[rtx][packet]")
{
    rtx_init(&testMutex);

    REQUIRE(rtx_rxPending() == 0);

    rtxPacket_t pkt;
    REQUIRE(rtx_recvPacket(&pkt) == false);

    rtx_terminate();
}

TEST_CASE("sendPacket enqueues a packet", "[rtx][packet]")
{
    rtx_init(&testMutex);

    rtxPacket_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    memcpy(pkt.data, "test", 4);
    pkt.len      = 4;
    pkt.protocol = PKT_PROTO_M17;

    REQUIRE(rtx_sendPacket(&pkt) == true);

    rtx_terminate();
}

TEST_CASE("sendPacket fills queue to capacity", "[rtx][packet]")
{
    rtx_init(&testMutex);

    for (unsigned int i = 0; i < PKTBUF_SLOTS; i++)
    {
        rtxPacket_t pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.data[0] = static_cast<uint8_t>(i);
        pkt.len     = 1;
        REQUIRE(rtx_sendPacket(&pkt) == true);
    }

    /* Queue is full — next push overwrites oldest */
    rtxPacket_t extra;
    memset(&extra, 0, sizeof(extra));
    extra.data[0] = 0xFF;
    extra.len     = 1;
    REQUIRE(rtx_sendPacket(&extra) == false);

    rtx_terminate();
}

TEST_CASE("rxPending reflects queue state", "[rtx][packet]")
{
    rtx_init(&testMutex);

    REQUIRE(rtx_rxPending() == 0);

    /* RX queue is only populated by OpMode handlers, so it stays empty
     * when no mode is actively receiving. */
    rtx_task();
    REQUIRE(rtx_rxPending() == 0);

    rtx_terminate();
}
