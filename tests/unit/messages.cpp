/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <errno.h>
#include <string>
#include "core/messages.h"

namespace
{

/* Build a store() template for a received message. */
struct message rxMessage(const char *sender, const char *body)
{
    struct message msg = {};

    msg.body = body;
    msg.body_len = strlen(body);
    msg.mode = 3;
    msg.direction = MSG_DIR_RX;
    msg.status = MSG_STATUS_RECEIVED;
    msg.unread = 1;
    strncpy(msg.sender, sender, MSG_ADDR_MAX_LEN - 1);

    return msg;
}

/* Build a store() template for a message being sent. */
struct message txMessage(const char *recipient, const char *body)
{
    struct message msg = {};

    msg.body = body;
    msg.body_len = strlen(body);
    msg.mode = 3;
    msg.direction = MSG_DIR_TX;
    msg.status = MSG_STATUS_SENDING;
    strncpy(msg.sender, "N0CALL", MSG_ADDR_MAX_LEN - 1);
    strncpy(msg.recipient, recipient, MSG_ADDR_MAX_LEN - 1);

    return msg;
}

} // namespace

TEST_CASE("messages: registry starts empty", "[messages]")
{
    messages_init();

    CHECK(messages_count() == 0);
    CHECK(messages_count_unread() == 0);
    CHECK(messages_get(0) == nullptr);
    CHECK(messages_tick() == 0);
}

TEST_CASE("messages: store copies the body and assigns a sequence",
          "[messages]")
{
    messages_init();

    char body[] = "hello";
    struct message msg = rxMessage("W1AW", body);
    uint32_t seq = 0;

    REQUIRE(messages_store(&msg, &seq) == 0);
    CHECK(seq != 0);

    /* The caller's buffer is no longer referenced. */
    memset(body, 'x', sizeof(body) - 1);

    const struct message *stored = messages_get(0);
    REQUIRE(stored != nullptr);
    CHECK(stored->sequence == seq);
    CHECK(std::string(stored->body) == "hello");
    CHECK(stored->body_len == 5);
    CHECK(std::string(stored->sender) == "W1AW");
    CHECK(stored->direction == MSG_DIR_RX);
    CHECK(stored->status == MSG_STATUS_RECEIVED);
    CHECK(stored->unread == 1);
    CHECK(messages_count() == 1);
    CHECK(messages_count_unread() == 1);
}

TEST_CASE("messages: store rejects bad arguments", "[messages]")
{
    messages_init();

    CHECK(messages_store(nullptr, nullptr) == -EINVAL);

    struct message msg = rxMessage("W1AW", "");
    msg.body = nullptr;
    msg.body_len = 1;
    CHECK(messages_store(&msg, nullptr) == -EINVAL);

    msg.body = "x";
    msg.body_len = MSG_BODY_MAX_LEN;
    CHECK(messages_store(&msg, nullptr) == -EMSGSIZE);

    CHECK(messages_count() == 0);
}

TEST_CASE("messages: empty bodies are allowed", "[messages]")
{
    messages_init();

    struct message msg = rxMessage("W1AW", "");
    msg.body = nullptr;
    msg.body_len = 0;

    REQUIRE(messages_store(&msg, nullptr) == 0);
    REQUIRE(messages_get(0) != nullptr);
    CHECK(messages_get(0)->body[0] == '\0');
}

TEST_CASE("messages: entries are listed newest first", "[messages]")
{
    messages_init();

    uint32_t first = 0;
    uint32_t second = 0;
    uint32_t third = 0;
    struct message msg = rxMessage("W1AW", "one");
    REQUIRE(messages_store(&msg, &first) == 0);
    msg = rxMessage("W1AW", "two");
    REQUIRE(messages_store(&msg, &second) == 0);
    msg = rxMessage("W1AW", "three");
    REQUIRE(messages_store(&msg, &third) == 0);

    CHECK(first < second);
    CHECK(second < third);
    REQUIRE(messages_count() == 3);
    CHECK(messages_get(0)->sequence == third);
    CHECK(messages_get(1)->sequence == second);
    CHECK(messages_get(2)->sequence == first);
    CHECK(std::string(messages_get(0)->body) == "three");
    CHECK(std::string(messages_get(2)->body) == "one");
}

TEST_CASE("messages: find_by_sequence tracks positions", "[messages]")
{
    messages_init();

    uint32_t seq[3];
    for (auto &s : seq) {
        struct message msg = rxMessage("W1AW", "x");
        REQUIRE(messages_store(&msg, &s) == 0);
    }

    CHECK(messages_find_by_sequence(seq[0]) == 2);
    CHECK(messages_find_by_sequence(seq[1]) == 1);
    CHECK(messages_find_by_sequence(seq[2]) == 0);
    CHECK(messages_find_by_sequence(0) == SIZE_MAX);
    CHECK(messages_find_by_sequence(seq[2] + 1) == SIZE_MAX);

    REQUIRE(messages_delete(1) == 0);
    CHECK(messages_find_by_sequence(seq[1]) == SIZE_MAX);
    CHECK(messages_find_by_sequence(seq[0]) == 1);
    CHECK(messages_find_by_sequence(seq[2]) == 0);
}

TEST_CASE("messages: tick reports unread arrivals once", "[messages]")
{
    messages_init();

    struct message msg = rxMessage("W1AW", "a");
    REQUIRE(messages_store(&msg, nullptr) == 0);
    REQUIRE(messages_store(&msg, nullptr) == 0);

    /* Sent messages and pre-read ones are not arrivals. */
    struct message tx = txMessage("W1AW", "b");
    REQUIRE(messages_store(&tx, nullptr) == 0);
    msg.unread = 0;
    REQUIRE(messages_store(&msg, nullptr) == 0);

    CHECK(messages_tick() == 2);
    CHECK(messages_tick() == 0);
    CHECK(messages_count_unread() == 2);
}

TEST_CASE("messages: mark_read toggles the unread flag", "[messages]")
{
    messages_init();

    struct message msg = rxMessage("W1AW", "a");
    REQUIRE(messages_store(&msg, nullptr) == 0);

    CHECK(messages_mark_read(0, true) == 0);
    CHECK(messages_get(0)->unread == 0);
    CHECK(messages_count_unread() == 0);

    CHECK(messages_mark_read(0, false) == 0);
    CHECK(messages_get(0)->unread == 1);
    CHECK(messages_count_unread() == 1);

    CHECK(messages_mark_read(1, true) == -ENOENT);
}

TEST_CASE("messages: set_status updates an entry by sequence", "[messages]")
{
    messages_init();

    struct message tx = txMessage("W1AW", "b");
    uint32_t seq = 0;
    REQUIRE(messages_store(&tx, &seq) == 0);
    CHECK(messages_get(0)->status == MSG_STATUS_SENDING);

    CHECK(messages_set_status(seq, MSG_STATUS_SENT) == 0);
    CHECK(messages_get(0)->status == MSG_STATUS_SENT);

    CHECK(messages_set_status(seq + 1, MSG_STATUS_FAILED) == -ENOENT);
}

TEST_CASE("messages: delete removes an entry and closes the gap", "[messages]")
{
    messages_init();

    for (const char *body : { "one", "two", "three" }) {
        struct message msg = rxMessage("W1AW", body);
        REQUIRE(messages_store(&msg, nullptr) == 0);
    }

    CHECK(messages_delete(3) == -ENOENT);
    REQUIRE(messages_delete(1) == 0);
    REQUIRE(messages_count() == 2);
    CHECK(std::string(messages_get(0)->body) == "three");
    CHECK(std::string(messages_get(1)->body) == "one");

    REQUIRE(messages_delete(0) == 0);
    REQUIRE(messages_delete(0) == 0);
    CHECK(messages_count() == 0);
    CHECK(messages_delete(0) == -ENOENT);
}

TEST_CASE("messages: a full entry table evicts the oldest entry", "[messages]")
{
    messages_init();

    uint32_t oldest = 0;
    struct message msg = rxMessage("W1AW", "first");
    REQUIRE(messages_store(&msg, &oldest) == 0);

    for (size_t i = 1; i < CONFIG_MESSAGES_MAX_ENTRIES; i++) {
        msg = rxMessage("W1AW", "filler");
        REQUIRE(messages_store(&msg, nullptr) == 0);
    }
    REQUIRE(messages_count() == CONFIG_MESSAGES_MAX_ENTRIES);
    CHECK(messages_find_by_sequence(oldest) != SIZE_MAX);

    uint32_t newest = 0;
    msg = rxMessage("W1AW", "last");
    REQUIRE(messages_store(&msg, &newest) == 0);

    CHECK(messages_count() == CONFIG_MESSAGES_MAX_ENTRIES);
    CHECK(messages_find_by_sequence(oldest) == SIZE_MAX);
    CHECK(messages_get(0)->sequence == newest);
    CHECK(std::string(messages_get(messages_count() - 1)->body) == "filler");
}

TEST_CASE("messages: a full body pool evicts the oldest entries", "[messages]")
{
    messages_init();

    /* Bodies of maximum length fill the pool after a few stores. */
    std::string big(MSG_BODY_MAX_LEN - 1, 'a');
    const size_t perPool = CONFIG_MESSAGES_POOL_BYTES / MSG_BODY_MAX_LEN;
    REQUIRE(perPool >= 1);

    uint32_t oldest = 0;
    struct message msg = rxMessage("W1AW", big.c_str());
    REQUIRE(messages_store(&msg, &oldest) == 0);

    for (size_t i = 1; i < perPool; i++) {
        REQUIRE(messages_store(&msg, nullptr) == 0);
    }
    CHECK(messages_count() == perPool);
    CHECK(messages_find_by_sequence(oldest) != SIZE_MAX);

    /* One more wraps the pool and lands on the oldest body. */
    std::string different(MSG_BODY_MAX_LEN - 1, 'b');
    msg = rxMessage("W1AW", different.c_str());
    REQUIRE(messages_store(&msg, nullptr) == 0);

    CHECK(messages_find_by_sequence(oldest) == SIZE_MAX);
    CHECK(messages_count() == perPool);

    /* Every surviving body is intact. */
    for (size_t i = 0; i < messages_count(); i++) {
        const struct message *m = messages_get(i);
        REQUIRE(m != nullptr);
        CHECK(strlen(m->body) == m->body_len);
        CHECK(m->body_len == MSG_BODY_MAX_LEN - 1);
        CHECK(std::string(m->body) == ((i == 0) ? different : big));
    }
}

TEST_CASE("messages: addresses are always NUL-terminated", "[messages]")
{
    messages_init();

    struct message msg = rxMessage("W1AW", "a");
    memset(msg.sender, 'A', sizeof(msg.sender));
    memset(msg.recipient, 'B', sizeof(msg.recipient));
    REQUIRE(messages_store(&msg, nullptr) == 0);

    const struct message *stored = messages_get(0);
    REQUIRE(stored != nullptr);
    CHECK(strlen(stored->sender) == MSG_ADDR_MAX_LEN - 1);
    CHECK(strlen(stored->recipient) == MSG_ADDR_MAX_LEN - 1);
}

TEST_CASE("messages: init discards stored entries", "[messages]")
{
    messages_init();

    struct message msg = rxMessage("W1AW", "a");
    REQUIRE(messages_store(&msg, nullptr) == 0);
    REQUIRE(messages_count() == 1);

    messages_init();
    CHECK(messages_count() == 0);
    CHECK(messages_tick() == 0);
}
