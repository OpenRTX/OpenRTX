/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <errno.h>
#include <string>
#include "core/m17_sms.h"
#include "core/messages.h"
#include "protocols/M17/m17.h"
#include "rtx_packet_stub.h"

static constexpr size_t HEADER = offsetof(struct m17Packet, payload);

/* Reset the inbox and activate it for M17, as the UI thread would. */
static void setup()
{
    rtxStub_reset();
    messages_init();
    REQUIRE(messages_registerSource(&m17_sms_ops) == 0);
    messages_task(OPMODE_M17);
}

/* Build a received packet descriptor carrying an SMS from src to dst. */
static struct pktDesc rxPacket(struct m17Packet &packet, const char *src,
                               const char *dst, const std::string &text)
{
    memset(&packet, 0, sizeof(packet));
    strncpy(packet.src, src, sizeof(packet.src));
    strncpy(packet.dst, dst, sizeof(packet.dst));
    packet.payload[0] = 0x05;
    memcpy(&packet.payload[1], text.data(), text.size());
    packet.payload[1 + text.size()] = 0x00;

    struct pktDesc desc = {};
    desc.status = PKT_STATUS_DONE;
    desc.buffer = &packet;
    desc.size = sizeof(packet);
    desc.res = 1 + text.size() + 1;

    return desc;
}

static struct message txMessage(const char *body)
{
    struct message msg = {};

    msg.body = body;
    msg.bodyLen = strlen(body);
    msg.mode = OPMODE_M17;
    strcpy(msg.sender, "N0CALL");
    strcpy(msg.recipient, "W1AW");

    return msg;
}

TEST_CASE("m17_sms: source serves the M17 opmode", "[m17][messages]")
{
    CHECK(m17_sms_ops.mode == OPMODE_M17);
    CHECK(std::string(m17_sms_ops.name) == "M17 SMS");
    CHECK(m17_sms_ops.processRx != nullptr);
    CHECK(m17_sms_ops.formatTx != nullptr);
}

TEST_CASE("m17_sms: a received SMS is filed in the inbox", "[m17][messages]")
{
    setup();

    struct m17Packet packet;
    struct pktDesc desc = rxPacket(packet, "W1AW", "N0CALL", "Hello");

    REQUIRE(m17_sms_ops.processRx(&desc) == 0);
    REQUIRE(messages_count() == 1);

    const struct message *msg = messages_get(0);
    CHECK(std::string(msg->body) == "Hello");
    CHECK(msg->bodyLen == 5);
    CHECK(std::string(msg->sender) == "W1AW");
    CHECK(std::string(msg->recipient) == "N0CALL");
    CHECK(msg->mode == OPMODE_M17);
    CHECK(msg->direction == MSG_DIR_RX);
    CHECK(msg->status == MSG_STATUS_RECEIVED);
    CHECK(msg->unread == 1);
    CHECK(messages_task(OPMODE_M17) == 1);
}

TEST_CASE("m17_sms: a maximum length SMS is stored intact", "[m17][messages]")
{
    setup();

    std::string text(MSG_BODY_MAX_LEN - 1, 'z');
    struct m17Packet packet;
    struct pktDesc desc = rxPacket(packet, "W1AW", "N0CALL", text);

    REQUIRE(m17_sms_ops.processRx(&desc) == 0);
    REQUIRE(messages_count() == 1);
    CHECK(std::string(messages_get(0)->body) == text);
}

TEST_CASE("m17_sms: packets that are not SMS are ignored", "[m17][messages]")
{
    setup();

    struct m17Packet packet;
    struct pktDesc desc = rxPacket(packet, "W1AW", "N0CALL", "Hello");

    packet.payload[0] = 0x04;
    CHECK(m17_sms_ops.processRx(&desc) == -EPROTO);

    packet.payload[0] = 0x05;
    desc.res = 0;
    CHECK(m17_sms_ops.processRx(&desc) == -EINVAL);

    desc.res = -EIO;
    CHECK(m17_sms_ops.processRx(&desc) == -EINVAL);

    CHECK(messages_count() == 0);
}

TEST_CASE("m17_sms: an outgoing SMS is formatted as an M17 packet",
          "[m17][messages]")
{
    setup();

    struct m17Packet packet;
    memset(&packet, 0xAA, sizeof(packet));
    struct pktDesc desc = {};
    desc.buffer = &packet;
    desc.size = sizeof(packet);

    struct message msg = txMessage("Hello");
    REQUIRE(m17_sms_ops.formatTx(&msg, &desc) == 0);

    CHECK(desc.size == HEADER + 1 + 5 + 1);
    CHECK(std::string(packet.src) == "N0CALL");
    CHECK(std::string(packet.dst) == "W1AW");
    CHECK(packet.payload[0] == 0x05);
    CHECK(memcmp(&packet.payload[1], "Hello", 5) == 0);
    CHECK(packet.payload[6] == 0x00);

    /* Callsign fields are NUL padded, not just terminated. */
    CHECK(packet.src[9] == '\0');
    CHECK(packet.dst[9] == '\0');
}

TEST_CASE("m17_sms: format rejects what does not fit a packet",
          "[m17][messages]")
{
    setup();

    struct m17Packet packet;
    struct pktDesc desc = {};
    desc.buffer = &packet;
    desc.size = sizeof(packet);

    struct message msg = txMessage("");
    CHECK(m17_sms_ops.formatTx(&msg, &desc) == -EINVAL);

    std::string tooLong(MSG_BODY_MAX_LEN, 'z');
    msg = txMessage(tooLong.c_str());
    CHECK(m17_sms_ops.formatTx(&msg, &desc) == -EMSGSIZE);

    msg = txMessage("Hello");
    desc.size = HEADER + 3;
    CHECK(m17_sms_ops.formatTx(&msg, &desc) == -EMSGSIZE);

    desc.size = HEADER;
    CHECK(m17_sms_ops.formatTx(&msg, &desc) == -EINVAL);
}

TEST_CASE("m17_sms: a sent SMS loops back through the inbox", "[m17][messages]")
{
    setup();

    /* Reception is armed by setup(); send. */
    struct pktDesc *rx = rtxStub_rxDesc();
    REQUIRE(rx != nullptr);

    struct message msg = txMessage("Round trip");
    REQUIRE(messages_send(&msg) == 0);
    REQUIRE(messages_count() == 1);
    CHECK(messages_get(0)->status == MSG_STATUS_SENDING);

    struct pktDesc *tx = rtxStub_txDesc();
    REQUIRE(tx != nullptr);
    CHECK(tx->size == HEADER + 1 + 10 + 1);

    /* Deliver the transmitted packet as if it had been received. */
    memcpy(rx->buffer, tx->buffer, tx->size);
    rx->res = tx->size - HEADER;
    rx->status = PKT_STATUS_DONE;
    tx->status = PKT_STATUS_DONE;
    CHECK(messages_task(OPMODE_M17) == 1);

    REQUIRE(messages_count() == 2);
    const struct message *received = messages_get(0);
    const struct message *sent = messages_get(1);
    CHECK(received->direction == MSG_DIR_RX);
    CHECK(std::string(received->body) == "Round trip");
    CHECK(std::string(received->sender) == "N0CALL");
    CHECK(std::string(received->recipient) == "W1AW");
    CHECK(sent->direction == MSG_DIR_TX);
    CHECK(sent->status == MSG_STATUS_SENT);
    CHECK(messages_task(OPMODE_M17) == 0);
}
