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

#include <audio_stream.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pthread.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

// Expand opaque pa_simple struct
struct pa_simple
{
    pa_threaded_mainloop *mainloop;
    pa_context           *context;
    pa_stream            *stream;
    pa_stream_direction_t direction;
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
static pa_simple         *paInstance = NULL;        // Pulseaudio instance
static size_t             remaining  = 0;
static pthread_cond_t     barrier;
static pthread_mutex_t    mutex;

static void buf_circ_write_cb(pa_stream* s, size_t length, void* userdata)
{
    (void) userdata;

    if((s == NULL) || (length <= 0))
        return;

    if(length > remaining)
    {
        // We can play all the rest of the buffer
        pa_stream_write(s, playBuf, remaining * sizeof(stream_sample_t),
                        NULL, 0, PA_SEEK_RELATIVE);
        remaining = 0;
    }
    else
    {
        pa_stream_write(s, playBuf, length, NULL, 0, PA_SEEK_RELATIVE);

        if (remaining > length)
            remaining -= length;
        else
            remaining = 0;
    }

    // All data in playBuffer has been sent
    if(remaining == 0)
    {
        // Reload counter
        remaining = bufLen/2;

        pthread_mutex_lock(&mutex);

        // Swap idle and play buffers
        stream_sample_t *tmp = idleBuf;
        playBuf = idleBuf;
        idleBuf = tmp;

        // Unlock waiting threads
        pthread_cond_signal(&barrier);
        pthread_mutex_unlock(&mutex);
    }
}

streamId outputStream_start(const enum AudioSink destination,
                            const enum AudioPriority prio,
                            stream_sample_t* const buffer,
                            const size_t length,
                            const enum BufMode mode,
                            const uint32_t sampleRate)
{

    if(destination != SINK_SPK)
        return -1;

    // Check if an output stream is already opened and, in case, handle
    // priority.
    if(running)
    {
        if(prio < priority) return -1;             // Lower priority, reject.
        if(prio > priority) outputStream_stop(0);  // Higher priority, takes over.
        while(running) ;                           // Same priority, wait.
    }

    // Assign priority and set stream as running
    running   = true;
    priority  = prio;
    bufMode   = mode;
    playBuf   = buffer;
    idleBuf   = buffer + (length/2);
    bufLen    = length;
    remaining = length/2;

    int  paError = 0;
    bool success = true;

    if(paInstance == NULL)
    {
        // Stream data sample format
        static pa_sample_spec spec;
        spec.format   = PA_SAMPLE_S16LE;
        spec.rate     = 0;
        spec.channels = 1;
        spec.rate     = sampleRate;

        paInstance = pa_simple_new(NULL, "OpenRTX", PA_STREAM_PLAYBACK, NULL,
                                   "Audio out", &spec, NULL, NULL, &paError);

        if(paInstance == NULL)
        {
            fprintf(stderr, __FILE__ ": pa_simple_new() failed: %s\n",
                    pa_strerror(paError));

            success = false;
        }
        else
        {
            pthread_mutex_init(&mutex, NULL);
            pthread_cond_init(&barrier, NULL);
        }
    }

    switch(mode)
    {
        case BUF_LINEAR:
            if(pa_simple_write(paInstance, buffer, length, &paError) < 0)
                success = false;
            break;

        case BUF_CIRC_DOUBLE:
        {
            if(paInstance->stream == NULL)
            {
                success = false;
                break;
            }

            // Register write callback
            pa_stream_set_write_callback(paInstance->stream, buf_circ_write_cb,
                                         NULL);

            // Set minimal prebuffering
            const pa_buffer_attr attr =
            {
                .fragsize  = -1,
                .maxlength = -1,
                .minreq    = -1,
                .prebuf    = 320,
                .tlength   = -1,
            };

            pa_stream_set_buffer_attr(paInstance->stream, &attr, NULL, NULL);

            // Get maximum pulse buffer size
            size_t wsize = pa_stream_writable_size(paInstance->stream);
            if(wsize > (length / 2))
                wsize = length / 2;

            // Start writing loop
            pa_stream_write(paInstance->stream, playBuf, wsize, NULL, 0,
                            PA_SEEK_RELATIVE);
        }
            break;
    }

    if(success == false)
    {
        running  = false;
        priority = PRIO_BEEP;
        return -1;
    }

    return 0;
}

stream_sample_t *outputStream_getIdleBuffer(const streamId id)
{
    (void) id;

    stream_sample_t *ptr = NULL;

    if(bufMode == BUF_CIRC_DOUBLE)
    {
        pthread_mutex_lock(&mutex);
        ptr = idleBuf;
        pthread_mutex_unlock(&mutex);
    }

    return ptr;
}

bool outputStream_sync(const streamId id, const bool bufChanged)
{
    (void) id;
    (void) bufChanged;

    if(bufMode == BUF_CIRC_DOUBLE)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&barrier, &mutex);
        pthread_mutex_unlock(&mutex);
    }

    //
    // TODO syncronisation barrrier also for linear buffer mode
    //

    return true;
}

void outputStream_stop(const streamId id)
{
    (void) id;

    int error = 0;
    if (pa_simple_flush(paInstance, &error) < 0)
    {
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n",
                pa_strerror(error));
    }

    running  = false;
    priority = PRIO_BEEP;
}

void outputStream_terminate(const streamId id)
{
    (void) id;

    running  = false;
    priority = PRIO_BEEP;

    if(paInstance != NULL)
    {
        pa_simple_free(paInstance);
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&barrier);
    }
}
