/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <string.h>
#include "core/messages.h"
#include "core/utils.h"

_Static_assert(CONFIG_MESSAGES_MAX_ENTRIES > 0,
               "CONFIG_MESSAGES_MAX_ENTRIES must be positive");
_Static_assert(CONFIG_MESSAGES_POOL_BYTES >= MSG_BODY_MAX_LEN,
               "CONFIG_MESSAGES_POOL_BYTES must hold at least one full body");
_Static_assert(CONFIG_MESSAGES_POOL_BYTES <= UINT16_MAX,
               "CONFIG_MESSAGES_POOL_BYTES must fit in a uint16_t offset");

/*
 * Entries are kept oldest-first, so entries[0] is the eviction candidate and
 * entries[numEntries - 1] is the newest. Bodies live in the pool as
 * NUL-terminated strings allocated in arrival order: poolHead is the next
 * byte to write and wraps to zero when a body would not fit at the end.
 */
static struct message entries[CONFIG_MESSAGES_MAX_ENTRIES];
static size_t numEntries;
static char pool[CONFIG_MESSAGES_POOL_BYTES];
static uint16_t poolHead;
static uint32_t nextSequence;
static size_t newArrivals;

/**
 * \internal
 * Remove entry @p pos, keeping the remaining entries in order.
 */
static void removeEntry(size_t pos)
{
    size_t tail = numEntries - pos - 1;

    memmove(&entries[pos], &entries[pos + 1], tail * sizeof(entries[0]));
    numEntries--;
}

/**
 * \internal
 * Reserve room in the pool for a body of @p len characters plus NUL,
 * evicting every entry whose body overlaps the reserved range. Those entries
 * are always older than any other one still stored, since the pool is
 * written in arrival order.
 *
 * @return pool offset of the reserved range.
 */
static uint16_t poolAlloc(uint16_t len)
{
    uint16_t need = len + 1;

    if ((size_t)poolHead + need > CONFIG_MESSAGES_POOL_BYTES)
        poolHead = 0;

    uint16_t start = poolHead;
    uint16_t end = poolHead + need;

    for (size_t i = 0; i < numEntries;) {
        uint16_t bodyStart = entries[i].body - pool;
        uint16_t bodyEnd = bodyStart + entries[i].body_len + 1;

        if ((bodyStart < end) && (bodyEnd > start))
            removeEntry(i);
        else
            i++;
    }

    poolHead = end;
    return start;
}

void messages_init(void)
{
    numEntries = 0;
    poolHead = 0;
    nextSequence = 1;
    newArrivals = 0;
}

void messages_terminate(void)
{
    numEntries = 0;
}

size_t messages_tick(void)
{
    size_t arrivals = newArrivals;

    newArrivals = 0;
    return arrivals;
}

size_t messages_count(void)
{
    return numEntries;
}

size_t messages_count_unread(void)
{
    size_t unread = 0;

    for (size_t i = 0; i < numEntries; i++) {
        if (entries[i].unread != 0)
            unread++;
    }

    return unread;
}

const struct message *messages_get(size_t idx)
{
    if (idx >= numEntries)
        return NULL;

    return &entries[numEntries - idx - 1];
}

size_t messages_find_by_sequence(uint32_t sequence)
{
    for (size_t i = 0; i < numEntries; i++) {
        if (entries[i].sequence == sequence)
            return numEntries - i - 1;
    }

    return SIZE_MAX;
}

int messages_store(const struct message *msg, uint32_t *sequence)
{
    if ((msg == NULL) || ((msg->body == NULL) && (msg->body_len != 0)))
        return -EINVAL;

    if (msg->body_len >= MSG_BODY_MAX_LEN)
        return -EMSGSIZE;

    if (numEntries == ARRAY_SIZE(entries))
        removeEntry(0);

    uint16_t offset = poolAlloc(msg->body_len);
    if (msg->body_len > 0)
        memcpy(&pool[offset], msg->body, msg->body_len);
    pool[offset + msg->body_len] = '\0';

    struct message *entry = &entries[numEntries++];
    *entry = *msg;
    entry->body = &pool[offset];
    entry->sequence = nextSequence++;
    entry->sender[MSG_ADDR_MAX_LEN - 1] = '\0';
    entry->recipient[MSG_ADDR_MAX_LEN - 1] = '\0';

    if ((entry->direction == MSG_DIR_RX) && (entry->unread != 0))
        newArrivals++;

    if (sequence != NULL)
        *sequence = entry->sequence;

    return 0;
}

int messages_set_status(uint32_t sequence, enum message_status status)
{
    size_t idx = messages_find_by_sequence(sequence);

    if (idx == SIZE_MAX)
        return -ENOENT;

    entries[numEntries - idx - 1].status = status;
    return 0;
}

int messages_mark_read(size_t idx, bool read)
{
    if (idx >= numEntries)
        return -ENOENT;

    entries[numEntries - idx - 1].unread = read ? 0 : 1;
    return 0;
}

int messages_delete(size_t idx)
{
    if (idx >= numEntries)
        return -ENOENT;

    removeEntry(numEntries - idx - 1);
    return 0;
}
