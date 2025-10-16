/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Test private methods
#define private public

#include "core/audio_stream.h"
#include <math.h>
#include <stdio.h>

#define BUF_LEN 4096 * 10
#define AMPLITUDE 20000
#define PI 3.14159
#define SAMPLE_RATE 48000
#define FREQUENCY 440

/**
 * Test sine playback
 */

int16_t buffer[BUF_LEN] = { 0 };

void fill_buffer_sine(int16_t *buffer, size_t len)
{
    for(size_t i = 0; i < len; i++)
    {
        buffer[i] = AMPLITUDE * sin(2 * PI * i *
                    ((float) FREQUENCY / SAMPLE_RATE));
    }
}

void play_sine_linear()
{
    int id = 0;

    // Create 440 Hz sine wave
    fill_buffer_sine(buffer, BUF_LEN);

    // Play back sine wave
    for(int i = 0; i < 10; i++)
    {
        id = outputStream_start(SINK_SPK,
                                PRIO_PROMPT,
                                buffer,
                                BUF_LEN,
                                BUF_LINEAR,
                                SAMPLE_RATE);
    };
    outputStream_sync(id, false);

    // Sync, flush, terminate
    outputStream_stop(id);
    outputStream_terminate(id);
}

void play_sine_circular()
{
    int id = 0;
    stream_sample_t *buf = NULL;

    // Fill first half of buffer with sine
    fill_buffer_sine(buffer, BUF_LEN / 2);

    // Play back sine wave
    id = outputStream_start(SINK_SPK,
                            PRIO_PROMPT,
                            buffer,
                            BUF_LEN,
                            BUF_CIRC_DOUBLE,
                            SAMPLE_RATE);

    for(int i = 0; i < 10; i++)
    {
        buf = outputStream_getIdleBuffer(id);
        fill_buffer_sine(buf, BUF_LEN / 2);
        outputStream_sync(id, true);
    }

    // Sync, flush, terminate
    outputStream_stop(id);
    outputStream_terminate(id);
}

int main()
{
    //play_sine_linear();
    play_sine_circular();
}
