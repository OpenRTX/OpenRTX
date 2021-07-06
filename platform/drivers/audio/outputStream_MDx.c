/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#include <interfaces/audio_stream.h>
#include <toneGenerator_MDx.h>
#include <interfaces/gpio.h>
#include <hwconfig.h>
#include <stdbool.h>

/*
 * MDx radios have only one output stream device, via PWM-based tone generator,
 * connected to both the speaker and HR_Cx000 audio input.
 */
int priority = PRIO_BEEP;

streamId outputStream_start(const enum AudioSink destination,
                            const enum AudioPriority prio,
                            const stream_sample_t* const buf,
                            const size_t length,
                            const uint32_t sampleRate)
{
    if(destination == SINK_MCU) return -1;              /* This device cannot sink to buffer             */
    if(toneGen_toneBusy())                              /* Check if a stream is already running          */
    {
        if(prio < priority) return -1;                  /* Requested priority is lower than current      */
        if(prio > priority) toneGen_stopAudioStream();  /* Stop an ongoing stream with lower priority    */
        if(!toneGen_waitForStreamEnd()) return -1;      /* Same priority, wait for current stream to end */
    }

    /* This assigment must be thread-safe */
    __disable_irq();
    priority = prio;
    __enable_irq();

    toneGen_playAudioStream(buf, length, sampleRate);   /* Start the stream, nonblocking function        */
    return 0;
}

void outputStream_stop(streamId id)
{
    (void) id;

    if(!toneGen_toneBusy()) return;
    toneGen_stopAudioStream();
}
