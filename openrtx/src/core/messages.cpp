/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/messages.h"
#include "core/MessageRegistry.hpp"

#ifdef CONFIG_MESSAGES_DEMO
#include "core/messages_demo.h"
#endif

#include <cerrno>

/* ------------------------------------------------------------------
 * Compile-time source table.
 *
 * Add one SourceEntry per protocol, guarded by its CONFIG_* flag.
 * The vtable and context pointers must remain valid for the entire
 * lifetime of the registry (i.e. until messages_terminate() returns).
 * ------------------------------------------------------------------ */

static const SourceEntry sources[] = {
#ifdef CONFIG_MESSAGES_DEMO
    { &messages_demo_vtable, &messages_demo_ctx },
#endif
    /* Sentinel: at least one entry to avoid a zero-length array when all
     * CONFIG_* guards are false (e.g. Module17 which has no SMS source).
     * NUM_SOURCES counts only the real entries before this position;
     * the registry ignores the sentinel because its vtable pointer is NULL.
     * IMPORTANT: always keep this as the last entry — never insert real
     * sources after it or NUM_SOURCES will under-count. */
    { NULL, NULL },
};

/* Subtract sentinel entry from count. */
static constexpr size_t NUM_SOURCES = sizeof(sources) / sizeof(sources[0]) - 1u;

#ifdef CONFIG_MSG_SNAPSHOT_SIZE
static_assert(CONFIG_MSG_SNAPSHOT_SIZE >= 0,
              "Set CONFIG_MSG_SNAPSHOT_SIZE in hwconfig.h");
#endif

/* Singleton registry instance. */
static MessageRegistry registry;

void messages_init(void)
{
    registry.init(sources, NUM_SOURCES);
}

void messages_terminate(void)
{
    registry.terminate();
}

void messages_tick(void)
{
    registry.tick();
}

size_t messages_count(void)
{
    return registry.count();
}

size_t messages_count_unread(void)
{
    return registry.countUnread();
}

message_header_t *messages_get(size_t idx)
{
    return registry.get(idx);
}

int messages_invoke_action(size_t idx, message_action_t action)
{
    return registry.invokeAction(idx, action);
}

bool messages_can_compose(uint8_t mode)
{
    return registry.canCompose(mode);
}

int messages_start_compose(uint8_t mode)
{
    return registry.startCompose(mode);
}

uint8_t messages_source_mode(size_t idx)
{
    return registry.sourceMode(idx);
}
