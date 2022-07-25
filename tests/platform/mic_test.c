/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO                             *
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
#include <interfaces/delays.h>
#include <interfaces/audio.h>
#include <interfaces/audio_path.h>
#include <interfaces/audio_stream.h>
#include <interfaces/platform.h>
#include <dsp.h>

int main()
{
    platform_init();

    filter_state_t dcrState;
    dsp_resetFilterState(&dcrState);

    static const size_t numSamples = 45*1024;       // 90kB
    stream_sample_t *sampleBuf = ((stream_sample_t *) malloc(numSamples *
                                                      sizeof(stream_sample_t)));

    audio_enableMic();
    streamId id = inputStream_start(SOURCE_MIC, PRIO_TX, sampleBuf, numSamples,
                                    BUF_LINEAR, 8000);

    sleepFor(3u, 0u);
    platform_ledOn(GREEN);

    dataBlock_t audio = inputStream_getData(id);

    platform_ledOff(GREEN);
    platform_ledOn(RED);
    sleepFor(10u, 0u);

    // Pre-processing gain
    for(size_t i = 0; i < audio.len; i++) audio.data[i] <<= 3;

    // DC removal
    dsp_dcRemoval(&dcrState, audio.data, audio.len);

    // Post-processing gain
    for(size_t i = 0; i < audio.len; i++) audio.data[i] *= 10;


    uint16_t *ptr = ((uint16_t *) audio.data);
    for(size_t i = 0; i < audio.len; i++)
    {
        iprintf("%04x ", ptr[i]);
    }

    while(1) ;

    return 0;
}
