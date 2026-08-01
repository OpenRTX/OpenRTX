/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PACKET_IO_H
#define PACKET_IO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Maximum message body length including NUL terminator.
 *
 * 822 = the largest text payload that fits in a single M17 packet-mode
 * frame (25 payload bytes/frame, up to 33 frames per packet transmission,
 * minus framing/addressing overhead), which is the protocol this queue was
 * designed to carry first. If a future source needs a larger body, widen
 * this constant rather than adding a second body queue.
 */
#define PKT_BODY_MAX_LEN 822

/* Maximum address length including NUL terminator. */
#define PKT_ADDR_MAX_LEN 16

/**
 * Outbound TX request posted by the UI thread and consumed by the RTX thread.
 *
 * @param mode_id:   opmode value identifying which protocol handles the TX.
 * @param dst:       NUL-terminated destination callsign / address.
 * @param body:      NUL-terminated message body text.
 * @param body_len:  Length of body not counting NUL.
 * @param tag:       Caller-assigned identifier echoed in the completion event.
 */
struct pkt_tx_request {
    uint8_t mode_id;
    char dst[PKT_ADDR_MAX_LEN];
    char body[PKT_BODY_MAX_LEN];
    size_t body_len;
    uint32_t tag;
};

/**
 * Inbound RX event posted by the RTX thread and consumed by the UI thread
 * (via a source's tick() callback, typically during messages_tick()).
 *
 * Always represents a freshly received message: mode_id identifies the
 * protocol, src holds the sender, body/body_len hold the decoded text.
 * This queue carries message bodies only — TX completion is a separate,
 * much smaller queue (see struct pkt_tx_done) so a completion event can never
 * block, delay, or be blocked by an inbound message.
 */
struct pkt_rx_event {
    uint8_t mode_id;
    char src[PKT_ADDR_MAX_LEN];
    char body[PKT_BODY_MAX_LEN];
    size_t body_len;
};

/**
 * TX completion event posted by the RTX thread and consumed by the UI
 * thread once a previously enqueued struct pkt_tx_request has finished (either
 * transmitted or failed).
 *
 * @param mode_id: opmode value identifying which protocol completed.
 * @param tag:     matches the originating struct pkt_tx_request's tag.
 * @param status:  a enum message_status value — MSG_STATUS_SENT or
 *                 MSG_STATUS_FAILED.
 */
struct pkt_tx_done {
    uint8_t mode_id;
    uint32_t tag;
    uint8_t status;
};

/**
 * Initialise the packet I/O queues and their shared mutex.
 * Called once from create_threads() before either thread starts.
 */
void packet_io_init(void);

/**
 * Post a TX request from the UI thread.
 *
 * If the single-slot queue is already full (a previous request has not yet
 * been dequeued by the RTX thread), the request is dropped and the caller
 * must retry; callers are expected to check the return value and treat a
 * false result as "TX busy", matching the send() ops contract's -EBUSY.
 *
 * @return true if enqueued, false if the single-slot queue is full.
 */
__attribute__((warn_unused_result)) bool
packet_io_enqueue_tx(const struct pkt_tx_request *req);

/**
 * Dequeue a pending TX request from the RTX thread.
 * @return true if a request was available and copied into @p req.
 */
bool packet_io_dequeue_tx(struct pkt_tx_request *req);

/**
 * Post an RX event from the RTX thread.
 *
 * If the single-slot queue is already full (the UI thread has not yet
 * drained the previous event), the newly received message is dropped.
 * Radios in this project receive only one message at a time and the UI
 * thread drains this queue every tick, so this is expected to be rare; a
 * dropped RX event is silent data loss with no retry, which callers must
 * account for by checking the return value.
 *
 * @return true if enqueued, false if the single-slot queue is full.
 */
__attribute__((warn_unused_result)) bool
packet_io_enqueue_rx(const struct pkt_rx_event *evt);

/**
 * Dequeue a pending RX event from the UI thread (typically from within a
 * source's tick() callback during messages_tick()).
 * @return true if an event was available and copied into @p evt.
 */
bool packet_io_dequeue_rx(struct pkt_rx_event *evt);

/**
 * Post a TX completion event from the RTX thread.
 *
 * Same single-slot drop policy as packet_io_enqueue_rx(): if the UI thread
 * has not yet drained the previous completion, this one is dropped and the
 * corresponding inbox entry will remain in MSG_STATUS_SENDING until the
 * source times it out or the next completion arrives.
 *
 * @return true if enqueued, false if the single-slot queue is full.
 */
__attribute__((warn_unused_result)) bool
packet_io_enqueue_tx_done(const struct pkt_tx_done *done);

/**
 * Dequeue a pending TX completion event from the UI thread (typically from
 * within a source's tick() callback during messages_tick()).
 * @return true if an event was available and copied into @p done.
 */
bool packet_io_dequeue_tx_done(struct pkt_tx_done *done);

#ifdef __cplusplus
}
#endif

#endif /* PACKET_IO_H */
