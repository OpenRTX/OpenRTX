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
#include <interfaces/gpio.h>
#include <interfaces/audio.h>
#include <interfaces/audio_path.h>
#include <interfaces/audio_stream.h>
#include <hwconfig.h>
#include <dsp.h>

/*********************************************
 * EFFECTIVE MEMORY OCCUPATION IS TWICE THIS *
 * NUMBER, AS SAMPLES OCCUPY TWO BYTES EACH  *
 *********************************************/
static const size_t numSamples = 45*1024;


using audio_sample_t = int16_t;
typedef uint16_t stream_sample_t;

static const char hexdigits[]="0123456789abcdef";
void printSignedInt(audio_sample_t x)
{
    char result[]="....\r";
    for(int i=3;i>=0;i--)
    {
        result[i]=hexdigits[x & 0xf];
        x>>=4;
    }
    puts(result);
}

/*
 * Converts 12-bit unsigned values packed into uint16_t into int16_t samples,
 * perform in-place conversion to save space.
 */
void adc_to_audio_stm32(std::array<audio_sample_t, numSamples/2> *audio)
{
    for (int i = 0; i < numSamples/2; i++)
    {
        (*audio)[i] = (*audio)[i] << 3;
    }
}

void *mic_task(void *arg)
{
    uint16_t *sampleBuf = ((uint16_t *) malloc(numSamples * sizeof(uint16_t)));

    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED,   OUTPUT);
    gpio_setMode(MIC_PWR,   OUTPUT);
    gpio_setPin(MIC_PWR);
    gpio_setPin(GREEN_LED);

    // Initialize audio stream
    streamId input_id = inputStream_start(SOURCE_MIC,
                                          PRIO_TX,
                                          sampleBuf,
                                          numSamples,
                                          BUF_CIRC_DOUBLE,
                                          8000);

    delayMs(3000);

    // Record
    gpio_clearPin(GREEN_LED);
    gpio_setPin(RED_LED);

    // Get audio chunk from the microphone stream
    std::array<stream_sample_t, numSamples/2> *stream =
        inputStream_getData<numSamples/2>(input_id);

    std::array<audio_sample_t, numSamples/2> *audio =
        reinterpret_cast<std::array<audio_sample_t, numSamples/2>*>(stream);

    // Convert 12-bit unsigned values into 16-bit signed
    adc_to_audio_stm32(audio);

    // Apply DC removal filter
    dsp_dcRemoval(audio->data(), audio->size());

    delayMs(10000);

    for(size_t i = 0; i < audio->size(); i++)
    {
        printSignedInt((*audio)[i]);
    }

    // Terminate microphone sampling
    inputStream_stop(input_id);

    return 0;
}

int main()
{
//     platform_init();

    // Create mic input thread
    pthread_t      mic_thread;
    pthread_attr_t mic_attr;

    pthread_attr_init(&mic_attr);
    pthread_attr_setstacksize(&mic_attr, 16 * 1024);
    pthread_create(&mic_thread, &mic_attr, mic_task, NULL);

    while(1) ;

    return 0;
}
