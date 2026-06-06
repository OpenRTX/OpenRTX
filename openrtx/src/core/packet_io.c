/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * packet_io.c — Thread-safe packet TX/RX queue between the UI and RTX threads.
 *
 * Provides two single-slot queues (one per direction) protected by a shared
 * mutex.  The mutex is held only for the duration of a memcpy, keeping
 * contention negligible.
 *
 * This module is part of the message inbox infrastructure.  It is compiled
 * into all builds.
 */

#include "core/packet_io.h"

#include <string.h>
#include <pthread.h>

static pthread_mutex_t pkt_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Single-slot TX queue (UI → RTX). */
static pkt_tx_request_t tx_slot;
static bool tx_pending = false;

/* Single-slot RX queue (RTX → UI). */
static pkt_rx_event_t rx_slot;
static bool rx_pending = false;

void packet_io_init(void)
{
    /* Mutex is statically initialised via PTHREAD_MUTEX_INITIALIZER.
     * This function only resets the queue pending flags, so it is safe
     * to call in test setups and before each test case. */
    pthread_mutex_lock(&pkt_mutex);
    tx_pending = false;
    rx_pending = false;
    pthread_mutex_unlock(&pkt_mutex);
}

bool packet_io_enqueue_tx(const pkt_tx_request_t *req)
{
    bool ok = false;
    pthread_mutex_lock(&pkt_mutex);
    if (!tx_pending) {
        memcpy(&tx_slot, req, sizeof(tx_slot));
        tx_pending = true;
        ok = true;
    }
    pthread_mutex_unlock(&pkt_mutex);
    return ok;
}

bool packet_io_dequeue_tx(pkt_tx_request_t *req)
{
    bool ok = false;
    pthread_mutex_lock(&pkt_mutex);
    if (tx_pending) {
        memcpy(req, &tx_slot, sizeof(*req));
        tx_pending = false;
        ok = true;
    }
    pthread_mutex_unlock(&pkt_mutex);
    return ok;
}

bool packet_io_enqueue_rx(const pkt_rx_event_t *evt)
{
    bool ok = false;
    pthread_mutex_lock(&pkt_mutex);
    if (!rx_pending) {
        memcpy(&rx_slot, evt, sizeof(rx_slot));
        rx_pending = true;
        ok = true;
    }
    pthread_mutex_unlock(&pkt_mutex);
    return ok;
}

bool packet_io_dequeue_rx(pkt_rx_event_t *evt)
{
    bool ok = false;
    pthread_mutex_lock(&pkt_mutex);
    if (rx_pending) {
        memcpy(evt, &rx_slot, sizeof(*evt));
        rx_pending = false;
        ok = true;
    }
    pthread_mutex_unlock(&pkt_mutex);
    return ok;
}
