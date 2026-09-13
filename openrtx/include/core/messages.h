/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "hwconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Protocol-agnostic message inbox.
 *
 * The registry owns all message storage: a fixed array of message entries
 * plus a shared, variable-length body pool. Protocol sources (M17 SMS, APRS,
 * ...) never keep messages of their own; they translate between packets and
 * entries, and file inbound messages here through messages_store().
 *
 * Entries are kept in arrival order and exposed newest-first. When the entry
 * array or the body pool is full, the oldest entries are evicted to make room
 * for a new one.
 *
 * Threading: every function in this header must be called from the UI thread.
 * Pointers returned by messages_get() are valid only until the next call that
 * stores, deletes or ticks; UI code must not cache them across ticks and must
 * pin an entry by its sequence number instead (messages_find_by_sequence()).
 */

/* Maximum number of entries held in the inbox. */
#ifndef CONFIG_MESSAGES_MAX_ENTRIES
#define CONFIG_MESSAGES_MAX_ENTRIES 32
#endif

/* Size of the shared body pool, in bytes. */
#ifndef CONFIG_MESSAGES_POOL_BYTES
#define CONFIG_MESSAGES_POOL_BYTES 3200
#endif

/*
 * Maximum message body length, including the NUL terminator. Sized for the
 * largest body any protocol can carry in a single packet: an M17 packet-mode
 * SMS holds up to 821 characters. Widen it if a future protocol needs more.
 */
#define MSG_BODY_MAX_LEN 822

/* Maximum address (callsign) length, including the NUL terminator. */
#define MSG_ADDR_MAX_LEN 10

/**
 * Direction of a message.
 */
enum message_direction {
    MSG_DIR_RX = 0, /**< Received message.    */
    MSG_DIR_TX = 1, /**< Transmitted message. */
};

/**
 * Lifecycle status of a message.
 */
enum message_status {
    MSG_STATUS_RECEIVED = 0, /**< Successfully received.  */
    MSG_STATUS_SENDING,      /**< Transmission in progress. */
    MSG_STATUS_SENT,         /**< Transmission completed.   */
    MSG_STATUS_FAILED,       /**< Transmission failed.      */
};

/**
 * A message entry.
 *
 * Also used as the template passed to messages_store(): the caller fills in
 * everything except sequence, and body may point anywhere (typically into a
 * packet buffer) since the registry copies it into its own pool.
 */
struct message {
    const char *body;  /**< NUL-terminated body, registry-owned.      */
    uint32_t sequence; /**< Unique, monotonic; zero means unset.      */
    uint16_t body_len; /**< Body length, not counting the NUL.        */
    uint16_t type;     /**< Protocol-specific message type.           */
    uint8_t mode;      /**< Originating opmode, enum opmode.          */
    uint8_t direction; /**< enum message_direction.                   */
    uint8_t status;    /**< enum message_status.                      */
    uint8_t unread;    /**< Nonzero until the user has seen it.       */
    char sender[MSG_ADDR_MAX_LEN];    /**< Originating address.        */
    char recipient[MSG_ADDR_MAX_LEN]; /**< Destination address.        */
};

/**
 * Initialise the message registry, discarding any stored entries.
 */
void messages_init(void);

/**
 * Shut down the message registry.
 */
void messages_terminate(void);

/**
 * Periodic update, to be called once per UI loop iteration.
 *
 * @return number of unread messages received since the previous call.
 */
size_t messages_tick(void);

/**
 * Get the number of stored entries.
 *
 * @return entry count.
 */
size_t messages_count(void);

/**
 * Get the number of unread entries.
 *
 * @return unread entry count.
 */
size_t messages_count_unread(void);

/**
 * Get an entry by position, newest first.
 *
 * @param idx: zero-based index, 0 <= idx < messages_count().
 * @return pointer to the entry, or NULL if idx is out of range.
 */
const struct message *messages_get(size_t idx);

/**
 * Find the current position of an entry from its sequence number.
 *
 * @param sequence: sequence number to look up.
 * @return index suitable for messages_get(), or SIZE_MAX if not found.
 */
size_t messages_find_by_sequence(uint32_t sequence);

/**
 * Store a new entry, copying its body into the registry pool. The oldest
 * entries are evicted if there is no room for the new one.
 *
 * @param msg: entry template, see struct message.
 * @param sequence: if not NULL, receives the sequence number assigned.
 * @return zero on success, -EINVAL on bad arguments, -EMSGSIZE if the body
 *         is longer than MSG_BODY_MAX_LEN - 1.
 */
int messages_store(const struct message *msg, uint32_t *sequence);

/**
 * Update the lifecycle status of an entry.
 *
 * @param sequence: sequence number of the entry.
 * @param status: new status.
 * @return zero on success, -ENOENT if no entry has that sequence number.
 */
int messages_set_status(uint32_t sequence, enum message_status status);

/**
 * Set or clear the unread flag of an entry.
 *
 * @param idx: index of the entry, as for messages_get().
 * @param read: true to mark the entry as read, false as unread.
 * @return zero on success, -ENOENT if idx is out of range.
 */
int messages_mark_read(size_t idx, bool read);

/**
 * Delete an entry.
 *
 * @param idx: index of the entry, as for messages_get().
 * @return zero on success, -ENOENT if idx is out of range.
 */
int messages_delete(size_t idx);

#ifdef __cplusplus
}
#endif

#endif /* MESSAGES_H */
