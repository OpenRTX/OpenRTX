/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * @file messages.h
 * @brief Generic protocol-agnostic message inbox API.
 *
 * Aggregates messages from one or more protocol sources (e.g. M17 SMS,
 * APRS) into a unified inbox sorted newest-first by sequence.
 *
 * All C facade calls must be made from the UI/state thread.  Protocol
 * producers must not call the registry directly from the RTX thread;
 * stage data via packet_io instead.
 *
 * Pointers returned by messages_get() are valid only until the next call
 * to messages_tick().  UI code must not cache them across ticks.
 *
 * Per-protocol entry structs MUST have struct message_header as their first
 * field.  The registry accesses all entries through that header pointer.
 *
 * Eviction invariant: sources may evict or mutate entries only from within a
 * UI-thread ops callback that marks the snapshot dirty — tick(),
 * invoke_action(), or send() (all three run on the UI thread and force a
 * rebuild on the next tick()).  The registry's double-buffered snapshot keeps
 * previously returned pointers non-dangling until that rebuild, but a source
 * that reuses a slot in one of these callbacks may change the *contents* the
 * pointer refers to; UI code must therefore not cache messages_get() pointers
 * across any of them.
 *
 * CONFIG_MESSAGES: targets define this flag to enable the messaging
 * feature.  This header and messages.cpp compile unconditionally; call
 * sites guard feature wiring with #ifdef CONFIG_MESSAGES and the linker
 * garbage-collects the unreferenced functions on targets without it.
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "hwconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Direction of a message (received or transmitted).
 */
enum message_direction {
    MSG_DIR_RX = 0, /**< Incoming message. */
    MSG_DIR_TX = 1, /**< Outgoing message. */
};

/**
 * @brief Lifecycle status of a message.
 */
enum message_status {
    MSG_STATUS_RECEIVED = 0, /**< Successfully received. */
    MSG_STATUS_SENDING,      /**< TX in progress. */
    MSG_STATUS_SENT,         /**< TX acknowledged / completed. */
    MSG_STATUS_FAILED,       /**< TX failed. */
};

/**
 * @brief Bitmap of actions that may be invoked on a message entry.
 *
 * A source reports which actions it supports via
 * `ops->supported_actions()`. The UI must check this bitmap before
 * offering an action affordance.
 */
enum message_action {
    MSG_ACTION_VIEW = 1u << 0,      /**< Open detail view. */
    MSG_ACTION_DELETE = 1u << 1,    /**< Remove the entry. */
    MSG_ACTION_REPLY = 1u << 2,     /**< Launch compose pre-filled w/ sender.*/
    MSG_ACTION_MARK_READ = 1u << 3, /**< Clear the unread flag. */
    MSG_ACTION_MARK_UNREAD = 1u << 4, /**< Set the unread flag. */
};

/**
 * @brief Maximum callsign / address length including NUL terminator.
 *
 * Kept in lockstep with packet_io.h's PKT_ADDR_MAX_LEN via a static_assert in
 * each source (e.g. m17_sms.cpp) so the registry's address fields and the
 * transport queue's cannot silently drift apart.
 */
#define MSG_ADDR_MAX_LEN 16

/**
 * @brief Common header embedded at the start of every per-protocol entry.
 *
 * Protocol-specific entry structs MUST begin with this field.  The registry
 * accesses all entries through a pointer to this header.
 */
struct message_header {
    uint16_t type;     /**< Protocol-specific message type; set by
                                        the source. */
    uint32_t sequence; /**< Monotonic counter for sort order and
                                        unique identity; allocated via
                                        messages_alloc_sequence(). */
    enum message_direction direction; /**< RX or TX. */
    enum message_status status;       /**< Lifecycle status. */
    bool unread;                   /**< True when not yet read by the user. */
    char sender[MSG_ADDR_MAX_LEN]; /**< Originating callsign / address. */
    char recipient[MSG_ADDR_MAX_LEN]; /**< Destination callsign / address. */
    const void *body; /**< Source-owned body pointer; valid until
                                       entry evicted. */
    size_t body_len;  /**< Byte length of body content. */
};

/**
 * @brief Operations linking a message source to the registry.
 *
 * `count` and `get` are required (must be non-NULL for any registered
 * source).  All other function pointers are optional; set to NULL if the
 * source does not support that capability.
 */
struct message_ops {
    /**
     * Display name shown in the "New" picker.  Short, e.g. "M17 SMS".
     * Required when `start_compose` is non-NULL.
     */
    const char *name;

    /**
     * @brief Return the number of entries currently held by this source.
     *
     * Required (must be non-NULL).
     *
     * @param src_ctx: opaque context pointer supplied at registration.
     * @return current entry count.
     */
    size_t (*count)(void *src_ctx);

    /**
     * @brief Return a pointer to entry @p i.
     *
     * Required (must be non-NULL).  The pointer is borrowed; the registry
     * does not copy it.  The caller (registry) guarantees it will not be
     * accessed after the source has evicted the entry.
     *
     * @param src_ctx: opaque context pointer supplied at registration.
     * @param i: zero-based entry index, 0 <= i < count().
     * @return pointer to the entry's embedded `struct message_header`.
     */
    struct message_header *(*get)(void *src_ctx, size_t i);

    /**
     * @brief Return a bitmap of actions supported for this entry.
     *
     * @param entry: borrowed pointer to the entry header.
     * @return bitwise-OR of `enum message_action` values.
     */
    uint32_t (*supported_actions)(const struct message_header *entry);

    /**
     * @brief Invoke an action on this entry.
     *
     * @param entry: borrowed pointer to the entry header.
     * @param action: one of the `enum message_action` values.
     * @return 0 on success, negative errno on failure.
     */
    int (*invoke_action)(struct message_header *entry,
                         enum message_action action);

    /**
     * @brief Launch the protocol's compose UI (no recipient pre-fill).
     *
     * If non-NULL, this source appears in the list view's "New" picker.
     * Reply (recipient pre-filled from an existing entry's sender) is
     * dispatched via MSG_ACTION_REPLY in invoke_action instead.
     *
     * @param src_ctx: opaque context pointer supplied at registration.
     */
    void (*start_compose)(void *src_ctx);

    /**
     * @brief Per-tick update called by messages_tick() on the UI thread.
     *
     * Sources should drain any pending struct pkt_rx_event entries from
     * packet_io_dequeue_rx() here and update TX completion status.
     * Called before the snapshot is rebuilt, so new entries are visible
     * in the same tick that they arrive.
     *
     * @param src_ctx: opaque context pointer supplied at registration.
     * @return true if the source added, removed, or mutated entries during
     *         this call (forces a snapshot rebuild this tick).  Return false
     *         when nothing changed so the registry can skip an unnecessary
     *         rebuild.
     */
    bool (*tick)(void *src_ctx);

    /**
     * @brief Enqueue an outgoing message for this source.
     *
     * Creates an inbox entry with MSG_STATUS_SENDING and posts a TX request
     * via packet_io.  The UI calls this through messages_send() rather than
     * directly into the source.
     *
     * @param src_ctx:   opaque context pointer supplied at registration.
     * @param body:      NUL-terminated message text.
     * @param body_len:  Length of body not counting NUL.
     * @param recipient: Destination callsign (up to 9 chars + NUL).
     * @return 0 on success, -EBUSY if a TX is already in flight,
     *         -EINVAL for bad arguments, -EMSGSIZE if text is too large.
     */
    int (*send)(void *src_ctx, const char *body, size_t body_len,
                const char *recipient);

    /**
     * @brief Radio operating mode this source serves.
     *
     * Holds one of the `enum opmode` values from rtx/rtx.h stored as a
     * plain uint8_t to avoid a layering dependency on that header.
     * Use OPMODE_NONE (0) for sources that have no associated mode (e.g.
     * demo stubs); such sources are never offered for compose or reply.
     */
    uint8_t mode_id;
};

/* ---------------------------------------------------------------------- */
/* C facade — all calls must be made from the UI/state thread.             */
/* ---------------------------------------------------------------------- */

/**
 * @brief Allocate a unique sequence number for a new message entry.
 *
 * Monotonically increasing; suitable as both sort key and stable identity.
 * Hydrating sources (those that load persisted entries) should allocate in
 * stored order so the sequence preserves arrival order.
 *
 * @return next available sequence number.
 */
uint32_t messages_alloc_sequence(void);

/**
 * @brief Find the snapshot index of an entry by its sequence number.
 *
 * @param seq: sequence number to look up.
 * @return snapshot index, or SIZE_MAX if not found.
 */
size_t messages_find_by_sequence(uint32_t seq);

/**
 * @brief Return the supported-actions bitmap for snapshot entry @p idx.
 *
 * Dispatches through the source ops's supported_actions() callback.
 *
 * @param idx: zero-based snapshot index.
 * @return action bitmap, or 0 if idx is out of range.
 */
uint32_t messages_supported_actions(size_t idx);

/**
 * @brief Initialise the message registry singleton.
 *
 * Called once from state_init().
 */
void messages_init(void);

/**
 * @brief Terminate the message registry and release any resources.
 *
 * Called once from state_terminate().
 */
void messages_terminate(void);

/**
 * @brief Recompute the sorted inbox snapshot.
 *
 * Must be called each UI cycle (typically from the UI thread loop).
 * After this call, pointers from the previous snapshot are invalid.
 *
 * Sources' tick() callbacks are invoked first to drain packet_io and advance
 * TX completion status.  Any source returning true from tick(), or a prior
 * successful send()/invoke_action(), forces a snapshot rebuild.
 *
 * @return count of newly arrived unread RX entries since the previous tick
 *         (a high-water mark on unread count).  Zero means nothing new.
 */
size_t messages_tick(void);

/**
 * @brief Return the number of entries in the current snapshot.
 *
 * @return number of entries, sorted newest-first.
 */
size_t messages_count(void);

/**
 * @brief Return the number of unread entries in the current snapshot.
 *
 * @return count of entries with `unread == true`.
 */
size_t messages_count_unread(void);

/**
 * @brief Return a pointer to the @p idx-th entry in the current snapshot.
 *
 * The returned pointer is valid only until the next call to messages_tick().
 * UI code must not cache it across ticks.
 *
 * @param idx: zero-based index, 0 <= idx < messages_count().
 * @return borrowed pointer to the entry header, or NULL if out of range.
 */
struct message_header *messages_get(size_t idx);

/**
 * @brief Invoke an action on the entry at snapshot index @p idx.
 *
 * Dispatches through the entry's source ops.
 *
 * @param idx: zero-based snapshot index.
 * @param action: action to invoke.
 * @return 0 on success, -ENOENT if idx is out of range, -ENOTSUP if the
 *         action is not supported, or another negative errno on failure.
 */
int messages_invoke_action(size_t idx, enum message_action action);

/**
 * @brief Return true if a compose-capable source exists for @p mode.
 *
 * A source qualifies when its ops has a non-NULL `start_compose` pointer
 * and its `mode_id` equals @p mode.  OPMODE_NONE (0) never matches.
 *
 * @param mode: one of the `enum opmode` values (stored as uint8_t).
 * @return true if compose is available for that mode.
 */
bool messages_can_compose(uint8_t mode);

/**
 * @brief Invoke `start_compose` on the source registered for @p mode.
 *
 * @param mode: one of the `enum opmode` values (stored as uint8_t).
 * @return 0 on success, -ENOENT if no matching source found.
 */
int messages_start_compose(uint8_t mode);

/**
 * @brief Return the mode_id of the source that produced snapshot entry @p idx.
 *
 * Use this to gate Reply: only offer it when the returned value matches the
 * current operating mode.
 *
 * @param idx: zero-based snapshot index.
 * @return mode_id of the owning source, or 0 if idx is out of range.
 */
uint8_t messages_source_mode(size_t idx);

/**
 * @brief Enqueue an outgoing message via the source registered for @p mode.
 *
 * Dispatches through the source ops's send() callback.  The UI must
 * call this instead of calling a protocol send function directly.
 *
 * @param mode:      opmode value identifying the target protocol source.
 * @param body:      NUL-terminated message text.
 * @param body_len:  Length of body not counting NUL.
 * @param recipient: Destination callsign.
 * @return 0 on success, -ENOENT if no matching source, or the source's
 *         send() return value on failure.
 */
int messages_send(uint8_t mode, const char *body, size_t body_len,
                  const char *recipient);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MESSAGES_H */
