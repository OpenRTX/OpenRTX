/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "core/messages.h"
#include "core/utils.h"
#include "rtx/rtx.h"

/* Maximum number of entries held in the inbox. */
#ifndef CONFIG_MESSAGES_MAX_ENTRIES
#define CONFIG_MESSAGES_MAX_ENTRIES 32
#endif

/* Size of the shared body pool, in bytes. */
#ifndef CONFIG_MESSAGES_POOL_BYTES
#define CONFIG_MESSAGES_POOL_BYTES 3200
#endif

/* Maximum number of protocol sources that can be registered. */
#ifndef CONFIG_MESSAGES_MAX_SOURCES
#define CONFIG_MESSAGES_MAX_SOURCES 2
#endif

/* Number of receive descriptors kept submitted to the rtx stage. */
#ifndef CONFIG_MESSAGES_RX_SLOTS
#define CONFIG_MESSAGES_RX_SLOTS 1
#endif

_Static_assert(CONFIG_MESSAGES_MAX_ENTRIES > 0,
               "CONFIG_MESSAGES_MAX_ENTRIES must be positive");
_Static_assert(CONFIG_MESSAGES_POOL_BYTES >= MSG_BODY_MAX_LEN,
               "CONFIG_MESSAGES_POOL_BYTES must hold at least one full body");

/**
 * \internal
 * A packet descriptor with its buffer and the operating mode it was
 * submitted in, which selects the source that handles it on completion.
 */
struct pktSlot {
    struct pktDesc desc;
    uint8_t mode;
    uint8_t buffer[MSG_PKT_MAX_SIZE];
};

/**
 * \internal
 * Everything the inbox needs while active, allocated as a single block.
 *
 * The entry table is a ring: head is the position of the oldest entry and
 * new entries are appended after the newest, so evicting the oldest is just
 * advancing head. Bodies live in the pool as NUL-terminated strings
 * allocated in arrival order: poolHead is the next byte to write and wraps
 * to zero when a body would not fit at the end.
 */
struct messageCtx {
    size_t head;
    size_t numEntries;
    size_t newArrivals;
    size_t poolHead;
    uint32_t txSequence;
    struct message entries[CONFIG_MESSAGES_MAX_ENTRIES];
    char pool[CONFIG_MESSAGES_POOL_BYTES];
    struct pktSlot rxSlots[CONFIG_MESSAGES_RX_SLOTS];
    struct pktSlot txSlot;
};

static const struct messageOps *sources[CONFIG_MESSAGES_MAX_SOURCES];
static size_t numSources;
static struct messageCtx *ctx;
static uint32_t nextSequence;

/**
 * \internal
 * Get the source registered for an operating mode, NULL if none.
 */
static const struct messageOps *findSource(uint8_t mode)
{
    for (size_t i = 0; i < numSources; i++) {
        if (sources[i]->mode == mode)
            return sources[i];
    }

    return NULL;
}

/**
 * \internal
 * Get an entry from its age: position 0 is the oldest entry stored,
 * position numEntries - 1 the newest.
 *
 * @param pos: position of the entry, oldest first.
 * @return pointer to the entry.
 */
static struct message *entryAt(size_t pos)
{
    return &ctx->entries[(ctx->head + pos) % CONFIG_MESSAGES_MAX_ENTRIES];
}

/**
 * \internal
 * Remove an entry, keeping the remaining entries in order. Removing the
 * oldest entry, the only case eviction ever needs, just advances the ring
 * head; removing a newer one shifts the entries behind it.
 *
 * @param pos: position of the entry, oldest first.
 */
static void removeEntry(size_t pos)
{
    if (pos == 0) {
        ctx->head = (ctx->head + 1) % CONFIG_MESSAGES_MAX_ENTRIES;
        ctx->numEntries--;
        return;
    }

    for (size_t i = pos; i + 1 < ctx->numEntries; i++)
        *entryAt(i) = *entryAt(i + 1);

    ctx->numEntries--;
}

/**
 * \internal
 * Check whether any stored body overlaps a range of the pool.
 *
 * @param start: first byte of the range.
 * @param end: one past the last byte of the range.
 * @return true if some body overlaps the range.
 */
static bool poolInUse(size_t start, size_t end)
{
    for (size_t i = 0; i < ctx->numEntries; i++) {
        const struct message *entry = entryAt(i);
        size_t bodyStart = entry->body - ctx->pool;
        size_t bodyEnd = bodyStart + entry->bodyLen + 1;

        if ((bodyStart < end) && (bodyEnd > start))
            return true;
    }

    return false;
}

/**
 * \internal
 * Reserve room in the pool for a body plus NUL, evicting the oldest entries
 * until nothing overlaps the reserved range. The pool is written in arrival
 * order, so the overlapping bodies are among the oldest, but after a wrap an
 * older body may survive at the end of the pool: evicting in age order
 * keeps eviction strictly oldest-first at the cost of an entry or two.
 *
 * @param len: body length, not counting the NUL.
 * @return pool offset of the reserved range.
 */
static size_t poolAlloc(size_t len)
{
    size_t need = len + 1;

    if (ctx->poolHead + need > CONFIG_MESSAGES_POOL_BYTES)
        ctx->poolHead = 0;

    size_t start = ctx->poolHead;
    size_t end = ctx->poolHead + need;

    while (poolInUse(start, end))
        removeEntry(0);

    ctx->poolHead = end;
    return start;
}

/**
 * \internal
 * Allocate and reset the storage. Fails silently if the heap is exhausted:
 * the next task run tries again.
 */
static void allocate(void)
{
    ctx = malloc(sizeof(*ctx));
    if (ctx == NULL)
        return;

    ctx->head = 0;
    ctx->numEntries = 0;
    ctx->newArrivals = 0;
    ctx->txSequence = 0;
    ctx->poolHead = 0;

    for (size_t i = 0; i < ARRAY_SIZE(ctx->rxSlots); i++)
        ctx->rxSlots[i].desc.status = PKT_STATUS_IDLE;

    ctx->txSlot.desc.status = PKT_STATUS_IDLE;
}

/**
 * \internal
 * Check whether the rtx stage still holds any packet descriptor: the storage
 * cannot be released before every one has been handed back.
 */
static bool slotsBusy(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(ctx->rxSlots); i++) {
        if (ctx->rxSlots[i].desc.status == PKT_STATUS_SUBMITTED)
            return true;
    }

    return ctx->txSlot.desc.status == PKT_STATUS_SUBMITTED;
}

/**
 * \internal
 * Drive one receive slot: hand a completed packet to its source, then keep
 * the slot submitted while a source is registered for the current mode.
 */
static void handleRx(struct pktSlot *slot, const struct messageOps *ops,
                     uint8_t mode)
{
    const struct messageOps *owner;

    switch (slot->desc.status) {
        case PKT_STATUS_DONE:
            owner = findSource(slot->mode);
            if (owner != NULL)
                owner->processRx(&slot->desc);

            /* fallthrough */
        case PKT_STATUS_ERROR:
            slot->desc.status = PKT_STATUS_IDLE;

            /* fallthrough */
        case PKT_STATUS_IDLE:
            if (ops == NULL)
                break;

            slot->desc.buffer = slot->buffer;
            slot->desc.size = sizeof(slot->buffer);
            slot->mode = mode;
            rtx_addPacketRx(&slot->desc);
            break;

        case PKT_STATUS_SUBMITTED:
            break;
    }
}

/**
 * \internal
 * Report the outcome of a completed transmission to its entry and free the
 * transmit slot.
 */
static void handleTx(void)
{
    switch (ctx->txSlot.desc.status) {
        case PKT_STATUS_DONE:
            messages_setStatus(ctx->txSequence, MSG_STATUS_SENT);
            ctx->txSlot.desc.status = PKT_STATUS_IDLE;
            break;

        case PKT_STATUS_ERROR:
            messages_setStatus(ctx->txSequence, MSG_STATUS_FAILED);
            ctx->txSlot.desc.status = PKT_STATUS_IDLE;
            break;

        case PKT_STATUS_IDLE:
        case PKT_STATUS_SUBMITTED:
            break;
    }
}

/**
 * \internal
 * Transmit a stored message through the source registered for its operating
 * mode. The outcome is reported asynchronously by handleTx().
 */
static int transmit(const struct message *msg)
{
    const struct messageOps *ops = findSource(msg->mode);

    if (ctx->txSlot.desc.status != PKT_STATUS_IDLE)
        return -EBUSY;

    ctx->txSlot.desc.buffer = ctx->txSlot.buffer;
    ctx->txSlot.desc.size = sizeof(ctx->txSlot.buffer);

    int ret = ops->formatTx(msg, &ctx->txSlot.desc);
    if (ret != 0)
        return ret;

    ctx->txSlot.mode = msg->mode;
    ctx->txSequence = msg->sequence;

    return rtx_addPacketTx(&ctx->txSlot.desc);
}

void messages_init(void)
{
    memset(sources, 0, sizeof(sources));
    numSources = 0;
    nextSequence = 1;
    free(ctx);
    ctx = NULL;
}

void messages_terminate(void)
{
    if ((ctx != NULL) && slotsBusy())
        return;

    free(ctx);
    ctx = NULL;
}

int messages_registerSource(const struct messageOps *ops)
{
    if ((ops == NULL) || (ops->processRx == NULL) || (ops->formatTx == NULL))
        return -EINVAL;

    if (findSource(ops->mode) != NULL)
        return -EEXIST;

    if (numSources == ARRAY_SIZE(sources))
        return -ENOSPC;

    sources[numSources++] = ops;
    return 0;
}

size_t messages_task(uint8_t mode)
{
    const struct messageOps *ops = findSource(mode);

    if (ctx == NULL) {
        if (ops == NULL)
            return 0;

        allocate();
        if (ctx == NULL)
            return 0;
    }

    for (size_t i = 0; i < ARRAY_SIZE(ctx->rxSlots); i++)
        handleRx(&ctx->rxSlots[i], ops, mode);

    handleTx();

    /*
     * No source for the current mode: release the storage as soon as the
     * rtx stage has handed back every descriptor, dropping the content.
     */
    if (ops == NULL) {
        messages_terminate();
        return 0;
    }

    size_t arrivals = ctx->newArrivals;
    ctx->newArrivals = 0;
    return arrivals;
}

size_t messages_count(void)
{
    if (ctx == NULL)
        return 0;

    return ctx->numEntries;
}

size_t messages_countUnread(void)
{
    size_t unread = 0;

    for (size_t i = 0; i < messages_count(); i++) {
        if (entryAt(i)->unread != 0)
            unread++;
    }

    return unread;
}

const struct message *messages_get(size_t idx)
{
    if (idx >= messages_count())
        return NULL;

    return entryAt(ctx->numEntries - idx - 1);
}

size_t messages_findBySequence(uint32_t sequence)
{
    for (size_t i = 0; i < messages_count(); i++) {
        if (entryAt(i)->sequence == sequence)
            return ctx->numEntries - i - 1;
    }

    return SIZE_MAX;
}

int32_t messages_store(const struct message *msg)
{
    if ((msg == NULL) || ((msg->body == NULL) && (msg->bodyLen != 0)))
        return -EINVAL;

    if (msg->bodyLen >= MSG_BODY_MAX_LEN)
        return -EMSGSIZE;

    if (ctx == NULL)
        return -ENODEV;

    if (ctx->numEntries == ARRAY_SIZE(ctx->entries))
        removeEntry(0);

    size_t offset = poolAlloc(msg->bodyLen);
    if (msg->bodyLen > 0)
        memcpy(&ctx->pool[offset], msg->body, msg->bodyLen);
    ctx->pool[offset + msg->bodyLen] = '\0';

    struct message *entry = entryAt(ctx->numEntries);
    ctx->numEntries++;
    *entry = *msg;
    entry->body = &ctx->pool[offset];
    entry->sequence = nextSequence++;
    entry->sender[MSG_ADDR_MAX_LEN - 1] = '\0';
    entry->recipient[MSG_ADDR_MAX_LEN - 1] = '\0';

    if ((entry->direction == MSG_DIR_RX) && (entry->unread != 0))
        ctx->newArrivals++;

    return (int32_t)entry->sequence;
}

int messages_setStatus(uint32_t sequence, enum messageStatus status)
{
    size_t idx = messages_findBySequence(sequence);

    if (idx == SIZE_MAX)
        return -ENOENT;

    entryAt(ctx->numEntries - idx - 1)->status = status;
    return 0;
}

int messages_markRead(size_t idx, bool read)
{
    if (idx >= messages_count())
        return -ENOENT;

    entryAt(ctx->numEntries - idx - 1)->unread = read ? 0 : 1;
    return 0;
}

int messages_delete(size_t idx)
{
    if (idx >= messages_count())
        return -ENOENT;

    removeEntry(ctx->numEntries - idx - 1);
    return 0;
}

bool messages_canCompose(uint8_t mode)
{
    return (findSource(mode) != NULL) && (ctx != NULL);
}

int messages_send(const struct message *msg)
{
    if (msg == NULL)
        return -EINVAL;

    if (findSource(msg->mode) == NULL)
        return -ENOENT;

    if (ctx == NULL)
        return -ENODEV;

    struct message entry = *msg;
    entry.direction = MSG_DIR_TX;
    entry.status = MSG_STATUS_SENDING;
    entry.unread = 0;

    int32_t seq = messages_store(&entry);
    if (seq < 0)
        return seq;

    /* The entry just stored is the newest one. */
    struct message *stored = entryAt(ctx->numEntries - 1);
    int ret = transmit(stored);
    if (ret != 0)
        stored->status = MSG_STATUS_FAILED;

    return ret;
}
