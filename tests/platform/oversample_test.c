/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/audio_stream.h"
#include "core/audio_path.h"
#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "core/memory_profiling.h"
#include "interfaces/audio.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/dsp.h"

static const size_t audioBufSize = 320;
static const size_t outBufSize = 45 * 1024;
// Value in range [1, 16] any larger will overflow the uint16_t and bit shifting will become necessary
static const size_t oversample = 4;
// Is essentially sqrt(oversample)/2
static const size_t oversample_bits = 0;

void error()
{
    while (1) {
        platform_ledOn(RED);
        sleepFor(0u, 500u);
        platform_ledOff(RED);
        sleepFor(0u, 500u);
    }
}

void blink_green(size_t count)
{
    for (size_t i = 0; count == 0 || i < count; i++)
    {
        platform_ledOn(GREEN);
        sleepFor(0u, 500u);
        platform_ledOff(GREEN);
        sleepFor(0u, 500u);
    }
}

int main()
{
    platform_init();

    int16_t *audioBuf = ((int16_t *)malloc(audioBufSize * sizeof(int16_t)));
    if (audioBuf == NULL)
        error();
    uint16_t *outBuf = ((uint16_t *)malloc(outBufSize * sizeof(uint16_t)));
    if (outBuf == NULL)
        error();

    // Requesting the audio path enables the preamp and some other stuff, this needs a few seconds to settle before ready
    pathId path = audioPath_request(SOURCE_MIC, SINK_MCU, PRIO_TX);

    blink_green(3);
    platform_ledOn(RED);

    streamId id = audioStream_start(path, audioBuf, audioBufSize, 8000 * oversample,
                                    BUF_CIRC_DOUBLE | STREAM_INPUT);

    size_t outPos = 0;
    size_t subPos = 0;
    uint32_t sum = 0;

    while (true) {
        dataBlock_t data = inputStream_getData(id);

        if (data.data == NULL)
            error();

        for (size_t i = 0; i < data.len; i++)
        {
            sum += (uint16_t)data.data[i];
            subPos++;
            if (subPos >= oversample)
            {
                subPos = 0;
                outBuf[outPos++] = sum >> oversample_bits;
                sum = 0;
                if (outPos >= outBufSize)
                    goto done;
            }
        }
    }
done:

    audioStream_stop(id);

    platform_ledOff(RED);
    blink_green(10);
    platform_ledOn(RED);

    for (size_t i = 0; i < outBufSize; i++)
        iprintf("%04x\n", outBuf[i]);

    platform_ledOff(RED);

    blink_green(0);
}
