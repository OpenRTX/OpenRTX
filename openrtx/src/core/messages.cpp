/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hwconfig.h"

#include "core/messages.h"
#include "core/MessageRegistry.hpp"
#include <new>

#ifdef CONFIG_MESSAGES_DEMO
#include "core/messages_demo.h"
#endif

/* ------------------------------------------------------------------
 * Compile-time source table.
 *
 * Add one SourceEntry per protocol, guarded by its CONFIG_* flag.
 * The ops and context pointers must remain valid for the entire
 * lifetime of the registry (i.e. until messages_terminate() returns).
 *
 * The trailing { nullptr, nullptr } sentinel exists solely so this array
 * is never zero-length when no optional source is compiled in (e.g. every
 * target before CONFIG_MESSAGES_DEMO or a protocol source is enabled): a
 * zero-size array is a GNU extension tolerated by clang (used for the
 * linux/host build) but rejected as a hard error by arm-miosix-eabi-g++
 * (used for cm4/cm7 targets).  MessageRegistry already skips any entry
 * whose vtable is NULL, so the sentinel is completely inert — it costs one
 * harmless iteration in tick()'s source loop, nothing more.
 * ------------------------------------------------------------------ */

static const SourceEntry sources[] = {
#ifdef CONFIG_MESSAGES_DEMO
    { &messages_demo_vtable, &messages_demo_ctx },
#endif
    { nullptr, nullptr }, /* sentinel — keep last, see comment above */
};

static constexpr size_t NUM_SOURCES = sizeof(sources) / sizeof(sources[0]);

static_assert(NUM_SOURCES <= 255,
              "NUM_SOURCES must fit in uint8_t for SnapshotEntry::source_idx");

#ifdef CONFIG_MSG_SNAPSHOT_SIZE
static_assert(CONFIG_MSG_SNAPSHOT_SIZE > 0,
              "CONFIG_MSG_SNAPSHOT_SIZE must be positive");
#endif

/* Singleton registry instance.
 *
 * The registry has no default constructor (it is built over the source table),
 * and its lifetime is driven explicitly by messages_init()/messages_terminate()
 * from state.c — not by static init/teardown.  So it lives in raw aligned
 * storage and is constructed/destroyed in place via placement-new, avoiding
 * both an early global constructor on embedded targets and any heap use. */
alignas(MessageRegistry) static unsigned char registry_storage[sizeof(
    MessageRegistry)];
static MessageRegistry *registry = nullptr;

uint32_t messages_alloc_sequence(void)
{
    /* Simple monotonic counter — sufficient as both sort key and identity.
     * Sequence 0 is never allocated so callers may treat it as "unset". */
    static uint32_t next_seq = 1;
    return next_seq++;
}

size_t messages_find_by_sequence(uint32_t seq)
{
    return registry->findBySequence(seq);
}

uint32_t messages_supported_actions(size_t idx)
{
    return registry->supportedActions(idx);
}

void messages_init(void)
{
    registry = new (registry_storage) MessageRegistry(sources, NUM_SOURCES);
}

void messages_terminate(void)
{
    if (registry != nullptr) {
        registry->~MessageRegistry();
        registry = nullptr;
    }
}

size_t messages_tick(void)
{
    size_t prev_unread = registry->countUnread();
    bool rebuilt = registry->tick();

    /* The snapshot can't have changed when tick() skipped the rebuild, so
     * unread count can't have changed either — skip the redundant walk on
     * this (common, steady-state) path. */
    if (!rebuilt)
        return 0;

    size_t new_unread = registry->countUnread();

    /* High-water mark: how many more unread entries exist now than before
     * this tick.  Deletions/mark-read in the same tick as an arrival will
     * undercount, which is an accepted tradeoff over the previous simple
     * delta (which could miss arrivals entirely when net unread was flat). */
    if (new_unread > prev_unread)
        return new_unread - prev_unread;

    return 0;
}

size_t messages_count(void)
{
    return registry->count();
}

size_t messages_count_unread(void)
{
    return registry->countUnread();
}

struct message_header *messages_get(size_t idx)
{
    return registry->get(idx);
}

int messages_invoke_action(size_t idx, enum message_action action)
{
    return registry->invokeAction(idx, action);
}

bool messages_can_compose(uint8_t mode)
{
    return registry->canCompose(mode);
}

int messages_start_compose(uint8_t mode)
{
    return registry->startCompose(mode);
}

uint8_t messages_source_mode(size_t idx)
{
    return registry->sourceMode(idx);
}

int messages_send(uint8_t mode, const char *body, size_t body_len,
                  const char *recipient)
{
    return registry->send(mode, body, body_len, recipient);
}
