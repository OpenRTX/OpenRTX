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
void printUnsignedInt(uint16_t x)
{
    char result[]="....\r";
    for(int i=3;i>=0;i--)
    {
        result[i]=hexdigits[x & 0xf];
        x>>=4;
    }
    puts(result);
}

int main()
{
    uint16_t *sampleBuf = ((uint16_t *) malloc(numSamples * sizeof(uint16_t)));

    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED,   OUTPUT);
    gpio_setMode(MIC_PWR,   OUTPUT);
    gpio_setPin(MIC_PWR);

    delayMs(1000);

    // Initialize audio stream
    streamId input_id = inputStream_start(SOURCE_MIC,
                                          PRIO_TX,
                                          sampleBuf,
                                          numSamples,
                                          BUF_CIRC,
                                          8000);

    // Record
    gpio_setPin(GREEN_LED);

    // Get audio chunk from the microphone stream
    std::array<stream_sample_t, numSamples> *stream =
        inputStream_getData<numSamples>(input_id);

    // stop stream here, otherwise samples will be overwritten while we're
    // trying to process them
    inputStream_stop(input_id);

    // Amplify signal by multiplying by 8
    for(size_t i = 0; i < stream->size(); i++) stream->at(i) <<= 3;

    std::array<audio_sample_t, numSamples> *audio =
        reinterpret_cast<std::array<audio_sample_t, numSamples>*>(stream);

    // Apply DC removal filter
    dsp_dcRemoval(audio->data(), audio->size());

    gpio_clearPin(GREEN_LED);
    delayMs(10000);
    gpio_setPin(RED_LED);

    for(size_t i = 0; i < stream->size(); i++)
    {
        printUnsignedInt(stream->at(i));
    }

    while(1)
    {
        gpio_clearPin(RED_LED);
        delayMs(500);
        gpio_setPin(RED_LED);
        delayMs(500);
    }

    return 0;
}
