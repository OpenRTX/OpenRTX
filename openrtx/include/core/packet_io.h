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

/* Maximum message body length including NUL terminator. */
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
typedef struct {
    uint8_t mode_id;
    char dst[PKT_ADDR_MAX_LEN];
    char body[PKT_BODY_MAX_LEN];
    size_t body_len;
    uint32_t tag;
} pkt_tx_request_t;

/**
 * Inbound RX event or TX completion event posted by the RTX thread and
 * consumed by the UI thread (via messages_tick).
 *
 * For RX events: mode_id identifies the protocol; src holds the sender;
 * body/body_len hold the decoded text; status is MSG_STATUS_RECEIVED (0).
 *
 * For TX completion events: body is empty; status is MSG_STATUS_SENT or
 * MSG_STATUS_FAILED; tag matches the originating pkt_tx_request_t.
 */
typedef struct {
    uint8_t mode_id;
    char src[PKT_ADDR_MAX_LEN];
    char body[PKT_BODY_MAX_LEN];
    size_t body_len;
    uint32_t tag;
    uint8_t status; /**< message_status_t value */
} pkt_rx_event_t;

/**
 * Initialise the packet I/O queues and their shared mutex.
 * Called once from create_threads() before either thread starts.
 */
void packet_io_init(void);

/**
 * Post a TX request from the UI thread.
 * @return true if enqueued, false if the single-slot queue is full.
 */
bool packet_io_enqueue_tx(const pkt_tx_request_t *req);

/**
 * Dequeue a pending TX request from the RTX thread.
 * @return true if a request was available and copied into @p req.
 */
bool packet_io_dequeue_tx(pkt_tx_request_t *req);

/**
 * Post an RX event or TX completion from the RTX thread.
 * @return true if enqueued, false if the single-slot queue is full.
 */
bool packet_io_enqueue_rx(const pkt_rx_event_t *evt);

/**
 * Dequeue a pending RX event from the UI thread (via messages_tick).
 * @return true if an event was available and copied into @p evt.
 */
bool packet_io_dequeue_rx(pkt_rx_event_t *evt);

#ifdef __cplusplus
}
#endif

#endif /* PACKET_IO_H */
