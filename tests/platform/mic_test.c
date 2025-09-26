/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <interfaces/delays.h>
#include <interfaces/audio.h>
#include <audio_path.h>
#include <audio_stream.h>
#include <interfaces/platform.h>
#include <dsp.h>

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
