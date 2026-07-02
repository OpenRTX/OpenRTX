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
    struct message_header hdr; /* MUST be first */
    char body[64];
};

struct FakeSource {
    FakeMessage entries[FAKE_SLOTS];
    size_t count;
    bool composeInvoked;
};

static size_t fake_count(void *ctx)
{
    return static_cast<FakeSource *>(ctx)->count;
}

static struct message_header *fake_get(void *ctx, size_t i)
{
    FakeSource *src = static_cast<FakeSource *>(ctx);
    if (i >= src->count)
        return nullptr;
    return &src->entries[i].hdr;
}

static uint32_t fake_supported_actions(const struct message_header * /*entry*/)
{
    return MSG_ACTION_VIEW | MSG_ACTION_DELETE | MSG_ACTION_REPLY
         | MSG_ACTION_MARK_READ | MSG_ACTION_MARK_UNREAD;
}

static int fake_invoke_action(struct message_header *entry,
                              enum message_action action)
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

/* tick() that reports no change — the common case for these tests, which
 * mutate FakeSource directly rather than through a real drain loop. */
static bool fake_tick(void *ctx)
{
    (void)ctx;
    return false;
}

static const struct message_ops fake_ops = {
    /* name              */ "FakeSrc",
    /* count             */ fake_count,
    /* get               */ fake_get,
    /* supported_actions */ fake_supported_actions,
    /* invoke_action     */ fake_invoke_action,
    /* start_compose     */ fake_start_compose,
    /* tick              */ fake_tick,
    /* send              */ nullptr,
    /* mode_id           */ 0u,
};

/* ------------------------------------------------------------------ */
/* Named callbacks for test-specific opss                           */
/* ------------------------------------------------------------------ */

/* restricted_ops: reports no supported actions */
static uint32_t restricted_supported(const struct message_header *)
{
    return 0u;
}

/* snapshot-cap ops */
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
static struct message_header *big_get(void * /*ctx*/, size_t i)
{
    if (i >= BIG_COUNT)
        return nullptr;
    return &bigBuf[i].hdr;
}

/* sparse-get ops: returns nullptr for index 1 */
static FakeMessage sparseEntries[3];
static size_t sparse_count(void *)
{
    return 3u;
}
static struct message_header *sparse_get(void *, size_t i)
{
    if (i == 1)
        return nullptr;
    return &sparseEntries[i].hdr;
}

/* tick() whose return value is controlled by a global for dirty-flag tests. */
static bool g_tick_returns_true = false;
static bool changing_tick(void *ctx)
{
    (void)ctx;
    return g_tick_returns_true;
}

/* send() that always succeeds — used for the send()-marks-dirty test. */
static int succeeding_send(void *ctx, const char *body, size_t body_len,
                           const char *recipient)
{
    (void)ctx;
    (void)body;
    (void)body_len;
    (void)recipient;
    return 0;
}

/* Helper: initialise a FakeSource and push n entries */
static void pushEntries(FakeSource &src, size_t n, int baseVal,
                        bool unread = true)
{
    for (size_t i = 0; i < n && src.count < FAKE_SLOTS; i++) {
        FakeMessage &m = src.entries[src.count];
        memset(&m, 0, sizeof(m));
        int val = baseVal + (int)i;
        m.hdr.sequence = static_cast<uint32_t>(val);
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

TEST_CASE("MessageRegistry: init and terminate with multiple sources",
          "[messages][registry]")
{
    FakeSource src1 = {};
    FakeSource src2 = {};

    SourceEntry entries[] = {
        { &fake_ops, &src1 },
        { &fake_ops, &src2 },
    };
    MessageRegistry reg(entries, 2);
    reg.tick();
    /* No entries yet, but both sources are active. */
    REQUIRE(reg.count() == 0);
}

TEST_CASE("MessageRegistry: push then enumerate finds entries",
          "[messages][registry]")
{
    FakeSource src = {};
    pushEntries(src, 3, 100);

    SourceEntry entries[] = { { &fake_ops, &src } };
    MessageRegistry reg(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 3);

    /* All entries should be accessible. */
    for (size_t i = 0; i < 3; i++)
        REQUIRE(reg.get(i) != nullptr);

    REQUIRE(reg.get(3) == nullptr);
}

TEST_CASE("MessageRegistry: merge across two sources sorted by sequence desc",
          "[messages][registry]")
{
    FakeSource srcA = {};
    FakeSource srcB = {};

    /* srcA: sequences 10, 12, 14 */
    pushEntries(srcA, 3, 10);
    srcA.entries[0].hdr.sequence = 10;
    srcA.entries[1].hdr.sequence = 12;
    srcA.entries[2].hdr.sequence = 14;

    /* srcB: sequences 11, 13, 15 */
    pushEntries(srcB, 3, 11);
    srcB.entries[0].hdr.sequence = 11;
    srcB.entries[1].hdr.sequence = 13;
    srcB.entries[2].hdr.sequence = 15;

    SourceEntry entries[] = {
        { &fake_ops, &srcA },
        { &fake_ops, &srcB },
    };
    MessageRegistry reg(entries, 2);

    reg.tick();
    REQUIRE(reg.count() == 6);

    /* Snapshot must be descending (newest first). */
    for (size_t i = 1; i < reg.count(); i++) {
        REQUIRE(reg.get(i - 1)->sequence >= reg.get(i)->sequence);
    }

    /* Highest sequence first. */
    REQUIRE(reg.get(0)->sequence == 15);
    REQUIRE(reg.get(reg.count() - 1)->sequence == 10);
}

TEST_CASE("MessageRegistry: MARK_READ flips unread via ops",
          "[messages][registry]")
{
    FakeSource src = {};
    pushEntries(src, 2, 200, /* unread= */ true);

    SourceEntry entries[] = { { &fake_ops, &src } };
    MessageRegistry reg(entries, 1);
    reg.tick();

    REQUIRE(reg.countUnread() == 2);

    int rc = reg.invokeAction(0, MSG_ACTION_MARK_READ);
    REQUIRE(rc == 0);
    REQUIRE(reg.get(0)->unread == false);

    /* invokeAction marked the snapshot dirty, so this tick rebuilds and
     * reflects the change. */
    reg.tick();
    REQUIRE(reg.countUnread() == 1);
}

TEST_CASE("MessageRegistry: DELETE invokes ops and entry vanishes after tick",
          "[messages][registry]")
{
    FakeSource src = {};
    pushEntries(src, 3, 300);

    SourceEntry entries[] = { { &fake_ops, &src } };
    MessageRegistry reg(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 3);

    /*
     * Simulate deletion: the fake ops's invoke_action() is a no-op on
     * storage; the test removes the entry from the fake source's storage
     * directly, as a real source's invoke_action() would.
     */
    int rc = reg.invokeAction(0, MSG_ACTION_DELETE);
    REQUIRE(rc == 0);

    /* Remove the entry from the fake source storage as well. */
    struct message_header *deleted_hdr = reg.get(0);
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
}

TEST_CASE(
    "MessageRegistry: unsupported action returns error without calling source",
    "[messages][registry]")
{
    /* Build a ops that reports no supported actions. */
    static const struct message_ops restricted_ops = {
        /* name              */ "Restricted",
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ restricted_supported,
        /* invoke_action     */ fake_invoke_action,
        /* start_compose     */ nullptr,
        /* tick              */ fake_tick,
        /* send              */ nullptr,
        /* mode_id           */ 0u,
    };

    FakeSource src = {};
    pushEntries(src, 1, 400);

    SourceEntry entries[] = { { &restricted_ops, &src } };
    MessageRegistry reg(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 1);

    /* Every action should be rejected. */
    REQUIRE(reg.invokeAction(0, MSG_ACTION_DELETE) == -ENOTSUP);
    REQUIRE(reg.invokeAction(0, MSG_ACTION_MARK_READ) == -ENOTSUP);
}

TEST_CASE("MessageRegistry: count_unread matches sum across sources",
          "[messages][registry]")
{
    FakeSource srcA = {};
    FakeSource srcB = {};

    pushEntries(srcA, 2, 500, /* unread= */ true);
    pushEntries(srcB, 3, 510, /* unread= */ false);

    SourceEntry entries[] = {
        { &fake_ops, &srcA },
        { &fake_ops, &srcB },
    };
    MessageRegistry reg(entries, 2);
    reg.tick();

    REQUIRE(reg.count() == 5);
    REQUIRE(reg.countUnread() == 2);
}

TEST_CASE(
    "MessageRegistry: canCompose returns true only when a matching source exists",
    "[messages][registry]")
{
    /* mode 1 source has compose; mode 2 source does not */
    static const struct message_ops ops_mode1 = {
        /* name              */ "SrcMode1",
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ fake_supported_actions,
        /* invoke_action     */ fake_invoke_action,
        /* start_compose     */ fake_start_compose,
        /* tick              */ fake_tick,
        /* send              */ nullptr,
        /* mode_id           */ 1u,
    };
    static const struct message_ops ops_mode2_nocompose = {
        /* name              */ nullptr,
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ fake_supported_actions,
        /* invoke_action     */ fake_invoke_action,
        /* start_compose     */ nullptr,
        /* tick              */ fake_tick,
        /* send              */ nullptr,
        /* mode_id           */ 2u,
    };

    FakeSource srcA = {};
    FakeSource srcB = {};

    SourceEntry entries[] = {
        { &ops_mode1, &srcA },
        { &ops_mode2_nocompose, &srcB },
    };
    MessageRegistry reg(entries, 2);

    REQUIRE(reg.canCompose(1) == true);  /* mode 1 has compose */
    REQUIRE(reg.canCompose(2) == false); /* mode 2 has no start_compose */
    REQUIRE(reg.canCompose(3) == false); /* no source for mode 3 */
    REQUIRE(reg.canCompose(0) == false); /* OPMODE_NONE never matches */
}

TEST_CASE("MessageRegistry: startCompose dispatches to source matching mode",
          "[messages][registry]")
{
    static const struct message_ops ops_mode1 = {
        /* name              */ "SrcMode1",
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ fake_supported_actions,
        /* invoke_action     */ fake_invoke_action,
        /* start_compose     */ fake_start_compose,
        /* tick              */ fake_tick,
        /* send              */ nullptr,
        /* mode_id           */ 1u,
    };
    static const struct message_ops ops_mode2 = {
        /* name              */ "SrcMode2",
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ fake_supported_actions,
        /* invoke_action     */ fake_invoke_action,
        /* start_compose     */ fake_start_compose,
        /* tick              */ fake_tick,
        /* send              */ nullptr,
        /* mode_id           */ 2u,
    };

    FakeSource srcA = {};
    FakeSource srcB = {};

    SourceEntry entries[] = {
        { &ops_mode1, &srcA },
        { &ops_mode2, &srcB },
    };
    MessageRegistry reg(entries, 2);

    /* startCompose(2) should invoke srcB (mode 2), not srcA (mode 1). */
    REQUIRE(reg.startCompose(2) == 0);
    REQUIRE(srcA.composeInvoked == false);
    REQUIRE(srcB.composeInvoked == true);

    /* No source for mode 99. */
    REQUIRE(reg.startCompose(99) == -ENOENT);

    /* OPMODE_NONE (0) always returns -ENOENT. */
    REQUIRE(reg.startCompose(0) == -ENOENT);
}

TEST_CASE("MessageRegistry: snapshot cap handled gracefully",
          "[messages][registry]")
{
    /* Fill a source with enough entries to exceed the snapshot cap. */
    memset(bigBuf, 0, sizeof(bigBuf));

    for (size_t i = 0; i < bigSrc.count; i++) {
        bigBuf[i].hdr.sequence = static_cast<uint32_t>(i);
        bigBuf[i].hdr.unread = true;
    }

    static const struct message_ops big_ops = {
        /* name              */ nullptr,
        /* count             */ big_count,
        /* get               */ big_get,
        /* supported_actions */ nullptr,
        /* invoke_action     */ nullptr,
        /* start_compose     */ nullptr,
        /* tick              */ fake_tick,
        /* send              */ nullptr,
        /* mode_id           */ 0u,
    };

    SourceEntry entries[] = { { &big_ops, &bigSrc } };
    MessageRegistry reg(entries, 1);
    reg.tick(); /* must not crash or overflow */

    REQUIRE(reg.count() == MessageRegistry::MAX_MESSAGES_SNAPSHOT);
}

TEST_CASE("MessageRegistry: source get() returning nullptr is skipped",
          "[messages][registry]")
{
    /* A source with 3 logical entries but get() returns nullptr for index 1. */
    memset(sparseEntries, 0, sizeof(sparseEntries));
    sparseEntries[0].hdr.sequence = 10;
    sparseEntries[2].hdr.sequence = 30;

    static const struct message_ops sparse_ops = {
        /* name              */ nullptr,
        /* count             */ sparse_count,
        /* get               */ sparse_get,
        /* supported_actions */ nullptr,
        /* invoke_action     */ nullptr,
        /* start_compose     */ nullptr,
        /* tick              */ fake_tick,
        /* send              */ nullptr,
        /* mode_id           */ 0u,
    };

    SourceEntry entries[] = { { &sparse_ops, nullptr } };
    MessageRegistry reg(entries, 1);
    reg.tick();

    /* Only 2 of 3 entries should appear (nullptr entry skipped). */
    REQUIRE(reg.count() == 2);
    REQUIRE(reg.get(0)->sequence == 30);
    REQUIRE(reg.get(1)->sequence == 10);
}

TEST_CASE(
    "MessageRegistry: invokeAction with out-of-range index returns -ENOENT",
    "[messages][registry]")
{
    FakeSource src = {};
    pushEntries(src, 2, 100);

    SourceEntry entries[] = { { &fake_ops, &src } };
    MessageRegistry reg(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 2);

    REQUIRE(reg.invokeAction(2, MSG_ACTION_MARK_READ) == -ENOENT);
    REQUIRE(reg.invokeAction(99, MSG_ACTION_MARK_READ) == -ENOENT);
}

TEST_CASE(
    "MessageRegistry: invokeAction returns -ENOTSUP when invoke_action is null",
    "[messages][registry]")
{
    /* Ops with supported_actions=nullptr and invoke_action=nullptr. */
    static const struct message_ops null_invoke_ops = {
        /* name              */ nullptr,
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ nullptr,
        /* invoke_action     */ nullptr,
        /* start_compose     */ nullptr,
        /* tick              */ fake_tick,
        /* send              */ nullptr,
        /* mode_id           */ 0u,
    };

    FakeSource src = {};
    pushEntries(src, 1, 200);

    SourceEntry entries[] = { { &null_invoke_ops, &src } };
    MessageRegistry reg(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 1);

    REQUIRE(reg.invokeAction(0, MSG_ACTION_MARK_READ) == -ENOTSUP);
}

TEST_CASE("MessageRegistry: sourceMode returns mode_id of the producing source",
          "[messages][registry]")
{
    static const struct message_ops ops_mode1 = {
        /* name              */ nullptr,
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ nullptr,
        /* invoke_action     */ nullptr,
        /* start_compose     */ nullptr,
        /* tick              */ fake_tick,
        /* send              */ nullptr,
        /* mode_id           */ 1u,
    };
    static const struct message_ops ops_mode3 = {
        /* name              */ nullptr,
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ nullptr,
        /* invoke_action     */ nullptr,
        /* start_compose     */ nullptr,
        /* tick              */ fake_tick,
        /* send              */ nullptr,
        /* mode_id           */ 3u,
    };

    FakeSource srcA = {};
    FakeSource srcB = {};

    /* srcA: second=10; srcB: second=20 → srcB is newer, sorts first */
    pushEntries(srcA, 1, 10);
    pushEntries(srcB, 1, 20);

    SourceEntry entries[] = {
        { &ops_mode1, &srcA },
        { &ops_mode3, &srcB },
    };
    MessageRegistry reg(entries, 2);
    reg.tick();

    REQUIRE(reg.count() == 2);
    /* Snapshot is newest-first: srcB (mode 3) at 0, srcA (mode 1) at 1. */
    REQUIRE(reg.sourceMode(0) == 3u);
    REQUIRE(reg.sourceMode(1) == 1u);
    REQUIRE(reg.sourceMode(99) == 0u); /* out of range → 0 */
}

/* ------------------------------------------------------------------ */
/* Double-buffer semantics                                             */
/* ------------------------------------------------------------------ */

TEST_CASE("MessageRegistry: double-buffer keeps old pointers valid until "
          "the next rebuild",
          "[messages][registry]")
{
    FakeSource src = {};
    pushEntries(src, 2, 100);

    SourceEntry entries[] = { { &fake_ops, &src } };
    MessageRegistry reg(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 2);

    /* Hold pointers and their pointed-to values from the first snapshot. */
    struct message_header *ptr0 = reg.get(0);
    struct message_header *ptr1 = reg.get(1);
    uint32_t seq0 = ptr0->sequence;
    uint32_t seq1 = ptr1->sequence;

    /* Mutate the source (add a third entry). fake_tick() reports no change
     * and the registry was not explicitly marked dirty, so tick() must
     * skip the rebuild and the old pointers/values must remain intact. */
    pushEntries(src, 1, 200);
    REQUIRE(reg.tick() == false);
    REQUIRE(reg.count() == 2);
    REQUIRE(reg.get(0) == ptr0);
    REQUIRE(reg.get(1) == ptr1);
    REQUIRE(ptr0->sequence == seq0);
    REQUIRE(ptr1->sequence == seq1);

    /* Now mark dirty and rebuild: the new entry becomes visible. */
    reg.markDirty();
    REQUIRE(reg.tick() == true);
    REQUIRE(reg.count() == 3);
}

/* ------------------------------------------------------------------ */
/* Dirty-flag propagation                                              */
/* ------------------------------------------------------------------ */

TEST_CASE("MessageRegistry: a source's tick() return value drives rebuild",
          "[messages][registry]")
{
    static const struct message_ops ops_changing = {
        /* name              */ "Changing",
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ fake_supported_actions,
        /* invoke_action     */ fake_invoke_action,
        /* start_compose     */ nullptr,
        /* tick              */ changing_tick,
        /* send              */ nullptr,
        /* mode_id           */ 0u,
    };

    FakeSource src = {};
    pushEntries(src, 2, 300);

    SourceEntry entries[] = { { &ops_changing, &src } };
    g_tick_returns_true = false;
    MessageRegistry reg(entries, 1);

    /* First tick: dirty=true from init() → rebuild regardless of tick(). */
    REQUIRE(reg.tick() == true);

    /* Second tick: not dirty, source reports no change → skip. */
    REQUIRE(reg.tick() == false);

    /* Third tick: source reports a change → forced rebuild even though the
     * registry's own dirty flag is still clear. */
    g_tick_returns_true = true;
    REQUIRE(reg.tick() == true);

    /* Reset the shared global for other tests. */
    g_tick_returns_true = false;
}

TEST_CASE("MessageRegistry: invokeAction marks dirty only on success",
          "[messages][registry]")
{
    FakeSource src = {};
    pushEntries(src, 2, 400);

    SourceEntry entries[] = { { &fake_ops, &src } };
    MessageRegistry reg(entries, 1);

    REQUIRE(reg.tick() == true);  /* dirty from init */
    REQUIRE(reg.tick() == false); /* nothing changed */

    int rc = reg.invokeAction(0, MSG_ACTION_MARK_READ);
    REQUIRE(rc == 0);

    REQUIRE(reg.tick() == true); /* dirty from the successful action */
}

TEST_CASE("MessageRegistry: send marks dirty only on success",
          "[messages][registry]")
{
    static const struct message_ops ops_send = {
        /* name              */ "Sendable",
        /* count             */ fake_count,
        /* get               */ fake_get,
        /* supported_actions */ nullptr,
        /* invoke_action     */ nullptr,
        /* start_compose     */ nullptr,
        /* tick              */ fake_tick,
        /* send              */ succeeding_send,
        /* mode_id           */ 1u,
    };

    FakeSource src = {};
    pushEntries(src, 1, 500);

    SourceEntry entries[] = { { &ops_send, &src } };
    MessageRegistry reg(entries, 1);

    REQUIRE(reg.tick() == true);  /* dirty from init */
    REQUIRE(reg.tick() == false); /* nothing changed */

    int rc = reg.send(1u, "hello", 5, "DEST");
    REQUIRE(rc == 0);

    REQUIRE(reg.tick() == true); /* dirty from the successful send */

    /* No source for mode 99. */
    REQUIRE(reg.send(99u, "hello", 5, "DEST") == -ENOENT);

    /* OPMODE_NONE (0) always returns -ENOENT. */
    REQUIRE(reg.send(0u, "hello", 5, "DEST") == -ENOENT);
}

/* ------------------------------------------------------------------ */
/* findBySequence / supportedActions                                   */
/* ------------------------------------------------------------------ */

TEST_CASE("MessageRegistry: findBySequence returns correct index",
          "[messages][registry]")
{
    FakeSource src = {};
    pushEntries(src, 3, 100);

    SourceEntry entries[] = { { &fake_ops, &src } };
    MessageRegistry reg(entries, 1);
    reg.tick();

    /* Entries have sequences 100, 101, 102; snapshot is newest-first. */
    REQUIRE(reg.findBySequence(102) == 0);
    REQUIRE(reg.findBySequence(101) == 1);
    REQUIRE(reg.findBySequence(100) == 2);
    REQUIRE(reg.findBySequence(999) == SIZE_MAX);
}

TEST_CASE("MessageRegistry: supportedActions returns bitmap from source",
          "[messages][registry]")
{
    FakeSource src = {};
    pushEntries(src, 1, 200);

    SourceEntry entries[] = { { &fake_ops, &src } };
    MessageRegistry reg(entries, 1);
    reg.tick();
    REQUIRE(reg.count() == 1);

    uint32_t actions = reg.supportedActions(0);
    REQUIRE((actions & MSG_ACTION_VIEW) != 0);
    REQUIRE((actions & MSG_ACTION_DELETE) != 0);
    REQUIRE((actions & MSG_ACTION_REPLY) != 0);

    /* Out of range returns 0. */
    REQUIRE(reg.supportedActions(99) == 0);
}

/* ------------------------------------------------------------------ */
/* New-unread high-water mark                                          */
/*                                                                     */
/* messages_tick() (the C facade in messages.cpp) computes this exact  */
/* prev/tick/new delta over the singleton registry and its compile-    */
/* time-fixed source table, so it cannot itself be exercised with a    */
/* test-controlled source — there is no way to register a FakeSource   */
/* with the production singleton.  These tests instead verify the      */
/* delta formula against MessageRegistry directly, which is the only   */
/* thing messages_tick() adds on top of tick()/countUnread().          */
/* ------------------------------------------------------------------ */

TEST_CASE("MessageRegistry: countUnread delta captures a new arrival",
          "[messages][registry]")
{
    FakeSource src = {};
    pushEntries(src, 1, 100, /* unread= */ true);

    SourceEntry entries[] = { { &fake_ops, &src } };
    MessageRegistry reg(entries, 1);

    size_t prev_unread = reg.countUnread();
    reg.tick();
    size_t new_unread = reg.countUnread();
    REQUIRE(prev_unread == 0);
    REQUIRE(new_unread == 1);
    REQUIRE(new_unread - prev_unread == 1); /* what messages_tick() returns */

    /* A second entry arrives before the next tick. */
    pushEntries(src, 1, 200, /* unread= */ true);
    prev_unread = reg.countUnread(); /* still 1: snapshot not yet rebuilt */
    reg.markDirty();
    reg.tick();
    new_unread = reg.countUnread();
    REQUIRE(prev_unread == 1);
    REQUIRE(new_unread == 2);
    REQUIRE(new_unread - prev_unread == 1);
}

TEST_CASE("MessageRegistry: countUnread delta is zero-clamped when unread "
          "count falls or holds steady",
          "[messages][registry]")
{
    FakeSource src = {};
    pushEntries(src, 2, 300, /* unread= */ true);

    SourceEntry entries[] = { { &fake_ops, &src } };
    MessageRegistry reg(entries, 1);
    reg.tick();
    REQUIRE(reg.countUnread() == 2);

    /* Mark one read: unread count falls from 2 to 1. The facade's
     * (new > prev) ? new - prev : 0 formula must clamp this to 0, not
     * underflow as an unsigned subtraction would. */
    size_t prev_unread = reg.countUnread();
    reg.invokeAction(0, MSG_ACTION_MARK_READ);
    reg.tick();
    size_t new_unread = reg.countUnread();
    REQUIRE(prev_unread == 2);
    REQUIRE(new_unread == 1);
    REQUIRE((new_unread > prev_unread ? new_unread - prev_unread : 0) == 0);

    /* Steady state: no arrival, no read — delta is 0. */
    prev_unread = reg.countUnread();
    reg.tick();
    new_unread = reg.countUnread();
    REQUIRE(new_unread == prev_unread);
}

TEST_CASE("MessageRegistry: simultaneous arrival and read can undercount "
          "(documented tradeoff)",
          "[messages][registry]")
{
    /* If one unread entry is marked read in the same tick that a new
     * unread entry arrives, the net unread count is unchanged and the
     * high-water-mark formula reports zero new arrivals, even though one
     * did arrive. This is an accepted, documented tradeoff (see
     * messages_tick()'s doc comment) over the previous implementation,
     * which could miss arrivals entirely whenever net unread was flat or
     * falling — this at least fires whenever unread strictly increases. */
    FakeSource src = {};
    pushEntries(src, 1, 400, /* unread= */ true);

    SourceEntry entries[] = { { &fake_ops, &src } };
    MessageRegistry reg(entries, 1);
    reg.tick();
    REQUIRE(reg.countUnread() == 1);

    size_t prev_unread = reg.countUnread();
    reg.invokeAction(0, MSG_ACTION_MARK_READ);    /* unread: 1 -> 0 */
    pushEntries(src, 1, 500, /* unread= */ true); /* unread: 0 -> 1 */
    reg.tick();
    size_t new_unread = reg.countUnread();

    REQUIRE(prev_unread == 1);
    REQUIRE(new_unread == 1);
    REQUIRE((new_unread > prev_unread ? new_unread - prev_unread : 0) == 0);
}

/* ------------------------------------------------------------------ */
/* C facade smoke test                                                 */
/* ------------------------------------------------------------------ */

TEST_CASE("messages_alloc_sequence returns monotonically increasing values",
          "[messages][facade]")
{
    uint32_t seq1 = messages_alloc_sequence();
    uint32_t seq2 = messages_alloc_sequence();
    REQUIRE(seq2 > seq1);
    REQUIRE(seq1 != 0); /* 0 is reserved as "unset" */
}
