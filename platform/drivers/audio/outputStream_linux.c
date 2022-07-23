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
#include <pulse/pulseaudio.h>
#include <stddef.h>
#include <stdio.h>

// Expand opaque pa_simple struct
struct pa_simple {
    pa_threaded_mainloop *mainloop;
    pa_context *context;
    pa_stream *stream;
    pa_stream_direction_t direction;

    const void *read_data;
    size_t read_index, read_length;

    int operation_success;
};

static int           priority          = PRIO_BEEP; // Current priority
static bool          running           = false;     // Stream is running
static pa_simple     *p                = NULL;      // Pulseaudio instance
static int           error             = 0;         // Error code
static enum BufMode  buf_mode;                      // Buffer operation mode
static stream_sample_t *buf            = NULL;      // Playback buffer
static size_t        buf_len           = 0;         // Buffer length
static bool          first_half_active = true; // Circular addressing mode flag
static size_t        offset            = 0;         // Playback offset

static void buf_circ_write_cb(pa_stream *s, size_t length, void *userdata)
{
    (void) userdata;
    size_t remaining = 0;

    if (!s || length <= 0)
        return;

    if (offset >= buf_len / 2)
        first_half_active = false;

    remaining = buf_len - offset;

    // We can play all the rest of the buffer
    if (length > remaining)
    {
        pa_stream_write(s, buf + offset, remaining, NULL, 0, PA_SEEK_RELATIVE);

        if(first_half_active == true)
            first_half_active = false;
        else
            first_half_active = true;

        offset = 0;
    }
    else
    {
        pa_stream_write(s, buf + offset, length, NULL, 0, PA_SEEK_RELATIVE);
        offset += length;
    }
}

streamId outputStream_start(const enum AudioSink destination,
                            const enum AudioPriority prio,
                            stream_sample_t * const buffer,
                            const size_t length,
                            const enum BufMode mode,
                            const uint32_t sampleRate)
{
    assert(destination == SINK_SPK && "Only speaker sink was implemented!\n");

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
    buf_mode = mode;
    buf = buffer;
    buf_len = length;
    first_half_active = true;

    if (!p)
    {
        // Create a new playback stream
        if (!(p = pa_simple_new(NULL, "OpenRTX", PA_STREAM_PLAYBACK, NULL, "Audio out", &ss, NULL, NULL, &error))) {
            fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
            return -1;
        }
    }

    if (mode == BUF_LINEAR)
    {
        if (pa_simple_write(p, buf, length, &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
            return -1;
        }
    }
    else if (mode == BUF_CIRC_DOUBLE)
    {
        assert(p->stream && "Invalid PulseAudio Stream!");

        if (pa_simple_write(p, buf, length / 2, &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
            return -1;
        }
        offset = length / 2;

        // Register write callback
        pa_stream_set_write_callback(p->stream, buf_circ_write_cb, NULL);
    }

    return 0;
}

stream_sample_t *outputStream_getIdleBuffer(const streamId id)
{
    (void) id;

    if (buf_mode == BUF_CIRC_DOUBLE)
    {
        // Return half of the buffer not currently being read
        return (!first_half_active) ? buf : buf + buf_len / 2;
    }
    else
        return NULL;
}

bool outputStream_sync(const streamId id, const bool bufChanged)
{
    (void) id;
    (void) bufChanged;

    /* Make sure that every single sample was played */
    if (pa_simple_drain(p, &error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
        return false;
    }

    if (buf_mode == BUF_LINEAR)
    {
        running = false;
    }

    return true;
}

void outputStream_stop(const streamId id)
{
    (void) id;

    /* Make sure that every single sample was played */
    if (pa_simple_flush(p, &error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
    }

    running = false;
}

void outputStream_terminate(const streamId id)
{
    (void) id;

    if (p)
        pa_simple_free(p);
}
