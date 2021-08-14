/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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
#include <memory_profiling.h>

#include <cstring>
#include <subprojects/codec2/src/codec2.h>

static const size_t numSamples = 320;

using audio_sample_t = int16_t;
using stream_t       = std::array<stream_sample_t, numSamples/2>;
using audio_t        = std::array<audio_sample_t,  numSamples/2>;
using data_t         = std::array<uint8_t, 8192>;

struct CODEC2* codec2;
uint8_t codec2_frame[8] = {0};

/*
 * Converts 12-bit unsigned values packed into uint16_t into int16_t samples,
 * perform in-place conversion to save space.
 */
void adc_to_audio_stm32(std::array<audio_sample_t, numSamples/2> *audio)
{
    for (int i = 0; i < audio->size(); i++)
    {
        audio->at(i) <<= 3;
    }
}

void *mic_task(void *arg)
{
    codec2 = codec2_create(CODEC2_MODE_3200);
    stream_sample_t *sampleBuf = ((stream_sample_t *) malloc(numSamples * sizeof(uint16_t)));
    data_t *data = new data_t;

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

    size_t pos = 0;

    while(pos < data->size())
    {
        gpio_setPin(RED_LED);
        stream_t *stream = inputStream_getData< numSamples/2 >(input_id);
        if(stream == nullptr) break;
        gpio_clearPin(RED_LED);

        audio_t *audio = reinterpret_cast< audio_t* >(stream);
        adc_to_audio_stm32(audio);
        dsp_dcRemoval(audio->data(), audio->size());

        uint8_t codec2_frame[8] = {0};
        codec2_encode(codec2, codec2_frame, &(audio->data()[0]));

        memcpy((data->data() + pos), codec2_frame, 8);
        pos += 8;
    }

    gpio_clearPin(GREEN_LED);
    inputStream_stop(input_id);
    delayMs(10000);

    for(size_t i = 0; i < data->size(); i++)
    {
        iprintf("%x ", data->at(i));
    }

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
