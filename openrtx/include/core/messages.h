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
 * The module owns all message storage: a table of entries plus a shared,
 * variable-length body pool. Protocol sources (M17 SMS, APRS, ...) are
 * registered at run time as a pair of translators between packets and
 * entries: they never keep messages of their own, and file inbound messages
 * here through messages_store().
 *
 * Packets move between the rtx stage and the sources through a small set of
 * packet descriptors and buffers owned here and treated as opaque bytes: a
 * receive descriptor is kept submitted whenever a source is registered for
 * the current operating mode, each completed one is handed to that source
 * for parsing, and on transmit the source formats the outgoing message into
 * a buffer which is then submitted, with the outcome reported to the entry.
 * The packet descriptor is the only thing crossing the thread boundary: the
 * opmodes take submissions through a lock-protected queue or an atomic slot,
 * so rtx_addPacketRx()/rtx_addPacketTx() are called from the UI thread and
 * the rtx thread reports completion by updating the descriptor status.
 *
 * Storage is allocated from the heap when the operating mode switches to one
 * with a registered source and released when it switches to one without, so
 * modes that cannot exchange messages pay nothing; the inbox content is
 * dropped on the way out. The release waits until the rtx stage has handed
 * every descriptor back, which the opmodes do when they are disabled.
 * Sequence numbers keep increasing across releases, so a stale one never
 * matches a later entry.
 *
 * Entries are kept in arrival order and exposed newest-first. When the entry
 * table or the body pool is full, the oldest entries are evicted, in order,
 * until there is room for the new one.
 *
 * Targets enable the inbox by defining CONFIG_MESSAGES; the module itself
 * compiles unconditionally.
 *
 * Threading: every function in this header must be called from the UI thread.
 * Pointers returned by messages_get() are valid only until the next call to
 * messages_store(), messages_send(), messages_delete(), messages_task(),
 * messages_init() or messages_terminate(); UI code must not cache them
 * across ticks and must pin an entry by its sequence number instead
 * (messages_findBySequence()).
 */

/*
 * Maximum message body length, including the NUL terminator. Sized for the
 * largest body any protocol can carry in a single packet: an M17 packet-mode
 * SMS holds up to 821 characters. Widen it if a future protocol needs more.
 */
#define MSG_BODY_MAX_LEN 822

/* Maximum address (callsign) length, including the NUL terminator. */
#define MSG_ADDR_MAX_LEN 10

/*
 * Size of a packet buffer, in bytes: the largest packet any protocol source
 * can receive or transmit. An M17 packet holds 20 bytes of addressing plus
 * up to 825 bytes of payload.
 */
#define MSG_PKT_MAX_SIZE 845

/**
 * Direction of a message.
 */
enum messageDirection {
    MSG_DIR_RX = 0, /**< Received message.    */
    MSG_DIR_TX = 1, /**< Transmitted message. */
};

/**
 * Lifecycle status of a message.
 */
enum messageStatus {
    MSG_STATUS_RECEIVED = 0, /**< Successfully received.    */
    MSG_STATUS_SENDING,      /**< Transmission in progress. */
    MSG_STATUS_SENT,         /**< Transmission completed.   */
    MSG_STATUS_FAILED,       /**< Transmission failed.      */
};

/**
 * A message entry.
 *
 * Also used as the template passed to messages_store(): the caller fills in
 * everything except sequence, and body may point anywhere (typically into a
 * packet buffer) since the module copies it into its own pool.
 */
struct message {
    const char *body;  /**< NUL-terminated body, module-owned.        */
    uint32_t sequence; /**< Unique, monotonic; zero means unset.      */
    uint16_t bodyLen;  /**< Body length, not counting the NUL.        */
    uint16_t type;     /**< Protocol-specific message type.           */
    uint8_t mode;      /**< Originating opmode, enum opmode.          */
    uint8_t direction; /**< enum messageDirection.                    */
    uint8_t status;    /**< enum messageStatus.                       */
    uint8_t unread;    /**< Nonzero until the user has seen it.       */
    char sender[MSG_ADDR_MAX_LEN];    /**< Originating address.        */
    char recipient[MSG_ADDR_MAX_LEN]; /**< Destination address.        */
};

struct pktDesc;

/**
 * Protocol source: translates between packets and message entries.
 *
 * A source is stateless as far as the inbox is concerned: it never keeps
 * messages of its own, it only parses or formats them.
 */
struct messageOps {
    /**
     * Parse a received packet and file its content in the inbox through
     * messages_store().
     *
     * On entry pkt->buffer holds the packet in the layout of the operating
     * mode that received it, pkt->res the number of bytes the opmode
     * reported and pkt->size the buffer size as submitted.
     *
     * @param pkt: completed receive descriptor, valid for the duration of
     * the call only.
     * @return zero on success, a negative error code otherwise.
     */
    int (*processRx)(const struct pktDesc *pkt);

    /**
     * Format an outgoing message into a packet buffer.
     *
     * On entry pkt->buffer and pkt->size describe the buffer available; on
     * success the source has written the packet there and set pkt->size to
     * the number of bytes to transmit.
     *
     * @param msg: message to transmit.
     * @param pkt: transmit descriptor to fill.
     * @return zero on success, a negative error code otherwise.
     */
    int (*formatTx)(const struct message *msg, struct pktDesc *pkt);

    const char *name; /**< Display name, e.g. "M17 SMS". */
    uint8_t mode;     /**< Operating mode served, enum opmode. */
};

/**
 * Initialise the message inbox, dropping any registered source and any
 * stored entry. Storage is not allocated here but by messages_task(), once
 * the operating mode has a source. Must not be called while the rtx stage
 * holds a packet descriptor, that is, after messages_task() has run in a
 * mode with a source.
 */
void messages_init(void);

/**
 * Shut down the message inbox, releasing its storage. The storage is left
 * allocated if the rtx stage still holds a packet descriptor, since the
 * descriptor lives inside it.
 */
void messages_terminate(void);

/**
 * Register a protocol source.
 *
 * @param ops: source to register, must stay valid until the next call to
 * messages_init().
 * @return zero on success, -EINVAL on bad arguments, -EEXIST if a source is
 * already registered for the same operating mode, -ENOSPC if the source
 * table is full.
 */
int messages_registerSource(const struct messageOps *ops);

/**
 * Periodic update, to be called once per UI loop iteration: allocates or
 * releases the storage following the operating mode, submits receive
 * descriptors, forwards completed packets to their source and reports
 * transmission outcomes to their entry.
 *
 * @param mode: current operating mode, enum opmode.
 * @return number of unread messages received since the previous call, zero
 *         while the inbox is inactive.
 */
size_t messages_task(uint8_t mode);

/**
 * Get the number of stored entries.
 *
 * @return entry count, zero while the inbox is inactive.
 */
size_t messages_count(void);

/**
 * Get the number of unread entries.
 *
 * @return unread entry count.
 */
size_t messages_countUnread(void);

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
size_t messages_findBySequence(uint32_t sequence);

/**
 * Store a new entry, copying its body into the inbox pool. The oldest
 * entries are evicted if there is no room for the new one.
 *
 * @param msg: entry template, see struct message.
 * @return the sequence number assigned, always positive, or -EINVAL on bad
 *         arguments, -EMSGSIZE if the body is longer than
 *         MSG_BODY_MAX_LEN - 1, -ENODEV if the inbox is inactive.
 */
int32_t messages_store(const struct message *msg);

/**
 * Update the lifecycle status of an entry.
 *
 * @param sequence: sequence number of the entry.
 * @param status: new status.
 * @return zero on success, -ENOENT if no entry has that sequence number.
 */
int messages_setStatus(uint32_t sequence, enum messageStatus status);

/**
 * Set or clear the unread flag of an entry.
 *
 * @param idx: index of the entry, as for messages_get().
 * @param read: true to mark the entry as read, false as unread.
 * @return zero on success, -ENOENT if idx is out of range.
 */
int messages_markRead(size_t idx, bool read);

/**
 * Delete an entry.
 *
 * @param idx: index of the entry, as for messages_get().
 * @return zero on success, -ENOENT if idx is out of range.
 */
int messages_delete(size_t idx);

/**
 * Check whether messages can be composed and sent in an operating mode,
 * that is, whether a protocol source is registered for it and the inbox is
 * active.
 *
 * @param mode: operating mode, enum opmode.
 * @return true if messages can be sent in that mode.
 */
bool messages_canCompose(uint8_t mode);

/**
 * Send a message: store it as an outgoing entry and hand it to the protocol
 * source registered for its operating mode. The entry is created with
 * MSG_STATUS_SENDING and updated to MSG_STATUS_SENT or MSG_STATUS_FAILED once
 * the transmission completes; if the message cannot be handed over at all the
 * entry is left as MSG_STATUS_FAILED.
 *
 * @param msg: template with mode, sender, recipient and body filled in.
 * @return zero on success, -ENOENT if no source is registered for the mode,
 *         -ENODEV if the inbox is inactive, -EBUSY if a transmission is
 *         already in progress, or another negative error code from
 *         messages_store() or the source.
 */
int messages_send(const struct message *msg);

#ifdef __cplusplus
}
#endif

#endif /* MESSAGES_H */
