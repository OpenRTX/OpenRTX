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
 * APRS) into a unified inbox sorted newest-first by timestamp.
 *
 * All C facade calls must be made from the UI/state thread.  Protocol
 * producers must not call the registry directly from the RTX thread;
 * stage data via rtxStatus_t instead.
 *
 * Pointers returned by messages_get() are valid only until the next call
 * to messages_tick().  UI code must not cache them across ticks.
 *
 * Per-protocol entry structs MUST have message_header_t as their first
 * field.  The registry accesses all entries through that header pointer.
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "core/datetime.h"
#include "core/graphics.h"
#include "core/input.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Direction of a message (received or transmitted).
 */
typedef enum {
    MSG_DIR_RX = 0, /**< Incoming message. */
    MSG_DIR_TX = 1, /**< Outgoing message. */
} message_direction_t;

/**
 * @brief Lifecycle status of a message.
 */
typedef enum {
    MSG_STATUS_RECEIVED = 0, /**< Successfully received. */
    MSG_STATUS_SENDING,      /**< TX in progress. */
    MSG_STATUS_SENT,         /**< TX acknowledged / completed. */
    MSG_STATUS_FAILED,       /**< TX failed. */
    MSG_STATUS_READ,         /**< RX, already read by the user. */
} message_status_t;

/**
 * @brief Bitmap of actions that may be invoked on a message entry.
 *
 * A source reports which actions it supports via
 * `vtable->supported_actions()`. The UI must check this bitmap before
 * offering an action affordance.
 */
typedef enum {
    MSG_ACTION_VIEW = 1u << 0,      /**< Open detail view. */
    MSG_ACTION_DELETE = 1u << 1,    /**< Remove the entry. */
    MSG_ACTION_REPLY = 1u << 2,     /**< Launch compose pre-filled w/ sender.*/
    MSG_ACTION_MARK_READ = 1u << 3, /**< Clear the unread flag. */
    MSG_ACTION_MARK_UNREAD = 1u << 4, /**< Set the unread flag. */
} message_action_t;

/**
 * @brief Common header embedded at the start of every per-protocol entry.
 *
 * Protocol-specific entry structs MUST begin with this field.  The registry
 * accesses all entries through a pointer to this header.
 */
typedef struct {
    uint16_t type;                 /**< Protocol-specific message type; set by
                                        the source. */
    datetime_t timestamp;          /**< Wall-clock time when the message was
                                        received or sent. */
    message_direction_t direction; /**< RX or TX. */
    message_status_t status;       /**< Lifecycle status. */
    bool unread;                   /**< True when not yet read by the user. */
    char sender[16];               /**< Originating callsign / address. */
    char recipient[16];            /**< Destination callsign / address. */
    const void *body;              /**< Source-owned body pointer; valid until
                                       entry evicted. */
    size_t body_len;               /**< Byte length of body content. */
} message_header_t;

/**
 * @brief Vtable linking a message source to the registry.
 *
 * All function pointers except `count` and `get` are optional; set to NULL
 * if the source does not support that capability.
 */
typedef struct {
    /**
     * Display name shown in the "New" picker.  Short, e.g. "M17 SMS".
     * Required when `start_compose` is non-NULL.
     */
    const char *name;

    /**
     * @brief Return the number of entries currently held by this source.
     *
     * @param src_ctx: opaque context pointer supplied at registration.
     * @return current entry count.
     */
    size_t (*count)(void *src_ctx);

    /**
     * @brief Return a pointer to entry @p i.
     *
     * The pointer is borrowed; the registry does not copy it.  The caller
     * (registry) guarantees it will not be accessed after the source has
     * evicted the entry.
     *
     * @param src_ctx: opaque context pointer supplied at registration.
     * @param i: zero-based entry index, 0 <= i < count().
     * @return pointer to the entry's embedded `message_header_t`.
     */
    message_header_t *(*get)(void *src_ctx, size_t i);

    /**
     * @brief Return a bitmap of actions supported for this entry.
     *
     * @param entry: borrowed pointer to the entry header.
     * @return bitwise-OR of `message_action_t` values.
     */
    uint32_t (*supported_actions)(const message_header_t *entry);

    /**
     * @brief Invoke an action on this entry.
     *
     * @param entry: borrowed pointer to the entry header.
     * @param action: one of the `message_action_t` values.
     * @return 0 on success, negative errno on failure.
     */
    int (*invoke_action)(message_header_t *entry, message_action_t action);

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
     * Sources should drain any pending pkt_rx_event_t entries from
     * packet_io_dequeue_rx() here and update TX completion status.
     * Called before the snapshot is rebuilt, so new entries are visible
     * in the same tick that they arrive.
     *
     * @param src_ctx: opaque context pointer supplied at registration.
     */
    void (*tick)(void *src_ctx);

    /**
     * @brief Enqueue an outgoing message for this source.
     *
     * Creates an inbox entry with MSG_STATUS_SENDING and posts a
     * pkt_tx_request_t via packet_io_enqueue_tx().  The UI calls this
     * through messages_send() rather than directly into the source.
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
} message_type_vtable_t;

/* ---------------------------------------------------------------------- */
/* C facade — all calls must be made from the UI/state thread.             */
/* ---------------------------------------------------------------------- */

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
 */
void messages_tick(void);

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
message_header_t *messages_get(size_t idx);

/**
 * @brief Invoke an action on the entry at snapshot index @p idx.
 *
 * Dispatches through the entry's source vtable.
 *
 * @param idx: zero-based snapshot index.
 * @param action: action to invoke.
 * @return 0 on success, -ENOENT if idx is out of range, -ENOTSUP if the
 *         action is not supported, or another negative errno on failure.
 */
int messages_invoke_action(size_t idx, message_action_t action);

/**
 * @brief Return true if a compose-capable source exists for @p mode.
 *
 * A source qualifies when its vtable has a non-NULL `start_compose` pointer
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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MESSAGES_H */
