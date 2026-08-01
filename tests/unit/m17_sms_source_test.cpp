/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * m17_sms_source_test.cpp — Unit tests for the M17 SMS message source.
 *
 * Exercises ops callbacks (count, get, supported_actions, invoke_action,
 * start_compose, send) without requiring a live RTX thread.
 * rtx_addPacketRx / rtx_addPacketTx are stubbed in m17_sms_stubs.cpp.
 * packet_io is compiled and linked against the real implementation.
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstring>

extern "C" {
#include "hwconfig.h"
#include "core/packet_io.h"
#include "rtx/rtx.h"
}
#include "core/m17_sms.h"
#include "core/messages.h"

#ifdef CONFIG_M17_SMS

/* Helper: remove all active entries between test cases. */
static void clean_sms(void)
{
    size_t n;
    while ((n = m17_sms_ops.count(NULL)) > 0) {
        struct message_header *h = m17_sms_ops.get(NULL, 0);
        if (h == nullptr)
            break;
        m17_sms_ops.invoke_action(h, MSG_ACTION_DELETE);
    }
}

TEST_CASE("m17_sms: initial state after init", "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();
    REQUIRE(m17_sms_ops.count(NULL) == 0);
}

TEST_CASE("m17_sms: send creates TX entry", "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();
    int ret = m17_sms_ops.send(NULL, "hello world", 11, "N0CALL");
    REQUIRE(ret == 0);
    REQUIRE(m17_sms_ops.count(NULL) == 1);

    struct message_header *hdr = m17_sms_ops.get(NULL, 0);
    REQUIRE(hdr != nullptr);
    REQUIRE(hdr->direction == MSG_DIR_TX);
    REQUIRE(hdr->unread == false);
    REQUIRE(hdr->body != nullptr);
    REQUIRE(hdr->body_len == 11);
    REQUIRE(strncmp(static_cast<const char *>(hdr->body), "hello world", 11)
            == 0);
}

TEST_CASE("m17_sms: send rejects null arguments", "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();
    REQUIRE(m17_sms_ops.send(NULL, nullptr, 0, "N0CALL") == -EINVAL);
    REQUIRE(m17_sms_ops.send(NULL, "hello", 5, nullptr) == -EINVAL);
    REQUIRE(m17_sms_ops.count(NULL) == 0);
}

TEST_CASE("m17_sms: send rejects empty recipient", "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();
    REQUIRE(m17_sms_ops.send(NULL, "hello", 5, "") == -EINVAL);
    REQUIRE(m17_sms_ops.count(NULL) == 0);
}

TEST_CASE("m17_sms: MSG_ACTION_MARK_READ clears unread flag", "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();
    m17_sms_ops.send(NULL, "test msg", 8, "N0CALL");

    struct message_header *hdr = m17_sms_ops.get(NULL, 0);
    REQUIRE(hdr != nullptr);
    hdr->unread = true; /* simulate unread */

    int ret = m17_sms_ops.invoke_action(hdr, MSG_ACTION_MARK_READ);
    REQUIRE(ret == 0);
    REQUIRE(hdr->unread == false);
}

TEST_CASE("m17_sms: MSG_ACTION_MARK_UNREAD sets unread flag", "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();
    m17_sms_ops.send(NULL, "test msg", 8, "N0CALL");

    struct message_header *hdr = m17_sms_ops.get(NULL, 0);
    REQUIRE(hdr != nullptr);
    hdr->unread = false;

    int ret = m17_sms_ops.invoke_action(hdr, MSG_ACTION_MARK_UNREAD);
    REQUIRE(ret == 0);
    REQUIRE(hdr->unread == true);
}

TEST_CASE("m17_sms: MSG_ACTION_DELETE removes entry", "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();
    m17_sms_ops.send(NULL, "bye", 3, "N0CALL");
    REQUIRE(m17_sms_ops.count(NULL) == 1);

    struct message_header *hdr = m17_sms_ops.get(NULL, 0);
    REQUIRE(hdr != nullptr);

    int ret = m17_sms_ops.invoke_action(hdr, MSG_ACTION_DELETE);
    REQUIRE(ret == 0);
    REQUIRE(m17_sms_ops.count(NULL) == 0);
    REQUIRE(m17_sms_ops.get(NULL, 0) == nullptr);
}

TEST_CASE("m17_sms: MSG_ACTION_REPLY returns success", "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();
    m17_sms_ops.send(NULL, "test", 4, "N0CALL");

    struct message_header *hdr = m17_sms_ops.get(NULL, 0);
    REQUIRE(hdr != nullptr);

    /* Simulate an RX entry so REPLY action is offered. */
    hdr->direction = MSG_DIR_RX;
    strncpy(hdr->sender, "W1AW", sizeof(hdr->sender) - 1);
    hdr->sender[sizeof(hdr->sender) - 1] = '\0';

    /* REPLY no longer sets compose state; the UI reads hdr->sender directly. */
    int ret = m17_sms_ops.invoke_action(hdr, MSG_ACTION_REPLY);
    REQUIRE(ret == 0);
    /* Verify sender is accessible (the UI will copy it). */
    REQUIRE(strcmp(hdr->sender, "W1AW") == 0);
}

TEST_CASE("m17_sms: start_compose is a no-op (compose driven by UI)",
          "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();
    /* start_compose should not crash; it is a no-op placeholder. */
    m17_sms_ops.start_compose(NULL);
    REQUIRE(m17_sms_ops.count(NULL) == 0);
}

TEST_CASE("m17_sms: multiple entries indexed correctly", "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();
    m17_sms_ops.send(NULL, "first", 5, "N0CALL");
    packet_io_init(); /* clear single-slot TX queue so next send succeeds */
    m17_sms_ops.send(NULL, "second", 6, "N0CALL");
    packet_io_init();
    m17_sms_ops.send(NULL, "third", 5, "N0CALL");

    REQUIRE(m17_sms_ops.count(NULL) == 3);
    REQUIRE(m17_sms_ops.get(NULL, 0) != nullptr);
    REQUIRE(m17_sms_ops.get(NULL, 1) != nullptr);
    REQUIRE(m17_sms_ops.get(NULL, 2) != nullptr);
    REQUIRE(m17_sms_ops.get(NULL, 3) == nullptr);

    /* Delete the middle entry and verify index compaction. */
    struct message_header *mid = m17_sms_ops.get(NULL, 1);
    REQUIRE(strncmp(static_cast<const char *>(mid->body), "second", 6) == 0);
    m17_sms_ops.invoke_action(mid, MSG_ACTION_DELETE);

    REQUIRE(m17_sms_ops.count(NULL) == 2);
    REQUIRE(m17_sms_ops.get(NULL, 2) == nullptr);
}

TEST_CASE("m17_sms: count() stays consistent with get() across eviction",
          "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();

    /* Send well past capacity so both the slot-eviction (alloc_slot) and
     * pool-eviction (pool_alloc) paths run repeatedly.  The maintained
     * active_count must never diverge from the number of entries actually
     * reachable through get(). */
    for (int i = 0; i < M17_SMS_MAX_MESSAGES * 2 + 5; i++) {
        char body[16];
        int n = snprintf(body, sizeof(body), "msg%d", i);
        m17_sms_ops.send(NULL, body, (size_t)n, "N0CALL");
        packet_io_init(); /* clear single-slot TX queue for the next send */

        /* Independently count reachable entries and compare to count(). */
        size_t scanned = 0;
        while (m17_sms_ops.get(NULL, scanned) != nullptr)
            scanned++;
        REQUIRE(m17_sms_ops.count(NULL) == scanned);
    }

    /* At steady state the inbox is full, not unbounded. */
    REQUIRE(m17_sms_ops.count(NULL) == (size_t)M17_SMS_MAX_MESSAGES);

    /* Deleting entries keeps the counter exactly in step. */
    while (m17_sms_ops.count(NULL) > 0) {
        size_t before = m17_sms_ops.count(NULL);
        struct message_header *h = m17_sms_ops.get(NULL, 0);
        REQUIRE(h != nullptr);
        m17_sms_ops.invoke_action(h, MSG_ACTION_DELETE);
        REQUIRE(m17_sms_ops.count(NULL) == before - 1);
    }
    REQUIRE(m17_sms_ops.count(NULL) == 0);
}

TEST_CASE("m17_sms: supported_actions includes REPLY only for RX", "[m17_sms]")
{
    packet_io_init();
    m17_sms_init();
    m17_sms_ops.send(NULL, "tx msg", 6, "N0CALL");

    struct message_header *hdr = m17_sms_ops.get(NULL, 0);
    REQUIRE(hdr != nullptr);

    /* TX entry: no REPLY */
    hdr->direction = MSG_DIR_TX;
    uint32_t acts_tx = m17_sms_ops.supported_actions(hdr);
    REQUIRE((acts_tx & MSG_ACTION_VIEW) != 0);
    REQUIRE((acts_tx & MSG_ACTION_DELETE) != 0);
    REQUIRE((acts_tx & MSG_ACTION_REPLY) == 0);

    /* RX entry: REPLY offered */
    hdr->direction = MSG_DIR_RX;
    uint32_t acts_rx = m17_sms_ops.supported_actions(hdr);
    REQUIRE((acts_rx & MSG_ACTION_REPLY) != 0);
}

TEST_CASE("m17_sms: ops metadata is correct", "[m17_sms]")
{
    REQUIRE(m17_sms_ops.name != nullptr);
    REQUIRE(strcmp(m17_sms_ops.name, "M17 SMS") == 0);
    REQUIRE(m17_sms_ops.mode_id == (uint8_t)OPMODE_M17);
    REQUIRE(m17_sms_ops.count != nullptr);
    REQUIRE(m17_sms_ops.get != nullptr);
    REQUIRE(m17_sms_ops.supported_actions != nullptr);
    REQUIRE(m17_sms_ops.invoke_action != nullptr);
    REQUIRE(m17_sms_ops.start_compose != nullptr);
    REQUIRE(m17_sms_ops.tick != nullptr);
    REQUIRE(m17_sms_ops.send != nullptr);
}

#else  /* !CONFIG_M17_SMS */

TEST_CASE("m17_sms: skipped — CONFIG_M17_SMS not defined", "[m17_sms]")
{
    /* Nothing to test when M17 SMS is disabled. */
    SUCCEED();
}

#endif /* CONFIG_M17_SMS */
