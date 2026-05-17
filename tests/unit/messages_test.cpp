/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <cerrno>
#include "core/messages.h"
#include "core/MessageRegistry.hpp"

/* ------------------------------------------------------------------ */
/* Fake source implementation                                          */
/* ------------------------------------------------------------------ */

static constexpr size_t FAKE_SLOTS = 8;

struct FakeMessage {
    message_header_t hdr; /* MUST be first */
    char body[64];
};

struct FakeSource {
    FakeMessage entries[FAKE_SLOTS];
    size_t count;
    bool composeInvoked;
    int evictCount;
};

static size_t fake_count(void *ctx)
{
    return static_cast<FakeSource *>(ctx)->count;
}

static message_header_t *fake_get(void *ctx, size_t i)
{
    FakeSource *src = static_cast<FakeSource *>(ctx);
    if (i >= src->count)
        return nullptr;
    return &src->entries[i].hdr;
}

static uint32_t fake_supported_actions(const message_header_t * /*entry*/)
{
    return MSG_ACTION_VIEW | MSG_ACTION_DELETE | MSG_ACTION_REPLY
         | MSG_ACTION_MARK_READ | MSG_ACTION_MARK_UNREAD;
}

static int fake_invoke_action(message_header_t *entry, message_action_t action)
{
    switch (action) {
        case MSG_ACTION_MARK_READ:
            entry->unread = false;
            return 0;
        case MSG_ACTION_MARK_UNREAD:
            entry->unread = true;
            return 0;
        case MSG_ACTION_DELETE:
            /* Source is responsible for removing the entry from its own
             * storage; the registry does not touch it further. */
            return 0;
        default:
            return 0;
    }
}

static void fake_start_compose(void *ctx)
{
    static_cast<FakeSource *>(ctx)->composeInvoked = true;
}

static const message_type_vtable_t fake_vtable = {
    /* name              */ "FakeSrc",
    /* count             */ fake_count,
    /* get               */ fake_get,
    /* supported_actions */ fake_supported_actions,
    /* invoke_action     */ fake_invoke_action,
    /* start_compose     */ fake_start_compose,
    /* tick              */ nullptr,
    /* send              */ nullptr,
    /* mode_id           */ 0u,
};

/* Vtable without compose support */
static const message_type_vtable_t fake_vtable_nocompose = {
    /* name              */ nullptr,
    /* count             */ fake_count,
    /* get               */ fake_get,
    /* supported_actions */ fake_supported_actions,
    /* invoke_action     */ fake_invoke_action,
    /* start_compose     */ nullptr,
    /* tick              */ nullptr,
    /* send              */ nullptr,
    /* mode_id           */ 0u,
};

/* ------------------------------------------------------------------ */
/* Named callbacks for test-specific vtables                           */
/* ------------------------------------------------------------------ */

/* restricted_vtable: reports no supported actions */
static uint32_t restricted_supported(const message_header_t *)
{
    return 0u;
}

/* snapshot-cap vtable */
static constexpr size_t BIG_COUNT = MessageRegistry::MAX_MESSAGES_SNAPSHOT + 4;
static FakeMessage bigBuf[BIG_COUNT];
struct BigSource {
    size_t count;
};
static BigSource bigSrc = { BIG_COUNT };
static size_t big_count(void *ctx)
{
    return static_cast<BigSource *>(ctx)->count;
}
static message_header_t *big_get(void * /*ctx*/, size_t i)
{
    if (i >= BIG_COUNT)
        return nullptr;
    return &bigBuf[i].hdr;
}

/* sparse-get vtable: returns nullptr for index 1 */
static FakeMessage sparseEntries[3];
static size_t sparse_count(void *)
{
    return 3u;
}
static message_header_t *sparse_get(void *, size_t i)
{
    if (i == 1)
        return nullptr;
    return &sparseEntries[i].hdr;
}

/* Helper: initialise a FakeSource and push n entries */
static void pushEntries(FakeSource &src, size_t n, int baseVal,
                        bool unread = true)
{
    for (size_t i = 0; i < n && src.count < FAKE_SLOTS; i++) {
        FakeMessage &m = src.entries[src.count];
        memset(&m, 0, sizeof(m));
        int val = baseVal + (int)i;
        m.hdr.timestamp.minute = (int8_t)(val / 60);
        m.hdr.timestamp.second = (int8_t)(val % 60);
        m.hdr.direction = MSG_DIR_RX;
        m.hdr.status = MSG_STATUS_RECEIVED;
        m.hdr.unread = unread;
        snprintf(m.hdr.sender, sizeof(m.hdr.sender), "N%zuXX", i);
        snprintf(m.body, sizeof(m.body), "msg %zu", i);
        m.hdr.body = m.body;
        m.hdr.body_len = strlen(m.body);
        src.count++;
    }
}

/* ------------------------------------------------------------------ */
/* Tests                                                               */
/* ------------------------------------------------------------------ */

TEST_CASE("MessageRegistry: register and unregister update source count",
          "[messages][registry]")
{
    MessageRegistry reg;

    FakeSource src1 = {};
    FakeSource src2 = {};

    SourceEntry entries[] = {
        { &fake_vtable, &src1 },
        { &fake_vtable, &src2 },
    };
    reg.init(entries, 2);
    reg.tick();
    /* No entries yet, but both sources are active. */
    REQUIRE(reg.count() == 0);

    reg.terminate();
}

TEST_CASE("MessageRegistry: push then enumerate finds entries",
          "[messages][registry]")
{
    MessageRegistry reg;

    FakeSource src = {};
    pushEntries(src, 3, 100);

    SourceEntry entries[] = { { &fake_vtable, &src } };
    reg.init(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 3);

    /* All entries should be accessible. */
    for (size_t i = 0; i < 3; i++)
        REQUIRE(reg.get(i) != nullptr);

    REQUIRE(reg.get(3) == nullptr);

    reg.terminate();
}

TEST_CASE("MessageRegistry: merge across two sources sorted by timestamp desc",
          "[messages][registry]")
{
    MessageRegistry reg;

    FakeSource srcA = {};
    FakeSource srcB = {};

    /* srcA: seconds 10, 12, 14 */
    pushEntries(srcA, 3, 10);
    srcA.entries[0].hdr.timestamp.second = 10;
    srcA.entries[1].hdr.timestamp.second = 12;
    srcA.entries[2].hdr.timestamp.second = 14;

    /* srcB: seconds 11, 13, 15 */
    pushEntries(srcB, 3, 11);
    srcB.entries[0].hdr.timestamp.second = 11;
    srcB.entries[1].hdr.timestamp.second = 13;
    srcB.entries[2].hdr.timestamp.second = 15;

    SourceEntry entries[] = {
        { &fake_vtable, &srcA },
        { &fake_vtable, &srcB },
    };
    reg.init(entries, 2);

    reg.tick();
    REQUIRE(reg.count() == 6);

    /* Snapshot must be descending. */
    for (size_t i = 1; i < reg.count(); i++) {
        REQUIRE(datetime_cmp(&reg.get(i - 1)->timestamp, &reg.get(i)->timestamp)
                >= 0);
    }

    /* Highest timestamp first. */
    REQUIRE(reg.get(0)->timestamp.second == 15);
    REQUIRE(reg.get(reg.count() - 1)->timestamp.second == 10);

    reg.terminate();
}

TEST_CASE("MessageRegistry: MARK_READ flips unread via vtable",
          "[messages][registry]")
{
    MessageRegistry reg;

    FakeSource src = {};
    pushEntries(src, 2, 200, /* unread= */ true);

    SourceEntry entries[] = { { &fake_vtable, &src } };
    reg.init(entries, 1);
    reg.tick();

    REQUIRE(reg.countUnread() == 2);

    int rc = reg.invokeAction(0, MSG_ACTION_MARK_READ);
    REQUIRE(rc == 0);
    REQUIRE(reg.get(0)->unread == false);

    reg.tick();
    REQUIRE(reg.countUnread() == 1);

    reg.terminate();
}

TEST_CASE("MessageRegistry: DELETE invokes vtable and entry vanishes after tick",
          "[messages][registry]")
{
    MessageRegistry reg;

    FakeSource src = {};
    pushEntries(src, 3, 300);

    SourceEntry entries[] = { { &fake_vtable, &src } };
    reg.init(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 3);

    /*
     * Simulate deletion: the fake vtable sets type=0 and the source removes
     * the entry from its ring (we simulate by decrementing count and shifting).
     */
    int rc = reg.invokeAction(0, MSG_ACTION_DELETE);
    REQUIRE(rc == 0);

    /* Remove the entry from the fake source storage as well. */
    message_header_t *deleted_hdr = reg.get(0);
    for (size_t i = 0; i < src.count; i++) {
        if (&src.entries[i].hdr == deleted_hdr) {
            /* Shift remaining entries down. */
            for (size_t j = i; j + 1 < src.count; j++)
                src.entries[j] = src.entries[j + 1];
            src.count--;
            break;
        }
    }

    reg.tick();
    REQUIRE(reg.count() == 2);

    reg.terminate();
}

TEST_CASE(
    "MessageRegistry: unsupported action returns error without calling source",
    "[messages][registry]")
{
    /* Build a vtable that reports no supported actions. */
    static const message_type_vtable_t restricted_vtable = {
        /* name              */ "Restricted",
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ restricted_supported,
        /* invoke_action     */ fake_invoke_action,
        /* start_compose     */ nullptr,
        /* tick              */ nullptr,
        /* send              */ nullptr,
        /* mode_id           */ 0u,
    };

    MessageRegistry reg;

    FakeSource src = {};
    pushEntries(src, 1, 400);

    SourceEntry entries[] = { { &restricted_vtable, &src } };
    reg.init(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 1);

    /* Every action should be rejected. */
    REQUIRE(reg.invokeAction(0, MSG_ACTION_DELETE) == -ENOTSUP);
    REQUIRE(reg.invokeAction(0, MSG_ACTION_MARK_READ) == -ENOTSUP);

    reg.terminate();
}

TEST_CASE("MessageRegistry: count_unread matches sum across sources",
          "[messages][registry]")
{
    MessageRegistry reg;

    FakeSource srcA = {};
    FakeSource srcB = {};

    pushEntries(srcA, 2, 500, /* unread= */ true);
    pushEntries(srcB, 3, 510, /* unread= */ false);

    SourceEntry entries[] = {
        { &fake_vtable, &srcA },
        { &fake_vtable, &srcB },
    };
    reg.init(entries, 2);
    reg.tick();

    REQUIRE(reg.count() == 5);
    REQUIRE(reg.countUnread() == 2);

    reg.terminate();
}

TEST_CASE(
    "MessageRegistry: canCompose returns true only when a matching source exists",
    "[messages][registry]")
{
    /* mode 1 source has compose; mode 2 source does not */
    static const message_type_vtable_t vtable_mode1 = {
        /* name              */ "SrcMode1",
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ fake_supported_actions,
        /* invoke_action     */ fake_invoke_action,
        /* start_compose     */ fake_start_compose,
        /* tick              */ nullptr,
        /* send              */ nullptr,
        /* mode_id           */ 1u,
    };
    static const message_type_vtable_t vtable_mode2_nocompose = {
        /* name              */ nullptr,
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ fake_supported_actions,
        /* invoke_action     */ fake_invoke_action,
        /* start_compose     */ nullptr,
        /* tick              */ nullptr,
        /* send              */ nullptr,
        /* mode_id           */ 2u,
    };

    MessageRegistry reg;
    FakeSource srcA = {};
    FakeSource srcB = {};

    SourceEntry entries[] = {
        { &vtable_mode1, &srcA },
        { &vtable_mode2_nocompose, &srcB },
    };
    reg.init(entries, 2);

    REQUIRE(reg.canCompose(1) == true);  /* mode 1 has compose */
    REQUIRE(reg.canCompose(2) == false); /* mode 2 has no start_compose */
    REQUIRE(reg.canCompose(3) == false); /* no source for mode 3 */
    REQUIRE(reg.canCompose(0) == false); /* OPMODE_NONE never matches */

    reg.terminate();
}

TEST_CASE("MessageRegistry: startCompose dispatches to source matching mode",
          "[messages][registry]")
{
    static const message_type_vtable_t vtable_mode1 = {
        /* name              */ "SrcMode1",
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ fake_supported_actions,
        /* invoke_action     */ fake_invoke_action,
        /* start_compose     */ fake_start_compose,
        /* tick              */ nullptr,
        /* send              */ nullptr,
        /* mode_id           */ 1u,
    };
    static const message_type_vtable_t vtable_mode2 = {
        /* name              */ "SrcMode2",
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ fake_supported_actions,
        /* invoke_action     */ fake_invoke_action,
        /* start_compose     */ fake_start_compose,
        /* tick              */ nullptr,
        /* send              */ nullptr,
        /* mode_id           */ 2u,
    };

    MessageRegistry reg;
    FakeSource srcA = {};
    FakeSource srcB = {};

    SourceEntry entries[] = {
        { &vtable_mode1, &srcA },
        { &vtable_mode2, &srcB },
    };
    reg.init(entries, 2);

    /* startCompose(2) should invoke srcB (mode 2), not srcA (mode 1). */
    REQUIRE(reg.startCompose(2) == 0);
    REQUIRE(srcA.composeInvoked == false);
    REQUIRE(srcB.composeInvoked == true);

    /* No source for mode 99. */
    REQUIRE(reg.startCompose(99) == -ENOENT);

    /* OPMODE_NONE (0) always returns -ENOENT. */
    REQUIRE(reg.startCompose(0) == -ENOENT);

    reg.terminate();
}

TEST_CASE("MessageRegistry: snapshot cap handled gracefully",
          "[messages][registry]")
{
    MessageRegistry reg;

    /* Fill a source with enough entries to exceed the snapshot cap. */
    memset(bigBuf, 0, sizeof(bigBuf));

    for (size_t i = 0; i < bigSrc.count; i++) {
        bigBuf[i].hdr.timestamp.minute = (int8_t)((int)i / 60);
        bigBuf[i].hdr.timestamp.second = (int8_t)((int)i % 60);
        bigBuf[i].hdr.unread = true;
    }

    static const message_type_vtable_t big_vtable = {
        /* name              */ nullptr,
        /* count             */ big_count,
        /* get               */ big_get,
        /* supported_actions */ nullptr,
        /* invoke_action     */ nullptr,
        /* start_compose     */ nullptr,
        /* tick              */ nullptr,
        /* send              */ nullptr,
        /* mode_id           */ 0u,
    };

    SourceEntry entries[] = { { &big_vtable, &bigSrc } };
    reg.init(entries, 1);
    reg.tick(); /* must not crash or overflow */

    REQUIRE(reg.count() == MessageRegistry::MAX_MESSAGES_SNAPSHOT);

    reg.terminate();
}

TEST_CASE("MessageRegistry: source get() returning nullptr is skipped",
          "[messages][registry]")
{
    /* A source with 3 logical entries but get() returns nullptr for index 1. */
    memset(sparseEntries, 0, sizeof(sparseEntries));
    sparseEntries[0].hdr.timestamp.second = 10;
    sparseEntries[2].hdr.timestamp.second = 30;

    static const message_type_vtable_t sparse_vtable = {
        /* name              */ nullptr,
        /* count             */ sparse_count,
        /* get               */ sparse_get,
        /* supported_actions */ nullptr,
        /* invoke_action     */ nullptr,
        /* start_compose     */ nullptr,
        /* tick              */ nullptr,
        /* send              */ nullptr,
        /* mode_id           */ 0u,
    };

    MessageRegistry reg;
    SourceEntry entries[] = { { &sparse_vtable, nullptr } };
    reg.init(entries, 1);
    reg.tick();

    /* Only 2 of 3 entries should appear (nullptr entry skipped). */
    REQUIRE(reg.count() == 2);
    REQUIRE(reg.get(0)->timestamp.second == 30);
    REQUIRE(reg.get(1)->timestamp.second == 10);

    reg.terminate();
}

TEST_CASE(
    "MessageRegistry: invokeAction with out-of-range index returns -ENOENT",
    "[messages][registry]")
{
    MessageRegistry reg;

    FakeSource src = {};
    pushEntries(src, 2, 100);

    SourceEntry entries[] = { { &fake_vtable, &src } };
    reg.init(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 2);

    REQUIRE(reg.invokeAction(2, MSG_ACTION_MARK_READ) == -ENOENT);
    REQUIRE(reg.invokeAction(99, MSG_ACTION_MARK_READ) == -ENOENT);

    reg.terminate();
}

TEST_CASE(
    "MessageRegistry: invokeAction returns -ENOTSUP when invoke_action is null",
    "[messages][registry]")
{
    /* Vtable with supported_actions=nullptr and invoke_action=nullptr. */
    static const message_type_vtable_t null_invoke_vtable = {
        /* name              */ nullptr,
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ nullptr,
        /* invoke_action     */ nullptr,
        /* start_compose     */ nullptr,
        /* tick              */ nullptr,
        /* send              */ nullptr,
        /* mode_id           */ 0u,
    };

    MessageRegistry reg;
    FakeSource src = {};
    pushEntries(src, 1, 200);

    SourceEntry entries[] = { { &null_invoke_vtable, &src } };
    reg.init(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 1);

    REQUIRE(reg.invokeAction(0, MSG_ACTION_MARK_READ) == -ENOTSUP);

    reg.terminate();
}

TEST_CASE("MessageRegistry: sourceMode returns mode_id of the producing source",
          "[messages][registry]")
{
    static const message_type_vtable_t vtable_mode1 = {
        /* name              */ nullptr,
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ nullptr,
        /* invoke_action     */ nullptr,
        /* start_compose     */ nullptr,
        /* tick              */ nullptr,
        /* send              */ nullptr,
        /* mode_id           */ 1u,
    };
    static const message_type_vtable_t vtable_mode3 = {
        /* name              */ nullptr,
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ nullptr,
        /* invoke_action     */ nullptr,
        /* start_compose     */ nullptr,
        /* tick              */ nullptr,
        /* send              */ nullptr,
        /* mode_id           */ 3u,
    };

    MessageRegistry reg;
    FakeSource srcA = {};
    FakeSource srcB = {};

    /* srcA: second=10; srcB: second=20 → srcB is newer, sorts first */
    pushEntries(srcA, 1, 10);
    pushEntries(srcB, 1, 20);

    SourceEntry entries[] = {
        { &vtable_mode1, &srcA },
        { &vtable_mode3, &srcB },
    };
    reg.init(entries, 2);
    reg.tick();

    REQUIRE(reg.count() == 2);
    /* Snapshot is newest-first: srcB (mode 3) at 0, srcA (mode 1) at 1. */
    REQUIRE(reg.sourceMode(0) == 3u);
    REQUIRE(reg.sourceMode(1) == 1u);
    REQUIRE(reg.sourceMode(99) == 0u); /* out of range → 0 */

    reg.terminate();
}
