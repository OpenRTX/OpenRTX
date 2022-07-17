/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

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
