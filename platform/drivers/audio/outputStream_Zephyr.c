/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#include <interfaces/audio_stream.h>
#include <pthread.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

// Expand opaque pa_simple struct
struct pa_simple
{
    const void           *read_data;
    size_t                read_index;
    size_t                read_length;
    int                   operation_success;
};

static enum BufMode       bufMode;                  // Buffer operation mode
static enum AudioPriority priority   = PRIO_BEEP;   // Priority level
static bool               running    = false;       // Stream is running
static size_t             bufLen     = 0;           // Total buffer length
static stream_sample_t   *playBuf    = NULL;        // Buffer being reproduced
static stream_sample_t   *idleBuf    = NULL;        // Idle buffer available to be filled
static size_t             remaining  = 0;
static pthread_cond_t     barrier;
static pthread_mutex_t    mutex;


streamId outputStream_start(const enum AudioSink destination,
                            const enum AudioPriority prio,
                            stream_sample_t* const buffer,
                            const size_t length,
                            const enum BufMode mode,
                            const uint32_t sampleRate)
{
    // TODO
    return 0;
}

stream_sample_t *outputStream_getIdleBuffer(const streamId id)
{
    // TODO
    stream_sample_t *ptr = NULL;
    return ptr;
}

bool outputStream_sync(const streamId id, const bool bufChanged)
{
    // TODO
    return true;
}

void outputStream_stop(const streamId id)
{
    // TODO
    ;
}

void outputStream_terminate(const streamId id)
{
    // TODO
    ;
}
