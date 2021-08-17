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
                            stream_sample_t * const buf,
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

    /*
     * Convert buffer elements from int16_t to unsigned 8 bit values, as required
     * by MDx tone generator. Processing can be done in-place because the API
     * mandates that the function caller does not modify the buffer content once
     * this function has been called. Code below exploits Cortex M4 SIMD
     * instructions for fast execution.
     */

    uint32_t *data = ((uint32_t *) buf);
    for(size_t i = 0; i < length/2; i++)
    {
        uint32_t value = __SADD16(data[i], 0x80008000);
        data[i]        = (value >> 8) & 0x00FF00FF;
    }

    /* Handle last element in case of odd buffer length */
    if((length % 2) != 0)
    {
        int16_t value   = buf[length - 1] + 32768;
        buf[length - 1] = ((uint16_t) value) >> 8;
    }

    /* Start the stream, nonblocking function */
    toneGen_playAudioStream(((uint16_t *) buf), length, sampleRate);
    return 0;
}

void outputStream_stop(streamId id)
{
    (void) id;

    if(!toneGen_toneBusy()) return;
    toneGen_stopAudioStream();
}
