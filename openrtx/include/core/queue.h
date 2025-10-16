/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Ring buffer size (MAX = 255)
#define MSG_QTY 10

typedef struct queue_t
{
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    uint8_t read_pos;
    uint8_t write_pos;
    uint8_t msg_num;
    uint32_t buffer[MSG_QTY];
}
queue_t;

/**
 *
 */
void queue_init(queue_t *q);

/**
 *
 */
void queue_terminate(queue_t *q);

/**
 *
 */
bool queue_pend(queue_t *q, uint32_t *msg, bool blocking);

/**
 *
 */
bool queue_post(queue_t *q, uint32_t msg);

#endif
