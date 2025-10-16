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
#include "codec2.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/dsp.h"

static const size_t audioBufSize = 320;
static const size_t dataBufSize = 2 * 1024;

void error()
{
    while (1) {
        platform_ledOn(RED);
        sleepFor(0u, 500u);
        platform_ledOff(RED);
        sleepFor(0u, 500u);
    }
}

void *mic_task(void *arg)
{
    (void)arg;

    struct CODEC2 *codec2 = codec2_create(CODEC2_MODE_3200);
    int16_t *audioBuf = ((int16_t *)malloc(audioBufSize * sizeof(int16_t)));
    if (audioBuf == NULL)
        error();
    uint8_t *dataBuf = ((uint8_t *)malloc(dataBufSize * sizeof(uint8_t)));
    memset(dataBuf, 0x00, dataBufSize);

    sleepFor(0u, 500u);

    pathId path = audioPath_request(SOURCE_MIC, SINK_MCU, PRIO_TX);
    streamId id = audioStream_start(path, audioBuf, audioBufSize, 8000,
                                    BUF_CIRC_DOUBLE | STREAM_INPUT);

    platform_ledOn(GREEN);

    filter_state_t dcr;
    size_t pos = 0;

    dsp_resetFilterState(&dcr);

    while (pos < dataBufSize) {
        dataBlock_t data = inputStream_getData(id);
        if (data.data == NULL)
            error();

        // Pre-amplification stage
        for (size_t i = 0; i < data.len; i++)
            data.data[i] <<= 3;

        // DC removal
        dsp_dcRemoval(&dcr, data.data, data.len);

        // Post-amplification stage
        for (size_t i = 0; i < data.len; i++)
            data.data[i] *= 20;

        codec2_encode(codec2, &dataBuf[pos], data.data);
        pos += 8;
    }

    platform_ledOff(GREEN);
    sleepFor(10u, 0u);
    platform_ledOn(RED);
    for (size_t i = 0; i < dataBufSize; i++) {
        iprintf("%02x ", dataBuf[i]);
    }
    platform_ledOff(RED);

    while (1)
        ;

    return 0;
}

int main()
{
    platform_init();

    // Create mic input thread
    pthread_t mic_thread;
    pthread_attr_t mic_attr;

    pthread_attr_init(&mic_attr);
    pthread_attr_setstacksize(&mic_attr, 20 * 1024);
    pthread_create(&mic_thread, &mic_attr, mic_task, NULL);

    while (1)
        ;

    return 0;
}
