/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
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

// Test private methods
#define private public

#include <interfaces/audio_stream.h>
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
