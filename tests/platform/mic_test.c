/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interfaces/delays.h"
#include "interfaces/audio.h"
#include "core/audio_path.h"
#include "core/audio_stream.h"
#include "interfaces/platform.h"
#include "core/dsp.h"

int main()
{
    platform_init();

    struct dcBlock dcb;
    dsp_resetState(dcb);

    static const size_t numSamples = 45 * 1024; // 90kB
    void *sampleBuf = malloc(numSamples * sizeof(stream_sample_t));

    pathId pi = audioPath_request(SOURCE_MIC, SINK_MCU, PRIO_TX);
    streamId id = audioStream_start(pi, (stream_sample_t *)sampleBuf,
                                    numSamples, 8000,
                                    BUF_LINEAR | STREAM_INPUT);

    sleepFor(3u, 0u);
    platform_ledOn(GREEN);

    dataBlock_t audio = inputStream_getData(id);

    platform_ledOff(GREEN);
    platform_ledOn(RED);
    sleepFor(10u, 0u);

    for (size_t i = 0; i < audio.len; i++) {
        int16_t sample = dsp_dcBlockFilter(&dcb, audio.data[i]);
        audio.data[i] = sample * 32;
    }

    uint16_t *ptr = ((uint16_t *)audio.data);
    for (size_t i = 0; i < audio.len; i++)
        iprintf("%04x ", ptr[i]);

    while (1)
        ;

    return 0;
}
