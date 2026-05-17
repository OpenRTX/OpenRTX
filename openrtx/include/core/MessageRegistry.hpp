/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MESSAGE_REGISTRY_HPP
#define MESSAGE_REGISTRY_HPP

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "core/messages.h"
#include "hwconfig.h"
#include <cstddef>

/**
 * @brief A single message source entry: vtable + opaque context.
 *
 * Build an array of these at compile time and pass it to
 * `MessageRegistry::init()`.
 */
struct SourceEntry {
    const message_type_vtable_t *vtable;
    void *ctx;
};

/**
 * @brief Protocol-agnostic message registry.
 *
 * The registry is a multiplexer — it does **not** own message storage.
 * Each registered source provides a `count()` / `get(i)` callback pair;
 * the registry holds borrowed references.
 *
 * A sorted snapshot (newest-first by `timestamp`) is rebuilt on every call
 * to `tick()`. Pointers in the snapshot are valid until the next `tick()`.
 *
 * All methods must be called from the UI/state thread.  No internal locking
 * is performed.
 */
class MessageRegistry
{
public:
    /** Maximum snapshot entries across all sources.
     *  Configured per target in hwconfig.h via CONFIG_MSG_SNAPSHOT_SIZE. */
    static constexpr size_t MAX_MESSAGES_SNAPSHOT = CONFIG_MSG_SNAPSHOT_SIZE;

    /**
     * @brief Compare two datetime_t values chronologically.
     *
     * @return negative if a < b, zero if equal, positive if a > b.
     */
    static int datetimeCmp(datetime_t a, datetime_t b);

    /**
     * @brief Initialise the registry with a fixed, compile-time source array.
     *
     * @param sources: pointer to an array of SourceEntry structs; must remain
     *                 valid for the lifetime of the registry.
     * @param count:   number of entries in the array.
     */
    void init(const SourceEntry *sources, size_t count);

    /**
     * @brief Clear the snapshot.  Source array pointer is released.
     */
    void terminate();

    /**
     * @brief Rebuild the sorted snapshot from all registered sources.
     *
     * Uses a stable insertion sort (O(n^2), n <= MAX_MESSAGES_SNAPSHOT) to
     * order entries by `timestamp` descending (newest first).
     */
    void tick();

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
     * Valid until the next `tick()`.
     *
     * @param idx: zero-based index.
     * @return borrowed pointer, or nullptr if out of range.
     */
    message_header_t *get(size_t idx) const;

    /**
     * @brief Invoke an action on the entry at snapshot index @p idx.
     *
     * @param idx: zero-based snapshot index.
     * @param action: action to invoke.
     * @return 0 on success, -ENOENT if out of range, -ENOTSUP if the action
     *         is not supported, or another negative errno on failure.
     */
    int invokeAction(size_t idx, message_action_t action);

    /**
     * @brief Return true if a compose-capable source exists for @p mode.
     *
     * Matches sources where `vtable->start_compose != nullptr` and
     * `vtable->mode_id == mode`.  mode == 0 (OPMODE_NONE) never matches.
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
     * @brief Return the vtable of the source that produced snapshot entry @p idx.
     *
     * @param idx: zero-based snapshot index.
     * @return borrowed pointer to the source vtable, or nullptr if out of range.
     */
    const message_type_vtable_t *getVtable(size_t idx) const;

private:
    const SourceEntry *sources;
    size_t sourcesLen;
    message_header_t *snapshot[MAX_MESSAGES_SNAPSHOT];
    uint8_t snapshotSource[MAX_MESSAGES_SNAPSHOT];
    size_t snapshotLen;
};

#endif /* MESSAGE_REGISTRY_HPP */
