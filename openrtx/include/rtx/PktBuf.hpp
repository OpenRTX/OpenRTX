/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PKTBUF_H
#define PKTBUF_H

#include "rtx.h"
#include <pthread.h>
#include <cstddef>

/**
 * Number of packet slots in each PktBuf queue.
 */
#define PKTBUF_SLOTS 4

/**
 * \class PktBuf
 * \brief Mutex-protected circular buffer of rtxPacket_t descriptors.
 *
 * Provides a thread-safe FIFO queue for passing packet descriptors between
 * the RTX task and higher-level code. Data is copied on push/pop so that
 * producers and consumers need not coordinate lifetimes.
 */
class PktBuf
{
public:
    PktBuf();
    ~PktBuf();

    /**
     * Push a packet into the queue. If the queue is full the oldest packet
     * is silently dropped before the new one is inserted.
     *
     * @param pkt: pointer to the packet descriptor to copy in.
     * @return true if the packet was inserted without dropping, false if
     *         an overwrite occurred.
     */
    bool push(const rtxPacket_t *pkt);

    /**
     * Pop the oldest packet from the queue.
     *
     * @param pkt: pointer to a descriptor that will receive the copy.
     * @return true if a packet was available, false if the queue was empty.
     */
    bool pop(rtxPacket_t *pkt);

    /**
     * Return the number of packets currently queued.
     */
    size_t pending();

    /**
     * Discard all queued packets.
     */
    void clear();

private:
    rtxPacket_t buf[PKTBUF_SLOTS];
    size_t head;
    size_t tail;
    size_t count;
    pthread_mutex_t mutex;
};

#endif /* PKTBUF_H */
