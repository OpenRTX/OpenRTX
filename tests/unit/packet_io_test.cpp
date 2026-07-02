/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include "core/packet_io.h"

/* ------------------------------------------------------------------ */
/* TX request queue (UI -> RTX)                                        */
/* ------------------------------------------------------------------ */

TEST_CASE("packet_io: TX request enqueue/dequeue round trip",
          "[messages][packet_io]")
{
    packet_io_init();

    struct pkt_tx_request req = {};
    req.mode_id = 7;
    strncpy(req.dst, "KD0OSS", sizeof(req.dst));
    strncpy(req.body, "hello there", sizeof(req.body));
    req.body_len = strlen(req.body);
    req.tag = 42;

    REQUIRE(packet_io_enqueue_tx(&req) == true);

    struct pkt_tx_request out = {};
    REQUIRE(packet_io_dequeue_tx(&out) == true);
    REQUIRE(out.mode_id == 7);
    REQUIRE(strcmp(out.dst, "KD0OSS") == 0);
    REQUIRE(strcmp(out.body, "hello there") == 0);
    REQUIRE(out.body_len == req.body_len);
    REQUIRE(out.tag == 42);

    /* Slot is now empty. */
    struct pkt_tx_request empty = {};
    REQUIRE(packet_io_dequeue_tx(&empty) == false);
}

TEST_CASE("packet_io: TX request enqueue rejects when slot is full",
          "[messages][packet_io]")
{
    packet_io_init();

    struct pkt_tx_request req1 = {};
    req1.tag = 1;
    struct pkt_tx_request req2 = {};
    req2.tag = 2;

    REQUIRE(packet_io_enqueue_tx(&req1) == true);
    /* Second enqueue must fail: slot is still occupied by req1. */
    REQUIRE(packet_io_enqueue_tx(&req2) == false);

    struct pkt_tx_request out = {};
    REQUIRE(packet_io_dequeue_tx(&out) == true);
    REQUIRE(out.tag == 1); /* req1, not req2 — req2 was dropped */

    /* Slot is free again. */
    REQUIRE(packet_io_enqueue_tx(&req2) == true);
    REQUIRE(packet_io_dequeue_tx(&out) == true);
    REQUIRE(out.tag == 2);
}

/* ------------------------------------------------------------------ */
/* RX event queue (RTX -> UI)                                          */
/* ------------------------------------------------------------------ */

TEST_CASE("packet_io: RX event enqueue/dequeue round trip",
          "[messages][packet_io]")
{
    packet_io_init();

    struct pkt_rx_event evt = {};
    evt.mode_id = 3;
    strncpy(evt.src, "W1AW", sizeof(evt.src));
    strncpy(evt.body, "incoming message", sizeof(evt.body));
    evt.body_len = strlen(evt.body);

    REQUIRE(packet_io_enqueue_rx(&evt) == true);

    struct pkt_rx_event out = {};
    REQUIRE(packet_io_dequeue_rx(&out) == true);
    REQUIRE(out.mode_id == 3);
    REQUIRE(strcmp(out.src, "W1AW") == 0);
    REQUIRE(strcmp(out.body, "incoming message") == 0);
    REQUIRE(out.body_len == evt.body_len);

    REQUIRE(packet_io_dequeue_rx(&out) == false);
}

TEST_CASE("packet_io: RX event enqueue rejects when slot is full",
          "[messages][packet_io]")
{
    packet_io_init();

    struct pkt_rx_event evt1 = {};
    strncpy(evt1.src, "AAA", sizeof(evt1.src));
    struct pkt_rx_event evt2 = {};
    strncpy(evt2.src, "BBB", sizeof(evt2.src));

    REQUIRE(packet_io_enqueue_rx(&evt1) == true);
    REQUIRE(packet_io_enqueue_rx(&evt2) == false); /* dropped */

    struct pkt_rx_event out = {};
    REQUIRE(packet_io_dequeue_rx(&out) == true);
    REQUIRE(strcmp(out.src, "AAA") == 0);
}

/* ------------------------------------------------------------------ */
/* TX completion queue (RTX -> UI)                                     */
/* ------------------------------------------------------------------ */

TEST_CASE("packet_io: TX completion enqueue/dequeue round trip",
          "[messages][packet_io]")
{
    packet_io_init();

    struct pkt_tx_done done = {};
    done.mode_id = 5;
    done.tag = 99;
    done.status = 2; /* MSG_STATUS_SENT */

    REQUIRE(packet_io_enqueue_tx_done(&done) == true);

    struct pkt_tx_done out = {};
    REQUIRE(packet_io_dequeue_tx_done(&out) == true);
    REQUIRE(out.mode_id == 5);
    REQUIRE(out.tag == 99);
    REQUIRE(out.status == 2);

    REQUIRE(packet_io_dequeue_tx_done(&out) == false);
}

TEST_CASE("packet_io: TX completion enqueue rejects when slot is full",
          "[messages][packet_io]")
{
    packet_io_init();

    struct pkt_tx_done done1 = {};
    done1.tag = 1;
    struct pkt_tx_done done2 = {};
    done2.tag = 2;

    REQUIRE(packet_io_enqueue_tx_done(&done1) == true);
    REQUIRE(packet_io_enqueue_tx_done(&done2) == false); /* dropped */

    struct pkt_tx_done out = {};
    REQUIRE(packet_io_dequeue_tx_done(&out) == true);
    REQUIRE(out.tag == 1);
}

/* ------------------------------------------------------------------ */
/* Queue independence — the whole point of Decision 12                 */
/* ------------------------------------------------------------------ */

TEST_CASE("packet_io: RX and TX-completion queues do not block each other",
          "[messages][packet_io]")
{
    packet_io_init();

    struct pkt_rx_event evt = {};
    strncpy(evt.src, "RX_SENDER", sizeof(evt.src));

    struct pkt_tx_done done = {};
    done.tag = 123;
    done.status = 3; /* MSG_STATUS_FAILED */

    /* Fill the RX slot; the completion slot must still accept a post. */
    REQUIRE(packet_io_enqueue_rx(&evt) == true);
    REQUIRE(packet_io_enqueue_tx_done(&done) == true);

    /* Both are retrievable independently, order does not matter. */
    struct pkt_tx_done done_out = {};
    REQUIRE(packet_io_dequeue_tx_done(&done_out) == true);
    REQUIRE(done_out.tag == 123);

    struct pkt_rx_event evt_out = {};
    REQUIRE(packet_io_dequeue_rx(&evt_out) == true);
    REQUIRE(strcmp(evt_out.src, "RX_SENDER") == 0);
}

TEST_CASE("packet_io: TX request queue is independent of RX/completion queues",
          "[messages][packet_io]")
{
    packet_io_init();

    struct pkt_tx_request req = {};
    req.tag = 7;
    struct pkt_rx_event evt = {};
    struct pkt_tx_done done = {};

    /* Fill all three simultaneously; none should block the others. */
    REQUIRE(packet_io_enqueue_tx(&req) == true);
    REQUIRE(packet_io_enqueue_rx(&evt) == true);
    REQUIRE(packet_io_enqueue_tx_done(&done) == true);

    struct pkt_tx_request req_out = {};
    struct pkt_rx_event evt_out = {};
    struct pkt_tx_done done_out = {};
    REQUIRE(packet_io_dequeue_tx(&req_out) == true);
    REQUIRE(packet_io_dequeue_rx(&evt_out) == true);
    REQUIRE(packet_io_dequeue_tx_done(&done_out) == true);
    REQUIRE(req_out.tag == 7);
}

/* ------------------------------------------------------------------ */
/* packet_io_init() resets pending state                               */
/* ------------------------------------------------------------------ */

TEST_CASE("packet_io: init() clears any previously pending slots",
          "[messages][packet_io]")
{
    packet_io_init();

    struct pkt_tx_request req = {};
    struct pkt_rx_event evt = {};
    struct pkt_tx_done done = {};
    REQUIRE(packet_io_enqueue_tx(&req) == true);
    REQUIRE(packet_io_enqueue_rx(&evt) == true);
    REQUIRE(packet_io_enqueue_tx_done(&done) == true);

    /* Re-init must clear all pending flags, as if freshly started. */
    packet_io_init();

    struct pkt_tx_request req_out = {};
    struct pkt_rx_event evt_out = {};
    struct pkt_tx_done done_out = {};
    REQUIRE(packet_io_dequeue_tx(&req_out) == false);
    REQUIRE(packet_io_dequeue_rx(&evt_out) == false);
    REQUIRE(packet_io_dequeue_tx_done(&done_out) == false);
}
