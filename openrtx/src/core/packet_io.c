/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * packet_io.c — Thread-safe packet TX/RX queues between the UI and RTX
 * threads.
 *
 * Provides three single-slot queues protected by a shared mutex: outbound
 * TX requests (UI -> RTX), inbound RX events (RTX -> UI), and TX completion
 * events (RTX -> UI). RX and TX-completion are kept as separate queues
 * (rather than one event type covering both, as an earlier design did) so
 * that a completion notification can never block or be blocked by an
 * inbound message — the two events have very different sizes and arrival
 * patterns and have no reason to contend for the same slot.
 *
 * The mutex is held only for the duration of a memcpy, keeping contention
 * negligible.
 *
 * This module is part of the message inbox infrastructure.
 *
 * Note: pthread_mutex_t is used directly, consistent with the rest of the
 * codebase (state.c, queue.c, chan.c, audio_codec.c).  All supported targets
 * provide pthread compatibility: Linux (glibc), MIOSIX (libmiosix-kernel),
 * and Zephyr/ESP32S3 (newlib-nano pthread shim).  If a future target lacks
 * this, the mutex implementation must be abstracted behind an OS layer.
 */

#include "hwconfig.h"

#include "core/packet_io.h"

#include <string.h>
#include <pthread.h>

static pthread_mutex_t pkt_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Single-slot TX request queue (UI -> RTX). */
static struct pkt_tx_request tx_slot;
static bool tx_pending = false;

/* Single-slot RX event queue (RTX -> UI). */
static struct pkt_rx_event rx_slot;
static bool rx_pending = false;

/* Single-slot TX completion queue (RTX -> UI). */
static struct pkt_tx_done tx_done_slot;
static bool tx_done_pending = false;

void packet_io_init(void)
{
    /* Mutex is statically initialised via PTHREAD_MUTEX_INITIALIZER.
     * This function resets the queue pending flags and zeroes slot data,
     * so it is safe to call in test setups and before each test case. */
    pthread_mutex_lock(&pkt_mutex);
    memset(&tx_slot, 0, sizeof(tx_slot));
    tx_pending = false;
    memset(&rx_slot, 0, sizeof(rx_slot));
    rx_pending = false;
    memset(&tx_done_slot, 0, sizeof(tx_done_slot));
    tx_done_pending = false;
    pthread_mutex_unlock(&pkt_mutex);
}

/**
 * Shared single-slot enqueue policy: copy @p size bytes from @p src into
 * @p slot iff @p *pending is currently false.
 */
static bool slot_enqueue(void *slot, size_t size, bool *pending,
                         const void *src)
{
    bool ok = false;
    pthread_mutex_lock(&pkt_mutex);
    if (!*pending) {
        memcpy(slot, src, size);
        *pending = true;
        ok = true;
    }
    pthread_mutex_unlock(&pkt_mutex);
    return ok;
}

/**
 * Shared single-slot dequeue policy: copy @p size bytes from @p slot into
 * @p dst iff @p *pending is currently true.
 */
static bool slot_dequeue(void *slot, size_t size, bool *pending, void *dst)
{
    bool ok = false;
    pthread_mutex_lock(&pkt_mutex);
    if (*pending) {
        memcpy(dst, slot, size);
        *pending = false;
        ok = true;
    }
    pthread_mutex_unlock(&pkt_mutex);
    return ok;
}

bool packet_io_enqueue_tx(const struct pkt_tx_request *req)
{
    return slot_enqueue(&tx_slot, sizeof(tx_slot), &tx_pending, req);
}

bool packet_io_dequeue_tx(struct pkt_tx_request *req)
{
    return slot_dequeue(&tx_slot, sizeof(tx_slot), &tx_pending, req);
}

bool packet_io_enqueue_rx(const struct pkt_rx_event *evt)
{
    return slot_enqueue(&rx_slot, sizeof(rx_slot), &rx_pending, evt);
}

bool packet_io_dequeue_rx(struct pkt_rx_event *evt)
{
    return slot_dequeue(&rx_slot, sizeof(rx_slot), &rx_pending, evt);
}

bool packet_io_enqueue_tx_done(const struct pkt_tx_done *done)
{
    return slot_enqueue(&tx_done_slot, sizeof(tx_done_slot), &tx_done_pending,
                        done);
}

bool packet_io_dequeue_tx_done(struct pkt_tx_done *done)
{
    return slot_dequeue(&tx_done_slot, sizeof(tx_done_slot), &tx_done_pending,
                        done);
}
