/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * messages_demo.c — Compile-time demo source for the message inbox.
 *
 * Enabled only when CONFIG_MESSAGES_DEMO is defined (Linux builds).
 * Provides three canned entries so the MESSAGES_LIST screen is
 * non-empty when running the Linux emulator without real radio traffic.
 *
 * The vtable and context pointer are declared in messages_demo.h and
 * referenced by the static source table in messages.cpp — no runtime
 * registration is required.
 */

#ifdef CONFIG_MESSAGES_DEMO

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "core/messages.h"
#include "core/messages_demo.h"

/* -------------------------------------------------------------------
 * Demo message storage
 * ----------------------------------------------------------------- */

typedef struct {
    message_header_t hdr;
    char body[64];
    bool deleted;
} demo_message_t;

#define DEMO_COUNT 3

static demo_message_t demo_msgs[] = {
    {
        .hdr = {
            .timestamp = { .hour = 22, .minute = 13, .second = 22,
                           .day  = 3,  .date   = 14, .month  = 11,
                           .year = 23 },
            .direction = MSG_DIR_RX,
            .status    = MSG_STATUS_RECEIVED,
            .unread    = true,
            .sender    = "KD0OSS",
        },
        .body    = "Hello from the inbox demo!",
        .deleted = false,
    },
    {
        .hdr = {
            .timestamp = { .hour = 22, .minute = 13, .second = 21,
                           .day  = 3,  .date   = 14, .month  = 11,
                           .year = 23 },
            .direction = MSG_DIR_TX,
            .status    = MSG_STATUS_SENT,
            .unread    = false,
            .sender    = "N0CALL",
            .recipient = "KD0OSS",
        },
        .body    = "This is a sent message.",
        .deleted = false,
    },
    {
        .hdr = {
            .timestamp = { .hour = 22, .minute = 13, .second = 20,
                           .day  = 3,  .date   = 14, .month  = 11,
                           .year = 23 },
            .direction = MSG_DIR_RX,
            .status    = MSG_STATUS_RECEIVED,
            .unread    = true,
            .sender    = "W1AW",
        },
        .body    = "Older unread, reply supported.",
        .deleted = false,
    },
};

/* -------------------------------------------------------------------
 * Vtable callbacks
 * ----------------------------------------------------------------- */

/* Wire body pointers once (static initializers can't self-reference). */
static bool demo_bodies_wired = false;

static void demo_wire_bodies(void)
{
    if (demo_bodies_wired)
        return;
    for (size_t i = 0; i < DEMO_COUNT; i++) {
        demo_msgs[i].hdr.body = demo_msgs[i].body;
        demo_msgs[i].hdr.body_len = strlen(demo_msgs[i].body);
    }
    demo_bodies_wired = true;
}

static size_t demo_count(void *ctx)
{
    demo_wire_bodies();
    size_t n = 0;
    (void)ctx;
    for (size_t i = 0; i < DEMO_COUNT; i++) {
        if (!demo_msgs[i].deleted)
            n++;
    }
    return n;
}

/* Logical index → physical index helper */
static int demo_phys(size_t logical)
{
    size_t found = 0;
    for (size_t i = 0; i < DEMO_COUNT; i++) {
        if (demo_msgs[i].deleted)
            continue;
        if (found == logical)
            return (int)i;
        found++;
    }
    return -1;
}

static message_header_t *demo_get(void *ctx, size_t idx)
{
    (void)ctx;
    int p = demo_phys(idx);
    if (p < 0)
        return NULL;
    return &demo_msgs[p].hdr;
}

static uint32_t demo_supported(const message_header_t *entry)
{
    uint32_t base = MSG_ACTION_VIEW | MSG_ACTION_DELETE | MSG_ACTION_MARK_READ;

    /* Incoming entries support Reply. */
    if (entry->direction == MSG_DIR_RX)
        base |= MSG_ACTION_REPLY;

    return base;
}

static int demo_invoke(message_header_t *entry, message_action_t action)
{
    switch (action) {
        case MSG_ACTION_MARK_READ:
            entry->unread = false;
            return 0;
        case MSG_ACTION_MARK_UNREAD:
            entry->unread = true;
            return 0;
        case MSG_ACTION_DELETE:
            ((demo_message_t *)entry)->deleted = true;
            return 0;
        case MSG_ACTION_REPLY:
            fprintf(stderr, "[demo] reply requested for sender '%s'\n",
                    entry->sender);
            return 0;
        default:
            return -ENOTSUP;
    }
}

static void demo_start_compose(void *ctx)
{
    (void)ctx;
    fprintf(stderr, "[demo] start_compose invoked\n");
}

/* -------------------------------------------------------------------
 * Exported vtable and context — referenced by messages.cpp
 * ----------------------------------------------------------------- */

const message_type_vtable_t messages_demo_vtable = {
    .name = "Demo",
    .count = demo_count,
    .get = demo_get,
    .supported_actions = demo_supported,
    .invoke_action = demo_invoke,
    .start_compose = demo_start_compose,
    .mode_id = 0, /* OPMODE_NONE: demo has no real mode */
};

/* Context is unused by this source; expose a NULL pointer. */
void *messages_demo_ctx = NULL;

#endif /* CONFIG_MESSAGES_DEMO */
