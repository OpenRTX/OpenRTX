/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/MessageRegistry.hpp"
#include <cerrno>
#include <cstring>

int MessageRegistry::datetimeCmp(datetime_t a, datetime_t b)
{
    if (a.year != b.year)
        return (int)a.year - (int)b.year;
    if (a.month != b.month)
        return (int)a.month - (int)b.month;
    if (a.date != b.date)
        return (int)a.date - (int)b.date;
    if (a.hour != b.hour)
        return (int)a.hour - (int)b.hour;
    if (a.minute != b.minute)
        return (int)a.minute - (int)b.minute;
    return (int)a.second - (int)b.second;
}

void MessageRegistry::init(const SourceEntry *srcs, size_t count)
{
    sources = srcs;
    sourcesLen = count;
    snapshotLen = 0;
}

void MessageRegistry::terminate()
{
    sources = nullptr;
    sourcesLen = 0;
    snapshotLen = 0;
}

void MessageRegistry::tick()
{
    snapshotLen = 0;

    /* Collect all entries from all sources into the snapshot. */
    for (size_t s = 0; s < sourcesLen; s++) {
        size_t n = sources[s].vtable->count(sources[s].ctx);
        for (size_t i = 0; i < n; i++) {
            if (snapshotLen >= MAX_MESSAGES_SNAPSHOT)
                goto snapshot_full;

            message_header_t *hdr = sources[s].vtable->get(sources[s].ctx, i);
            if (hdr == nullptr)
                continue;

            snapshotSource[snapshotLen] = static_cast<uint8_t>(s);
            snapshot[snapshotLen] = hdr;
            snapshotLen++;
        }
    }
snapshot_full:

    /*
     * Stable insertion sort by timestamp descending (newest first).
     * O(n^2) is acceptable because n <= MAX_MESSAGES_SNAPSHOT.
     * snapshotSource[] is kept in sync with snapshot[] during each swap.
     */
    for (size_t i = 1; i < snapshotLen; i++) {
        message_header_t *key = snapshot[i];
        uint8_t keySrc = snapshotSource[i];
        size_t j = i;
        while (j > 0
               && datetimeCmp(snapshot[j - 1]->timestamp, key->timestamp) < 0) {
            snapshot[j] = snapshot[j - 1];
            snapshotSource[j] = snapshotSource[j - 1];
            j--;
        }
        snapshot[j] = key;
        snapshotSource[j] = keySrc;
    }
}

size_t MessageRegistry::count() const
{
    return snapshotLen;
}

size_t MessageRegistry::countUnread() const
{
    size_t n = 0;
    for (size_t i = 0; i < snapshotLen; i++) {
        if (snapshot[i]->unread)
            n++;
    }
    return n;
}

message_header_t *MessageRegistry::get(size_t idx) const
{
    if (idx >= snapshotLen)
        return nullptr;
    return snapshot[idx];
}

int MessageRegistry::invokeAction(size_t idx, message_action_t action)
{
    if (idx >= snapshotLen)
        return -ENOENT;

    message_header_t *hdr = snapshot[idx];

    /* Route back to source via parallel index array. */
    size_t s = snapshotSource[idx];

    /* Check that the action is supported. */
    if (sources[s].vtable->supported_actions != nullptr) {
        uint32_t supported = sources[s].vtable->supported_actions(hdr);
        if (!(supported & static_cast<uint32_t>(action)))
            return -ENOTSUP;
    }

    if (sources[s].vtable->invoke_action == nullptr)
        return -ENOTSUP;

    return sources[s].vtable->invoke_action(hdr, action);
}

bool MessageRegistry::canCompose(uint8_t mode) const
{
    if (mode == 0)
        return false;
    for (size_t i = 0; i < sourcesLen; i++) {
        if (sources[i].vtable->start_compose != nullptr
            && sources[i].vtable->mode_id == mode)
            return true;
    }
    return false;
}

int MessageRegistry::startCompose(uint8_t mode)
{
    if (mode == 0)
        return -ENOENT;
    for (size_t i = 0; i < sourcesLen; i++) {
        if (sources[i].vtable->start_compose != nullptr
            && sources[i].vtable->mode_id == mode) {
            sources[i].vtable->start_compose(sources[i].ctx);
            return 0;
        }
    }
    return -ENOENT;
}

uint8_t MessageRegistry::sourceMode(size_t idx) const
{
    if (idx >= snapshotLen)
        return 0;
    return sources[snapshotSource[idx]].vtable->mode_id;
}
