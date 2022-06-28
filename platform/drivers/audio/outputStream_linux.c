/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#include <assert.h>
#include <interfaces/audio_stream.h>
#include <pulse/error.h>
#include <pulse/simple.h>
#include <stddef.h>
#include <stdio.h>

static int    priority     = PRIO_BEEP; // Current priority
static bool   running      = false;     // Stream is running
pa_simple *s = NULL;                    // Pulseaudio instance
int error;                              // Error code

streamId outputStream_start(const enum AudioSink destination,
                            const enum AudioPriority prio,
                            stream_sample_t * const buf,
                            const size_t length,
                            const enum BufMode mode,
                            const uint32_t sampleRate)
{
    assert(destination == SINK_SPK && "Only speaker sink was implemented!\n");
    assert(mode == BUF_LINEAR && "Only linear buffering was implemented!\n");

    /* The Sample format to use */
    static pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 0,
        .channels = 1
    };

    ss.rate = sampleRate;

    // Check if an output stream is already opened and, in case, handle priority.
    if(running)
    {
        if((int) prio < priority) return -1;          // Lower priority, reject.
        if((int) prio > priority) outputStream_stop(0);     // Higher priority, takes over.
        outputStream_sync(0, false);            // Same priority, wait.
    }

    // Assign priority and set stream as running
    priority = prio;
    running  = true;

    if (!s)
    {
        if (!(s = pa_simple_new(NULL,
                                "OpenRTX",
                                PA_STREAM_PLAYBACK,
                                NULL,
                                "playback",
                                &ss,
                                NULL,
                                NULL,
                                &error)))
        {
            fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
            return -1;
        }
    }

    if (pa_simple_write(s, buf, length, &error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
        return -1;
    }

    return 0;
}

stream_sample_t *outputStream_getIdleBuffer(const streamId id)
{
    (void) id;

    return NULL;
}

bool outputStream_sync(const streamId id, const bool bufChanged)
{
    (void) id;
    (void) bufChanged;

    /* Make sure that every single sample was played */
    if (pa_simple_drain(s, &error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
        return false;
    }

    running = false;

    return true;
}

void outputStream_stop(const streamId id)
{
    (void) id;

    /* Make sure that every single sample was played */
    if (pa_simple_flush(s, &error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
    }

    running = false;
}

void outputStream_terminate(const streamId id)
{
    (void) id;

    if (s)
        pa_simple_free(s);
}
