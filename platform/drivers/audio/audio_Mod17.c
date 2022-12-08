/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#include <interfaces/audio.h>
#include <interfaces/gpio.h>
#include <hwconfig.h>
#include "MAX9814.h"


static const uint8_t pathCompatibilityMatrix[9][9] =
{
    // MIC-SPK MIC-RTX MIC-MCU RTX-SPK RTX-RTX RTX-MCU MCU-SPK MCU-RTX MCU-MCU
    {    0   ,   0   ,   0   ,   1   ,   0   ,   1   ,   1   ,   0   ,   1   },  // MIC-RTX
    {    0   ,   0   ,   0   ,   0   ,   1   ,   1   ,   0   ,   1   ,   1   },  // MIC-SPK
    {    0   ,   0   ,   0   ,   1   ,   1   ,   0   ,   1   ,   1   ,   0   },  // MIC-MCU
    {    0   ,   1   ,   1   ,   0   ,   0   ,   0   ,   0   ,   1   ,   1   },  // RTX-SPK
    {    1   ,   0   ,   1   ,   0   ,   0   ,   0   ,   1   ,   0   ,   1   },  // RTX-RTX
    {    1   ,   1   ,   0   ,   0   ,   0   ,   0   ,   1   ,   1   ,   0   },  // RTX-MCU
    {    0   ,   1   ,   1   ,   0   ,   1   ,   1   ,   0   ,   0   ,   0   },  // MCU-SPK
    {    1   ,   0   ,   1   ,   1   ,   0   ,   1   ,   0   ,   0   ,   0   },  // MCU-RTX
    {    1   ,   1   ,   0   ,   1   ,   1   ,   0   ,   0   ,   0   ,   0   }   // MCU-MCU
};


void audio_init()
{
    gpio_setMode(SPK_MUTE, OUTPUT);
    gpio_setMode(MIC_MUTE, OUTPUT);

    gpio_setPin(SPK_MUTE);      // Off  = logic high
    gpio_clearPin(MIC_MUTE);    // Off  = logic low
    max9814_setGain(0);         // 40 dB gain
}

void audio_terminate()
{
    gpio_setPin(SPK_MUTE);
    gpio_clearPin(MIC_MUTE);
}

void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
    if(source == SOURCE_MIC) gpio_setPin(MIC_MUTE);
    if(sink == SINK_SPK)     gpio_clearPin(SPK_MUTE);
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    if(source == SOURCE_MIC) gpio_clearPin(MIC_MUTE);
    if(sink == SINK_SPK)     gpio_setPin(SPK_MUTE);
}

bool audio_checkPathCompatibility(const enum AudioSource p1Source,
                                  const enum AudioSink   p1Sink,
                                  const enum AudioSource p2Source,
                                  const enum AudioSink   p2Sink)

{
    uint8_t p1Index = (p1Source * 3) + p1Sink;
    uint8_t p2Index = (p2Source * 3) + p2Sink;

    return pathCompatibilityMatrix[p1Index][p2Index] == 1;
}
