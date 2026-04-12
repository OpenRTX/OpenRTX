/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rtx/PktBuf.hpp"
#include <cstring>

PktBuf::PktBuf() : head(0), tail(0), count(0)
{
    pthread_mutex_init(&mutex, NULL);
    memset(buf, 0, sizeof(buf));
}

PktBuf::~PktBuf()
{
    pthread_mutex_destroy(&mutex);
}

bool PktBuf::push(const rtxPacket_t *pkt)
{
    pthread_mutex_lock(&mutex);

    bool overwrite = false;

    if (count == PKTBUF_SLOTS)
    {
        /* Queue full — drop the oldest entry */
        tail = (tail + 1) % PKTBUF_SLOTS;
        count--;
        overwrite = true;
    }

    memcpy(&buf[head], pkt, sizeof(rtxPacket_t));
    head = (head + 1) % PKTBUF_SLOTS;
    count++;

    pthread_mutex_unlock(&mutex);
    return !overwrite;
}

bool PktBuf::pop(rtxPacket_t *pkt)
{
    pthread_mutex_lock(&mutex);

    if (count == 0)
    {
        pthread_mutex_unlock(&mutex);
        return false;
    }

    memcpy(pkt, &buf[tail], sizeof(rtxPacket_t));
    tail = (tail + 1) % PKTBUF_SLOTS;
    count--;

    pthread_mutex_unlock(&mutex);
    return true;
}

size_t PktBuf::pending()
{
    pthread_mutex_lock(&mutex);
    size_t n = count;
    pthread_mutex_unlock(&mutex);
    return n;
}

void PktBuf::clear()
{
    pthread_mutex_lock(&mutex);
    head  = 0;
    tail  = 0;
    count = 0;
    pthread_mutex_unlock(&mutex);
}
