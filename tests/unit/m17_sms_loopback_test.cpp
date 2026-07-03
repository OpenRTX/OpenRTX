/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * M17 SMS loopback integration test.
 *
 * Exercises the full RX and TX paths of m17_sms by:
 *   - Using loopback stubs (m17_sms_loopback_stubs.cpp) that expose the live
 *     pktDesc pointer via loopback_get_rx_desc().
 *   - Injecting formatted SMS payloads directly into the RX buffer and
 *     marking the descriptor PKT_STATUS_DONE, then calling m17_sms_task_rtx()
 *     to drive reception through the same code path used on real hardware.
 *   - Draining packet_io events into entries[] by calling ops->tick().
 *   - Verifying TX entries are advanced to MSG_STATUS_SENT after a task tick.
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>

extern "C" {
#include "hwconfig.h"
#include "core/packet_io.h"
#include "protocols/M17/m17.h"
}
#include "core/m17_sms.h"
#include "protocols/M17/SmsPacket.hpp"
#include "rtx/rtx.h"

#ifdef CONFIG_M17_SMS

extern "C" struct pktDesc *loopback_get_rx_desc(void);
extern "C" struct pktDesc *loopback_get_tx_desc(void);
extern "C" const char *loopback_get_tx_destination(void);

static bool loopback_inject(const char *text, size_t len)
{
    struct pktDesc *desc = loopback_get_rx_desc();
    if (desc == nullptr || desc->buffer == nullptr)
        return false;

    struct m17Packet *pkt = static_cast<struct m17Packet *>(desc->buffer);
    size_t written = M17::sms_format_packet(text, len, pkt->payload,
                                            sizeof(pkt->payload));
    if (written == 0)
        return false;

    const rtxStatus_t *st = rtx_getStatus();
    strncpy(pkt->src, st->source_address, sizeof(pkt->src) - 1);
    pkt->src[sizeof(pkt->src) - 1] = '\0';

    desc->res = static_cast<ssize_t>(written);
    desc->status = PKT_STATUS_DONE;
    return true;
}

TEST_CASE("m17_sms loopback: TX send is promoted to SENT after task tick",
          "[m17_sms][loopback]")
{
    packet_io_init();
    m17_sms_init();
    m17_sms_task_rtx();

    REQUIRE(m17_sms_ops.send(NULL, "hello", 5, "N0CALL") == 0);
    REQUIRE(m17_sms_ops.count(NULL) == 1);

    m17_sms_task_rtx();
    m17_sms_ops.tick(NULL);

    struct message_header *hdr = m17_sms_ops.get(NULL, 0);
    REQUIRE(hdr != nullptr);
    REQUIRE(hdr->direction == MSG_DIR_TX);
    REQUIRE(hdr->status == MSG_STATUS_SENT);
    REQUIRE(strcmp(static_cast<const char *>(hdr->body), "hello") == 0);
}

TEST_CASE("m17_sms loopback: TX destination is embedded in m17Packet buffer",
          "[m17_sms][loopback]")
{
    packet_io_init();
    m17_sms_init();
    m17_sms_task_rtx();

    REQUIRE(m17_sms_ops.send(NULL, "hi", 2, "W1AW") == 0);
    m17_sms_task_rtx();

    REQUIRE(strcmp(loopback_get_tx_destination(), "W1AW") == 0);
}

TEST_CASE("m17_sms loopback: injected RX packet creates inbox entry",
          "[m17_sms][loopback]")
{
    packet_io_init();
    m17_sms_init();
    m17_sms_task_rtx();

    REQUIRE(loopback_inject("world", 5) == true);
    m17_sms_task_rtx();
    m17_sms_ops.tick(NULL);

    REQUIRE(m17_sms_ops.count(NULL) == 1);

    struct message_header *hdr = m17_sms_ops.get(NULL, 0);
    REQUIRE(hdr != nullptr);
    REQUIRE(hdr->direction == MSG_DIR_RX);
    REQUIRE(hdr->status == MSG_STATUS_RECEIVED);
    REQUIRE(hdr->unread == true);
    REQUIRE(strcmp(hdr->sender, "W1AW") == 0);
    REQUIRE(strcmp(static_cast<const char *>(hdr->body), "world") == 0);
}

TEST_CASE("m17_sms loopback: TX then RX round-trip yields two entries",
          "[m17_sms][loopback]")
{
    packet_io_init();
    m17_sms_init();
    m17_sms_task_rtx();

    REQUIRE(m17_sms_ops.send(NULL, "ping", 4, "REMOTE") == 0);
    m17_sms_task_rtx();
    m17_sms_ops.tick(NULL);

    REQUIRE(loopback_inject("pong", 4) == true);
    m17_sms_task_rtx();
    m17_sms_ops.tick(NULL);

    REQUIRE(m17_sms_ops.count(NULL) == 2);

    struct message_header *tx_hdr = nullptr;
    struct message_header *rx_hdr = nullptr;
    for (size_t i = 0; i < 2; i++) {
        struct message_header *h = m17_sms_ops.get(NULL, i);
        REQUIRE(h != nullptr);
        if (h->direction == MSG_DIR_TX)
            tx_hdr = h;
        else
            rx_hdr = h;
    }
    REQUIRE(tx_hdr != nullptr);
    REQUIRE(rx_hdr != nullptr);
    REQUIRE(tx_hdr->status == MSG_STATUS_SENT);
    REQUIRE(strcmp(static_cast<const char *>(tx_hdr->body), "ping") == 0);
    REQUIRE(rx_hdr->status == MSG_STATUS_RECEIVED);
    REQUIRE(strcmp(static_cast<const char *>(rx_hdr->body), "pong") == 0);
    REQUIRE(strcmp(rx_hdr->sender, "W1AW") == 0);
}

TEST_CASE("m17_sms loopback: RX descriptor is re-armed after receipt",
          "[m17_sms][loopback]")
{
    packet_io_init();
    m17_sms_init();
    m17_sms_task_rtx();

    REQUIRE(loopback_get_rx_desc() != nullptr);
    REQUIRE(loopback_inject("first", 5) == true);
    m17_sms_task_rtx();
    m17_sms_ops.tick(NULL);

    struct pktDesc *desc = loopback_get_rx_desc();
    REQUIRE(desc != nullptr);
    REQUIRE(desc->status == PKT_STATUS_IDLE);

    REQUIRE(loopback_inject("second", 6) == true);
    m17_sms_task_rtx();
    m17_sms_ops.tick(NULL);

    REQUIRE(m17_sms_ops.count(NULL) == 2);
}

#else
TEST_CASE("M17 SMS Loopback (skipped: CONFIG_M17_SMS not set)",
          "[m17][sms][loopback]")
{
    SUCCEED();
}
#endif
