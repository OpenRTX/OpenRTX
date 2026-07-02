/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MESSAGE_REGISTRY_H
#define MESSAGE_REGISTRY_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "core/messages.h"
#include <cerrno>
#include <cstddef>

/**
 * @brief A single message source entry: ops + opaque context.
 *
 * Build an array of these at compile time and pass it to the
 * `MessageRegistry` constructor.  `ops` must be non-NULL and its `count`/
 * `get` members must be non-NULL for every registered source; this is a
 * configuration contract checked defensively (not asserted) by every
 * registry method that dereferences it.
 */
struct SourceEntry {
    const struct message_ops *ops;
    void *ctx;
};

/**
 * @brief Protocol-agnostic message registry.
 *
 * The registry is a multiplexer — it does **not** own message storage.
 * Each registered source provides a `count()` / `get(i)` callback pair;
 * the registry holds borrowed references.
 *
 * A sorted snapshot (newest-first by `sequence`) is rebuilt by `tick()`
 * whenever a source reports a change (or the snapshot was explicitly marked
 * dirty).  Pointers in the snapshot are valid until the next `tick()` call
 * that actually rebuilds (i.e. until the next call that returns true).
 *
 * All methods must be called from the UI/state thread.  No internal locking
 * is performed.
 */
class MessageRegistry
{
public:
    /** Maximum snapshot entries across all sources.
     *  Configured per target in hwconfig.h via CONFIG_MSG_SNAPSHOT_SIZE.
     *  Falls back to 1 so the class remains usable (e.g. in unit tests)
     *  when built without a target hwconfig.h setting it. */
    static constexpr size_t MAX_MESSAGES_SNAPSHOT =
#ifdef CONFIG_MSG_SNAPSHOT_SIZE
        CONFIG_MSG_SNAPSHOT_SIZE;
#else
        1;
#endif

    /**
     * @brief Construct the registry over a fixed, compile-time source array.
     *
     * @param sources: pointer to an array of SourceEntry structs; must remain
     *                 valid for the lifetime of the registry.
     * @param count:   number of entries in the array.
     */
    MessageRegistry(const SourceEntry *sources, size_t count);

    /**
     * @brief Destroy the registry.  The borrowed source array is released; the
     * registry owns no message storage, so nothing else needs cleanup.
     */
    ~MessageRegistry() = default;

    /* Non-copyable: holds borrowed pointers and a large snapshot buffer. */
    MessageRegistry(const MessageRegistry &) = delete;
    MessageRegistry &operator=(const MessageRegistry &) = delete;

    /**
     * @brief Rebuild the sorted snapshot from all registered sources.
     *
     * Invokes each source's tick() callback first (if non-NULL); the
     * bitwise-OR of their return values is combined with the internal dirty
     * flag to decide whether a rebuild is needed this call.  Uses a stable
     * insertion sort (O(n^2), n <= MAX_MESSAGES_SNAPSHOT) to order entries by
     * `sequence` descending (newest first).  A double-buffered approach is
     * used: the new snapshot is built into a separate buffer and only
     * swapped in after sorting completes, so pointers obtained before this
     * call remain valid for the duration of the rebuild.
     *
     * @return true if the snapshot was rebuilt, false if skipped (not dirty
     *         and no source reported a change).
     */
    bool tick();

    /**
     * @brief Return the number of entries in the current snapshot.
     *
     * @return entry count.
     */
    size_t count() const;

    /**
     * @brief Return the number of unread entries in the current snapshot.
     *
     * @return count of entries with `unread == true`.
     */
    size_t countUnread() const;

    /**
     * @brief Return a pointer to the @p idx-th snapshot entry.
     *
     * Valid until the next `tick()` that rebuilds.  The double-buffered
     * approach ensures pointers remain valid during the rebuild phase.
     *
     * @param idx: zero-based index.
     * @return borrowed pointer, or nullptr if out of range.
     */
    struct message_header *get(size_t idx) const;

    /**
     * @brief Find the snapshot index of an entry by its sequence number.
     *
     * Intended for callers that must survive a snapshot rebuild (e.g. the
     * message detail view): pin a sequence number instead of a raw index.
     *
     * @param seq: sequence number to look up.
     * @return snapshot index, or SIZE_MAX if not found.
     */
    size_t findBySequence(uint32_t seq) const;

    /**
     * @brief Return the supported-actions bitmap for snapshot entry @p idx.
     *
     * Dispatches through the source ops's supported_actions() callback.
     *
     * @param idx: zero-based snapshot index.
     * @return action bitmap, or 0 if idx is out of range or the source does
     *         not implement supported_actions().
     */
    uint32_t supportedActions(size_t idx) const;

    /**
     * @brief Invoke an action on the entry at snapshot index @p idx.
     *
     * Marks the snapshot dirty on success so the next tick() rebuilds.
     *
     * @param idx: zero-based snapshot index.
     * @param action: action to invoke.
     * @return 0 on success, -ENOENT if out of range, -ENOTSUP if the action
     *         is not supported, or another negative errno on failure.
     */
    int invokeAction(size_t idx, enum message_action action);

    /**
     * @brief Return true if a compose-capable source exists for @p mode.
     *
     * Matches sources where `ops->start_compose != nullptr` and
     * `ops->mode_id == mode`.  mode == 0 (OPMODE_NONE) never matches.
     *
     * @param mode: operating mode (one of enum opmode, stored as uint8_t).
     * @return true if at least one matching source is registered.
     */
    bool canCompose(uint8_t mode) const;

    /**
     * @brief Invoke `start_compose` on the source registered for @p mode.
     *
     * @param mode: operating mode (one of enum opmode, stored as uint8_t).
     * @return 0 on success, -ENOENT if no matching source found.
     */
    int startCompose(uint8_t mode);

    /**
     * @brief Return the mode_id of the source that produced snapshot entry @p idx.
     *
     * @param idx: zero-based snapshot index.
     * @return mode_id of the owning source, or 0 if idx is out of range.
     */
    uint8_t sourceMode(size_t idx) const;

    /**
     * @brief Enqueue an outgoing message via the source registered for @p mode.
     *
     * Marks the snapshot dirty on success so the next tick() rebuilds.
     *
     * @param mode:      operating mode identifying the target source.
     * @param body:      NUL-terminated message text.
     * @param body_len:  Length of body not counting NUL.
     * @param recipient: Destination callsign.
     * @return 0 on success, -ENOENT if no matching source, or the source
     *         send() return value.
     */
    int send(uint8_t mode, const char *body, size_t body_len,
             const char *recipient);

    /**
     * @brief Mark the snapshot as dirty, forcing a rebuild on next tick().
     *
     * Call this from a source when it adds or removes entries so the UI
     * can skip an unnecessary rebuild on ticks where nothing changed.
     */
    void markDirty();

private:
    /** One resolved snapshot slot: the borrowed header pointer plus which
     *  source it came from (needed to route invokeAction()/sourceMode()
     *  back to the owning source). */
    struct SnapshotEntry {
        struct message_header *hdr;
        uint8_t source_idx;
    };

    const SourceEntry *sources = nullptr;
    size_t sourcesLen = 0;

    /* Double-buffered snapshot: two buffers are alternated on each rebuild.
     * `snapshotActive` selects which buffer is currently visible to callers. */
    SnapshotEntry snapshots[2][MAX_MESSAGES_SNAPSHOT] = {};
    size_t snapshotLens[2] = { 0, 0 };
    uint8_t snapshotActive = 0;

    bool dirty = false;
};

/* ---------------------------------------------------------------------- */
/* Inline implementation.
 *
 * MessageRegistry is header-only (no MessageRegistry.cpp): every method is
 * a plain, non-template member function, so each is marked `inline` to
 * satisfy the one-definition rule across the multiple translation units
 * that include this header (messages.cpp, UI sources, unit tests).
 * ---------------------------------------------------------------------- */

inline MessageRegistry::MessageRegistry(const SourceEntry *srcs, size_t count) :
    sources(srcs), sourcesLen(count), dirty(true)
{
}

inline void MessageRegistry::markDirty()
{
    dirty = true;
}

inline bool MessageRegistry::tick()
{
    /* Give each source a chance to drain the packet_io RX queue and
     * advance TX completion status before we rebuild the snapshot. */
    bool any_changed = false;
    for (size_t s = 0; s < sourcesLen; s++) {
        if (sources[s].ops == nullptr || sources[s].ops->tick == nullptr)
            continue;
        if (sources[s].ops->tick(sources[s].ctx))
            any_changed = true;
    }

    /* Skip rebuild if nothing has changed since the last tick. */
    if (!dirty && !any_changed)
        return false;

    dirty = false;

    /* Build into the inactive buffer to avoid stale pointers during rebuild. */
    uint8_t next = snapshotActive ^ 1;
    snapshotLens[next] = 0;

    /* Collect all entries from all sources into the next buffer. */
    for (size_t s = 0;
         s < sourcesLen && snapshotLens[next] < MAX_MESSAGES_SNAPSHOT; s++) {
        if (sources[s].ops == nullptr || sources[s].ops->count == nullptr)
            continue;
        size_t n = sources[s].ops->count(sources[s].ctx);
        for (size_t i = 0; i < n && snapshotLens[next] < MAX_MESSAGES_SNAPSHOT;
             i++) {
            if (sources[s].ops->get == nullptr)
                break;
            struct message_header *hdr = sources[s].ops->get(sources[s].ctx, i);
            if (hdr == nullptr)
                continue;

            snapshots[next][snapshotLens[next]].hdr = hdr;
            snapshots[next][snapshotLens[next]].source_idx =
                static_cast<uint8_t>(s);
            snapshotLens[next]++;
        }
    }

    /*
     * Stable insertion sort by sequence descending (newest first).
     * O(n^2) is acceptable because n <= MAX_MESSAGES_SNAPSHOT.
     */
    for (size_t i = 1; i < snapshotLens[next]; i++) {
        SnapshotEntry key = snapshots[next][i];
        size_t j = i;
        while (j > 0
               && snapshots[next][j - 1].hdr->sequence < key.hdr->sequence) {
            snapshots[next][j] = snapshots[next][j - 1];
            j--;
        }
        snapshots[next][j] = key;
    }

    /* Swap: make the newly built buffer active. */
    snapshotActive = next;
    return true;
}

inline size_t MessageRegistry::count() const
{
    return snapshotLens[snapshotActive];
}

inline size_t MessageRegistry::countUnread() const
{
    size_t n = 0;
    for (size_t i = 0; i < snapshotLens[snapshotActive]; i++) {
        if (snapshots[snapshotActive][i].hdr->unread)
            n++;
    }
    return n;
}

inline struct message_header *MessageRegistry::get(size_t idx) const
{
    if (idx >= snapshotLens[snapshotActive])
        return nullptr;
    return snapshots[snapshotActive][idx].hdr;
}

inline size_t MessageRegistry::findBySequence(uint32_t seq) const
{
    for (size_t i = 0; i < snapshotLens[snapshotActive]; i++) {
        if (snapshots[snapshotActive][i].hdr->sequence == seq)
            return i;
    }
    return SIZE_MAX;
}

inline uint32_t MessageRegistry::supportedActions(size_t idx) const
{
    if (idx >= snapshotLens[snapshotActive])
        return 0;

    size_t s = snapshots[snapshotActive][idx].source_idx;
    if (s >= sourcesLen || sources[s].ops == nullptr)
        return 0;
    if (sources[s].ops->supported_actions == nullptr)
        return 0;

    return sources[s].ops->supported_actions(
        snapshots[snapshotActive][idx].hdr);
}

inline int MessageRegistry::invokeAction(size_t idx, enum message_action action)
{
    if (idx >= snapshotLens[snapshotActive])
        return -ENOENT;

    size_t s = snapshots[snapshotActive][idx].source_idx;
    if (s >= sourcesLen || sources[s].ops == nullptr)
        return -ENOENT;

    struct message_header *hdr = snapshots[snapshotActive][idx].hdr;

    /* Check that the action is supported. */
    if (sources[s].ops->supported_actions != nullptr) {
        uint32_t supported = sources[s].ops->supported_actions(hdr);
        if (!(supported & static_cast<uint32_t>(action)))
            return -ENOTSUP;
    }

    if (sources[s].ops->invoke_action == nullptr)
        return -ENOTSUP;

    int rc = sources[s].ops->invoke_action(hdr, action);
    if (rc == 0)
        markDirty();
    return rc;
}

inline bool MessageRegistry::canCompose(uint8_t mode) const
{
    if (mode == 0)
        return false;
    for (size_t i = 0; i < sourcesLen; i++) {
        if (sources[i].ops == nullptr)
            continue;
        if (sources[i].ops->start_compose != nullptr
            && sources[i].ops->mode_id == mode)
            return true;
    }
    return false;
}

inline int MessageRegistry::startCompose(uint8_t mode)
{
    if (mode == 0)
        return -ENOENT;
    for (size_t i = 0; i < sourcesLen; i++) {
        if (sources[i].ops == nullptr)
            continue;
        if (sources[i].ops->start_compose != nullptr
            && sources[i].ops->mode_id == mode) {
            sources[i].ops->start_compose(sources[i].ctx);
            return 0;
        }
    }
    return -ENOENT;
}

inline uint8_t MessageRegistry::sourceMode(size_t idx) const
{
    if (idx >= snapshotLens[snapshotActive])
        return 0;
    size_t s = snapshots[snapshotActive][idx].source_idx;
    if (s >= sourcesLen || sources[s].ops == nullptr)
        return 0;
    return sources[s].ops->mode_id;
}

inline int MessageRegistry::send(uint8_t mode, const char *body,
                                 size_t body_len, const char *recipient)
{
    if (mode == 0)
        return -ENOENT;
    for (size_t i = 0; i < sourcesLen; i++) {
        if (sources[i].ops == nullptr)
            continue;
        if (sources[i].ops->send != nullptr
            && sources[i].ops->mode_id == mode) {
            int rc = sources[i].ops->send(sources[i].ctx, body, body_len,
                                          recipient);
            if (rc == 0)
                markDirty();
            return rc;
        }
    }
    return -ENOENT;
}

#endif /* MESSAGE_REGISTRY_H */
