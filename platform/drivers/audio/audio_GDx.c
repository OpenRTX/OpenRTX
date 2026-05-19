/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/audio.h"
#include "peripherals/gpio.h"
#include "interfaces/radio.h"
#include "hwconfig.h"

#define PATH(x,y) ((x << 4) | y)

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

const struct audioDevice outputDevices[] =
{
    {NULL, 0, 0, SINK_MCU},
    {NULL, 0, 0, SINK_RTX},
    {NULL, 0, 0, SINK_SPK},
};

const struct audioDevice inputDevices[] =
{
    {NULL, 0, 0, SINK_MCU},
    {NULL, 0, 0, SINK_RTX},
    {NULL, 0, 0, SINK_SPK},
};

void audio_init()
{
    gpio_setMode(AUDIO_AMP_EN, OUTPUT);
    gpio_clearPin(AUDIO_AMP_EN);
}

void audio_terminate()
{
    gpio_clearPin(AUDIO_AMP_EN);
}

void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
    if(PATH(source, sink) == PATH(SOURCE_RTX, SINK_SPK))
    {
        radio_enableAfOutput();
        gpio_setPin(AUDIO_AMP_EN);
    }
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    if(PATH(source, sink) == PATH(SOURCE_RTX, SINK_SPK))
    {
        gpio_clearPin(AUDIO_AMP_EN);
        radio_disableAfOutput();
    }
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
