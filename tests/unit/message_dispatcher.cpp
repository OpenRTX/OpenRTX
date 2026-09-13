/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <errno.h>
#include <string>
#include "core/message_dispatcher.h"
#include "core/messages.h"
#include "rtx_packet_stub.h"

namespace
{

constexpr uint8_t MODE_A = 3;
constexpr uint8_t MODE_B = 4;

/*
 * A fake protocol whose packets are the message body as plain text: receive
 * files the buffer as-is from a fixed sender, transmit copies the body.
 */
size_t rxCalls = 0;
size_t txCalls = 0;
int txResult = 0;
std::string lastRx;

int fakeProcessRx(const struct pktDesc *pkt)
{
    rxCalls++;
    lastRx.assign(static_cast<const char *>(pkt->buffer), pkt->size);

    struct message msg = {};
    msg.body = lastRx.c_str();
    msg.body_len = lastRx.size();
    msg.mode = MODE_A;
    msg.direction = MSG_DIR_RX;
    msg.status = MSG_STATUS_RECEIVED;
    msg.unread = 1;
    strcpy(msg.sender, "W1AW");

    return messages_store(&msg, nullptr);
}

int fakeFormatTx(const struct message *msg, struct pktDesc *pkt)
{
    txCalls++;
    if (txResult != 0)
        return txResult;

    if (msg->body_len > pkt->size)
        return -EMSGSIZE;

    memcpy(pkt->buffer, msg->body, msg->body_len);
    pkt->size = msg->body_len;
    return 0;
}

int failRx(const struct pktDesc *pkt)
{
    (void)pkt;
    return -EIO;
}

int failTx(const struct message *msg, struct pktDesc *pkt)
{
    (void)msg;
    (void)pkt;
    return -EIO;
}

const struct message_ops fakeOps = { fakeProcessRx, fakeFormatTx, "Fake",
                                     MODE_A };
const struct message_ops otherOps = { failRx, failTx, "Other", MODE_B };

void setup()
{
    rtxStub_reset();
    messages_init();
    message_dispatcher_init();
    rxCalls = 0;
    txCalls = 0;
    txResult = 0;
    lastRx.clear();
}

/* Complete a submitted receive descriptor with the given packet bytes. */
void deliver(struct pktDesc *desc, const char *text)
{
    REQUIRE(desc != nullptr);
    REQUIRE(desc->status == PKT_STATUS_SUBMITTED);
    size_t len = strlen(text);
    REQUIRE(len <= desc->size);
    memcpy(desc->buffer, text, len);
    desc->size = len;
    desc->status = PKT_STATUS_DONE;
}

struct message txMessage(const char *body, uint8_t mode = MODE_A)
{
    struct message msg = {};

    msg.body = body;
    msg.body_len = strlen(body);
    msg.mode = mode;
    strcpy(msg.sender, "N0CALL");
    strcpy(msg.recipient, "W1AW");

    return msg;
}

} // namespace

TEST_CASE("message_dispatcher: source registration", "[messages]")
{
    setup();

    CHECK(message_dispatcher_source(MODE_A) == nullptr);
    CHECK(message_dispatcher_register(nullptr) == -EINVAL);

    struct message_ops incomplete = fakeOps;
    incomplete.format_tx = nullptr;
    CHECK(message_dispatcher_register(&incomplete) == -EINVAL);

    REQUIRE(message_dispatcher_register(&fakeOps) == 0);
    CHECK(message_dispatcher_source(MODE_A) == &fakeOps);
    CHECK(message_dispatcher_register(&fakeOps) == -EEXIST);

    REQUIRE(message_dispatcher_register(&otherOps) == 0);
    CHECK(message_dispatcher_source(MODE_B) == &otherOps);

    /* Table full: CONFIG_MESSAGES_MAX_SOURCES defaults to two. */
    struct message_ops third = fakeOps;
    third.mode = 5;
    CHECK(message_dispatcher_register(&third) == -ENOSPC);

    message_dispatcher_init();
    CHECK(message_dispatcher_source(MODE_A) == nullptr);
}

TEST_CASE("message_dispatcher: no receive without a source for the mode",
          "[messages]")
{
    setup();
    REQUIRE(message_dispatcher_register(&fakeOps) == 0);

    message_dispatcher_task(MODE_B);
    CHECK(rtxStub_rxSubmissions() == 0);

    message_dispatcher_task(MODE_A);
    CHECK(rtxStub_rxSubmissions() == 1);
}

TEST_CASE("message_dispatcher: received packets reach their source",
          "[messages]")
{
    setup();
    REQUIRE(message_dispatcher_register(&fakeOps) == 0);

    message_dispatcher_task(MODE_A);
    struct pktDesc *desc = rtxStub_rxDesc();
    REQUIRE(desc != nullptr);
    CHECK(desc->buffer != nullptr);
    CHECK(desc->size == MSG_PKT_MAX_SIZE);

    /* Nothing happens while the descriptor is pending. */
    message_dispatcher_task(MODE_A);
    CHECK(rtxStub_rxSubmissions() == 1);
    CHECK(rxCalls == 0);

    deliver(desc, "hello");
    message_dispatcher_task(MODE_A);

    CHECK(rxCalls == 1);
    CHECK(lastRx == "hello");
    REQUIRE(messages_count() == 1);
    CHECK(std::string(messages_get(0)->body) == "hello");
    CHECK(messages_tick() == 1);

    /* The slot is resubmitted right away with its full buffer. */
    CHECK(rtxStub_rxSubmissions() == 2);
    CHECK(desc->status == PKT_STATUS_SUBMITTED);
    CHECK(desc->size == MSG_PKT_MAX_SIZE);
}

TEST_CASE("message_dispatcher: receive errors are dropped and rearmed",
          "[messages]")
{
    setup();
    REQUIRE(message_dispatcher_register(&fakeOps) == 0);

    message_dispatcher_task(MODE_A);
    struct pktDesc *desc = rtxStub_rxDesc();
    REQUIRE(desc != nullptr);

    desc->res = -EIO;
    desc->status = PKT_STATUS_ERROR;
    message_dispatcher_task(MODE_A);

    CHECK(rxCalls == 0);
    CHECK(messages_count() == 0);
    CHECK(rtxStub_rxSubmissions() == 2);
    CHECK(desc->status == PKT_STATUS_SUBMITTED);
}

TEST_CASE("message_dispatcher: a refused receive is retried", "[messages]")
{
    setup();
    REQUIRE(message_dispatcher_register(&fakeOps) == 0);

    rtxStub_setRxResult(-EAGAIN);
    message_dispatcher_task(MODE_A);
    CHECK(rtxStub_rxSubmissions() == 0);

    rtxStub_setRxResult(0);
    message_dispatcher_task(MODE_A);
    CHECK(rtxStub_rxSubmissions() == 1);
}

TEST_CASE("message_dispatcher: a packet is handled by the mode it was "
          "submitted in",
          "[messages]")
{
    setup();
    REQUIRE(message_dispatcher_register(&fakeOps) == 0);

    message_dispatcher_task(MODE_A);
    struct pktDesc *desc = rtxStub_rxDesc();
    deliver(desc, "late");

    /* The mode changed to one without a source before completion. */
    message_dispatcher_task(MODE_B);

    CHECK(rxCalls == 1);
    CHECK(lastRx == "late");
    CHECK(desc->status == PKT_STATUS_IDLE);
    CHECK(rtxStub_rxSubmissions() == 1);
}

TEST_CASE("message_dispatcher: send formats and submits a packet", "[messages]")
{
    setup();
    REQUIRE(message_dispatcher_register(&fakeOps) == 0);

    struct message msg = txMessage("outgoing");
    uint32_t seq = 0;
    msg.direction = MSG_DIR_TX;
    msg.status = MSG_STATUS_SENDING;
    REQUIRE(messages_store(&msg, &seq) == 0);

    REQUIRE(message_dispatcher_send(messages_get(0)) == 0);
    CHECK(txCalls == 1);
    CHECK(rtxStub_txSubmissions() == 1);

    struct pktDesc *desc = rtxStub_txDesc();
    REQUIRE(desc != nullptr);
    CHECK(desc->status == PKT_STATUS_SUBMITTED);
    CHECK(desc->size == 8);
    CHECK(memcmp(desc->buffer, "outgoing", 8) == 0);

    /* Only one transmission at a time. */
    CHECK(message_dispatcher_send(messages_get(0)) == -EBUSY);
    CHECK(txCalls == 1);

    message_dispatcher_task(MODE_A);
    CHECK(messages_get(0)->status == MSG_STATUS_SENDING);

    desc->status = PKT_STATUS_DONE;
    message_dispatcher_task(MODE_A);
    CHECK(messages_get(0)->status == MSG_STATUS_SENT);
    CHECK(desc->status == PKT_STATUS_IDLE);

    /* The slot is free again. */
    CHECK(message_dispatcher_send(messages_get(0)) == 0);
}

TEST_CASE("message_dispatcher: a failed transmission is reported", "[messages]")
{
    setup();
    REQUIRE(message_dispatcher_register(&fakeOps) == 0);

    struct message msg = txMessage("outgoing");
    msg.direction = MSG_DIR_TX;
    msg.status = MSG_STATUS_SENDING;
    REQUIRE(messages_store(&msg, nullptr) == 0);
    REQUIRE(message_dispatcher_send(messages_get(0)) == 0);

    struct pktDesc *desc = rtxStub_txDesc();
    desc->res = -EIO;
    desc->status = PKT_STATUS_ERROR;
    message_dispatcher_task(MODE_A);

    CHECK(messages_get(0)->status == MSG_STATUS_FAILED);
    CHECK(desc->status == PKT_STATUS_IDLE);
}

TEST_CASE("message_dispatcher: send errors leave the slot free", "[messages]")
{
    setup();
    REQUIRE(message_dispatcher_register(&fakeOps) == 0);

    struct message msg = txMessage("outgoing");
    CHECK(message_dispatcher_send(nullptr) == -EINVAL);

    msg.mode = MODE_B;
    CHECK(message_dispatcher_send(&msg) == -ENOENT);
    msg.mode = MODE_A;

    txResult = -EMSGSIZE;
    CHECK(message_dispatcher_send(&msg) == -EMSGSIZE);
    CHECK(rtxStub_txSubmissions() == 0);
    txResult = 0;

    rtxStub_setTxResult(-ENOTSUP);
    CHECK(message_dispatcher_send(&msg) == -ENOTSUP);
    rtxStub_setTxResult(0);

    CHECK(message_dispatcher_send(&msg) == 0);
    CHECK(rtxStub_txSubmissions() == 1);
}

TEST_CASE("messages: can_compose follows source registration", "[messages]")
{
    setup();

    CHECK_FALSE(messages_can_compose(MODE_A));
    REQUIRE(message_dispatcher_register(&fakeOps) == 0);
    CHECK(messages_can_compose(MODE_A));
    CHECK_FALSE(messages_can_compose(MODE_B));
}

TEST_CASE("messages: send stores an outgoing entry and tracks its outcome",
          "[messages]")
{
    setup();
    REQUIRE(message_dispatcher_register(&fakeOps) == 0);

    struct message msg = txMessage("outgoing");
    REQUIRE(messages_send(&msg) == 0);

    REQUIRE(messages_count() == 1);
    const struct message *stored = messages_get(0);
    CHECK(stored->direction == MSG_DIR_TX);
    CHECK(stored->status == MSG_STATUS_SENDING);
    CHECK(stored->unread == 0);
    CHECK(std::string(stored->sender) == "N0CALL");
    CHECK(std::string(stored->recipient) == "W1AW");
    CHECK(std::string(stored->body) == "outgoing");
    CHECK(messages_tick() == 0);

    rtxStub_txDesc()->status = PKT_STATUS_DONE;
    message_dispatcher_task(MODE_A);
    CHECK(messages_get(0)->status == MSG_STATUS_SENT);
}

TEST_CASE("messages: send failures leave a failed entry", "[messages]")
{
    setup();
    REQUIRE(message_dispatcher_register(&fakeOps) == 0);

    struct message msg = txMessage("nowhere", MODE_B);
    CHECK(messages_send(&msg) == -ENOENT);
    CHECK(messages_count() == 0);

    msg = txMessage("first");
    REQUIRE(messages_send(&msg) == 0);
    msg = txMessage("second");
    CHECK(messages_send(&msg) == -EBUSY);

    REQUIRE(messages_count() == 2);
    CHECK(std::string(messages_get(0)->body) == "second");
    CHECK(messages_get(0)->status == MSG_STATUS_FAILED);
    CHECK(messages_get(1)->status == MSG_STATUS_SENDING);
}
