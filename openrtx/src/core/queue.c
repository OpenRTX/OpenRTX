/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include "core/queue.h"

void queue_init(queue_t *q)
{
    if(q == NULL) return;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    q->read_pos  = 0;
    q->write_pos = 0;
    q->msg_num   = 0;
}

void queue_terminate(queue_t *q)
{
    if(q == NULL) return;
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty);
}

bool queue_pend(queue_t *q, uint32_t *msg, bool blocking)
{
    if((q == NULL) || (msg == NULL)) return false;

    pthread_mutex_lock(&q->mutex);

    // The queue is empty
    if(q->msg_num == 0)
    {
        if(blocking)
        {
            while(q->msg_num == 0)
            {
                pthread_cond_wait(&q->not_empty, &q->mutex);
            }
        }
        else
        {
            pthread_mutex_unlock(&q->mutex);
            return false;
        }
    }

    *msg = q->buffer[q->read_pos];

    // Wrap around pointer to make a circular buffer
    q->read_pos = (q->read_pos + 1) % MSG_QTY;
    q->msg_num -= 1;
    pthread_mutex_unlock(&q->mutex);

    return true;
}

bool queue_post(queue_t *q, uint32_t msg)
{
    if(q == NULL) return false;

    pthread_mutex_lock(&q->mutex);

    if(q->msg_num < MSG_QTY)
    {
        q->buffer[q->write_pos] = msg;

        // Wrap around pointer to make a circular buffer
        q->write_pos = (q->write_pos + 1) % MSG_QTY;

        // Signal that the queue is not empty
        if(q->msg_num == 0)
        {
            pthread_cond_signal(&q->not_empty);
        }

        q->msg_num += 1;
    }
    else
    {
        pthread_mutex_unlock(&q->mutex);
        return false;
    }

    pthread_mutex_unlock(&q->mutex);
    return true;
}
