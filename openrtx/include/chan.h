/***************************************************************************
 *   Copyright (C) 2021 by Alessio Caiazza IU5BON                          *
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
#ifndef CHAN_H
#define CHAN_H

#include <pthread.h>
#include <stdbool.h>

/*
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

void chan_init(chan_t *c);
void chan_send(chan_t *c, void *data);
void chan_recv(chan_t *c, void **data);
bool chan_can_recv(chan_t *c);
bool chan_can_send(chan_t *c);
void chan_close(chan_t *c);
void chan_terminate(chan_t *c);

#endif
