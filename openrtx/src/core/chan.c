/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/chan.h"

void chan_init(chan_t *c)
{
    if (c == NULL) return;

    pthread_mutex_init(&c->m_meta, NULL);
    pthread_mutex_init(&c->m_read, NULL);
    pthread_mutex_init(&c->m_write, NULL);

    pthread_cond_init(&c->c_reader, NULL);
    pthread_cond_init(&c->c_writer, NULL);

    c->reader = false;
    c->writer = false;
    c->closed = false;
}

void chan_send(chan_t *c, void *data)
{
    pthread_mutex_lock(&c->m_write);
    pthread_mutex_lock(&c->m_meta);

    if (c->closed)
    {
        pthread_mutex_unlock(&c->m_meta);
        pthread_mutex_unlock(&c->m_write);

        return;
    }

    c->data = data;
    c->writer = true;

    // notify the waiting reader that data is ready
    if (c->reader)
    {
        pthread_cond_signal(&c->c_writer);
    }

    // wait until data is consumed
    pthread_cond_wait(&c->c_reader, &c->m_meta);
    c->writer = false;

    pthread_mutex_unlock(&c->m_meta);
    pthread_mutex_unlock(&c->m_write);
}

void chan_recv(chan_t *c, void **data)
{
    pthread_mutex_lock(&c->m_read);
    pthread_mutex_lock(&c->m_meta);

    // wait for a writer
    while (!c->closed && !c->writer)
    {
        c->reader = true;
        pthread_cond_wait(&c->c_writer, &c->m_meta);
        c->reader = false;
    }

    if (c->closed)
    {
        pthread_mutex_unlock(&c->m_meta);
        pthread_mutex_unlock(&c->m_read);

        return;
    }

    if (data != NULL)
    {
        *data = c->data;
    }

    // notify the waiting writer that the reader consumed the data
    pthread_cond_signal(&c->c_reader);

    pthread_mutex_unlock(&c->m_meta);
    pthread_mutex_unlock(&c->m_read);
}


bool chan_can_recv(chan_t *c)
{
    pthread_mutex_lock(&c->m_meta);
    bool can_receive = c->writer;
    pthread_mutex_unlock(&c->m_meta);

    return can_receive;
}

bool chan_can_send(chan_t *c)
{
    pthread_mutex_lock(&c->m_meta);
    bool can_send = c->reader;
    pthread_mutex_unlock(&c->m_meta);

    return can_send;
}

void chan_close(chan_t *c)
{
    pthread_mutex_lock(&c->m_meta);
    if (!c->closed)
    {
        c->closed = true;
        pthread_cond_broadcast(&c->c_reader);
        pthread_cond_broadcast(&c->c_writer);
    }
    pthread_mutex_unlock(&c->m_meta);
}

void chan_terminate(chan_t *c)
{
    chan_close(c);

    pthread_mutex_destroy(&c->m_write);
    pthread_mutex_destroy(&c->m_read);
    pthread_mutex_destroy(&c->m_meta);

    pthread_cond_destroy(&c->c_writer);
    pthread_cond_destroy(&c->c_reader);
}
