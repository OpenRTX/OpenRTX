/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CHAN_H
#define CHAN_H

#include <pthread.h>
#include <stdbool.h>

/**
 * chan_t is an unbuffered synchronization channel.
 * Both reader and writer are blocked untill the data is exchanged.
 */
typedef struct chan_t
{
    pthread_mutex_t m_meta;
    pthread_mutex_t m_read;
    pthread_mutex_t m_write;
    pthread_cond_t c_reader;
    pthread_cond_t c_writer;

    void *data;
    bool closed;
    bool reader;
    bool writer;
}
chan_t;

/**
 * This function initializes a channel.
 *
 * @param c: the cannel to initialize.
 */
void chan_init(chan_t *c);

/**
 * This function writes data into a channel.
 * This is a synchronous write and the function blocks until a read happens.
 *
 * @param c: the channel.
 * @param data: the data to publish on the channel.
 */
void chan_send(chan_t *c, void *data);

/**
 * This function reads data from a channel.
 * This is a synchronous read and the function blocks until a write happens.
 *
 * @param c: the channel.
 * @param data: the read data will be assigned to this pointer.
 */
void chan_recv(chan_t *c, void **data);

/**
 * This function check if the channel has a writer waiting with data.
 *
 * @param c: the channel.
 * @return true if a writer is waiting on the channel.
 */
bool chan_can_recv(chan_t *c);

/**
 * This function check if the channel has a reader waiting for data.
 *
 * @param c: the channel.
 * @return true if a reader is waiting on the channel.
 */
bool chan_can_send(chan_t *c);

/**
 * This function closes a channel.
 * When a channel is closed, it is no longer possible to read or write from it.
 *
 * @param c: the channel.
 */
void chan_close(chan_t *c);

/**
 * Destructor function to de-allocate a channel.
 *
 * @param c: the channel.
 */
void chan_terminate(chan_t *c);

#endif
