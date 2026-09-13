/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MESSAGE_DISPATCHER_H
#define MESSAGE_DISPATCHER_H

#include <stddef.h>
#include <stdint.h>
#include "core/messages.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Message dispatcher: moves data packets between the rtx stage and the
 * protocol sources feeding the message registry.
 *
 * The dispatcher owns a small pool of packet descriptors and buffers, and
 * knows nothing about their content. Running from the UI thread, it keeps a
 * receive descriptor submitted to the rtx stage whenever a source is
 * registered for the current operating mode, and hands every completed one
 * to that source for parsing. On the transmit side it lets a source format an
 * outgoing message into its buffer, submits it, and reports the outcome back
 * to the registry.
 *
 * The packet descriptor is the only thing crossing the thread boundary, and
 * rtx_addPacketRx()/rtx_addPacketTx() are safe to call from the UI thread.
 */

/*
 * Size of a packet buffer, in bytes: the largest packet any protocol source
 * can receive or transmit. An M17 packet holds 20 bytes of addressing plus
 * up to 825 bytes of payload.
 */
#define MSG_PKT_MAX_SIZE 845

/* Maximum number of protocol sources that can be registered. */
#ifndef CONFIG_MESSAGES_MAX_SOURCES
#define CONFIG_MESSAGES_MAX_SOURCES 2
#endif

struct pktDesc;

/**
 * Protocol source: translates between packets and message entries.
 *
 * A source is stateless as far as the inbox is concerned: it never keeps
 * messages of its own, it only parses or formats them.
 */
struct message_ops {
    /**
     * Parse a received packet and file its content in the registry through
     * messages_store().
     *
     * @param pkt: completed receive descriptor, valid for the duration of
     * the call only.
     * @return zero on success, a negative error code otherwise.
     */
    int (*process_rx)(const struct pktDesc *pkt);

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
    int (*format_tx)(const struct message *msg, struct pktDesc *pkt);

    const char *name; /**< Display name, e.g. "M17 SMS". */
    uint8_t mode;     /**< Operating mode served, enum opmode. */
};

/**
 * Initialise the message dispatcher, dropping any registered source.
 */
void message_dispatcher_init(void);

/**
 * Register a protocol source.
 *
 * @param ops: source to register, must stay valid until the next call to
 * message_dispatcher_init().
 * @return zero on success, -EINVAL on bad arguments, -EEXIST if a source is
 * already registered for the same operating mode, -ENOSPC if the source
 * table is full.
 */
int message_dispatcher_register(const struct message_ops *ops);

/**
 * Get the source registered for an operating mode.
 *
 * @param mode: operating mode, enum opmode.
 * @return the source, or NULL if none is registered for that mode.
 */
const struct message_ops *message_dispatcher_source(uint8_t mode);

/**
 * Periodic update, to be called once per UI loop iteration: submits receive
 * descriptors, forwards completed packets to their source and reports
 * transmission outcomes to the registry.
 *
 * @param mode: current operating mode, enum opmode.
 */
void message_dispatcher_task(uint8_t mode);

/**
 * Transmit a message through the source registered for its operating mode.
 * The outcome is reported asynchronously through messages_set_status().
 *
 * @param msg: message to transmit, already stored in the registry.
 * @return zero on success, -ENOENT if no source is registered for the
 * message's operating mode, -EBUSY if a transmission is already in progress,
 * or the error returned by the source or by the rtx stage.
 */
int message_dispatcher_send(const struct message *msg);

#ifdef __cplusplus
}
#endif

#endif /* MESSAGE_DISPATCHER_H */
