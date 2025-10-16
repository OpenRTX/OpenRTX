/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/audio.h"
#include "peripherals/gpio.h"
#include "hwconfig.h"
#include "MAX9814.h"
#include "stm32_dac.h"
#include "stm32_adc.h"

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
    {NULL,                    0, 0,             SINK_MCU},
    {&stm32_dac_audio_driver, 0, STM32_DAC_CH1, SINK_RTX},
    {&stm32_dac_audio_driver, 0, STM32_DAC_CH2, SINK_SPK},
};

const struct audioDevice inputDevices[] =
{
    {NULL,                    0,                0,              SOURCE_MCU},
    {&stm32_adc_audio_driver, (const void *) 1, STM32_ADC_ADC2, SOURCE_RTX},
    {&stm32_adc_audio_driver, (const void *) 2, STM32_ADC_ADC2, SOURCE_MIC},
};

void audio_init()
{
    gpio_setMode(SPK_MUTE,    OUTPUT);
    gpio_setMode(MIC_MUTE,    OUTPUT);
    gpio_setMode(AUDIO_MIC,   ANALOG);
    gpio_setMode(AUDIO_SPK,   ANALOG);
    gpio_setMode(BASEBAND_RX, ANALOG);
    gpio_setMode(BASEBAND_TX, ANALOG);

    gpio_setPin(SPK_MUTE);      // Off  = logic high
    gpio_clearPin(MIC_MUTE);    // Off  = logic low
    max9814_setGain(0);         // 40 dB gain

    stm32dac_init(STM32_DAC_CH1, 2048);
    stm32dac_init(STM32_DAC_CH2, 2048);
    stm32adc_init(STM32_ADC_ADC2);
}

void audio_terminate()
{
    gpio_setPin(SPK_MUTE);
    gpio_clearPin(MIC_MUTE);

    stm32dac_terminate();
    stm32adc_terminate();
}

void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
    if(source == SOURCE_MIC)
        gpio_setPin(MIC_MUTE);

    if(sink == SINK_SPK)
        gpio_clearPin(SPK_MUTE);
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    if(source == SOURCE_MIC)
        gpio_clearPin(MIC_MUTE);

    if(sink == SINK_SPK)
        gpio_setPin(SPK_MUTE);
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
