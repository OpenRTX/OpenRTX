/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
 *                          Niccolò Izzo IU2KIN                            *
 *                          Frederik Saraci IU2NRO                         *
 *                          Silvano Seva IU2KWO                            *
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

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/audio.h>
#include <interfaces/radio.h>
#include <peripherals/gpio.h>
#include <hwconfig.h>
#include <threads.h>
#include <state.h>
#include <HR_C6000.h>
#include <spi_stm32.h>
#include "stm32_dac.h"
#include "stm32_adc.h"
#include "Cx000_dac.h"

#define PATH(x,y) ((x << 4) | y)

const struct audioDevice outputDevices[] =
{
    {NULL,                    NULL, 0,             SINK_MCU},
    {&stm32_dac_audio_driver, NULL, STM32_DAC_CH2, SINK_RTX},
    {&Cx000_dac_audio_driver, NULL, 0,             SINK_SPK},
};

const struct audioDevice inputDevices[] =
{
    {NULL,                    0,                          0,              SOURCE_MCU},
    {&stm32_adc_audio_driver, (const void *) ADC_RTX_CH,  STM32_ADC_ADC2, SOURCE_RTX},
    {&stm32_adc_audio_driver, (const void *) ADC_MIC_CH,  STM32_ADC_ADC2, SOURCE_MIC},
};

static void *audio_thread(void *arg)
{
    (void) arg;

    unsigned long long now = getTick();

    Cx000dac_init(&C6000);

    while(state.devStatus != SHUTDOWN)
    {
        Cx000dac_task();

        now += 4;
        sleepUntil(now);
    }

    Cx000dac_terminate();

    return NULL;
}

void audio_init()
{
    gpio_setMode(BEEP_OUT, ANALOG);
    gpio_setMode(AIN_MIC,  ANALOG);
    gpio_setMode(AIN_RTX,  ANALOG);
    gpio_setMode(C6K_CLK,  ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(C6K_MOSI, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(C6K_MISO, ALTERNATE | ALTERNATE_FUNC(5));

    stm32dac_init(STM32_DAC_CH2, 2048);
    stm32adc_init(STM32_ADC_ADC2);

    gpioDev_clear(C6K_SLEEP);
    delayMs(10);
    spiStm32_init(&c6000_spi, 11000000, SPI_FLAG_CPHA);
    C6000.init();

    pthread_attr_t attr;
    pthread_t      thread;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, AUDIO_THREAD_STKSIZE);

    param.sched_priority = THREAD_PRIO_RT;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&thread, &attr, audio_thread, NULL);
}

void audio_terminate()
{
    gpioDev_clear(AUDIO_AMP_EN);
    gpioDev_clear(MIC_PWR_EN);
    gpioDev_set(C6K_SLEEP);

    stm32dac_terminate();
    stm32adc_terminate();
    C6000.terminate();
}


void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
    uint32_t path = PATH(source, sink);

    switch(path)
    {
        case PATH(SOURCE_MIC, SINK_SPK):
        case PATH(SOURCE_MIC, SINK_RTX):
        case PATH(SOURCE_MIC, SINK_MCU):
            gpioDev_set(MIC_PWR_EN);
            gpioDev_set(INT_MIC_SEL);
            break;

        case PATH(SOURCE_RTX, SINK_SPK):
            radio_enableAfOutput();
            // Fallthrough

        case PATH(SOURCE_MCU, SINK_SPK):
            gpioDev_set(AF_MUTE);
            break;

        default:
            break;
    }

    if(sink == SINK_SPK)
    {
        // Anti-pop: unmute speaker after 10ms from amp. power on
        gpioDev_set(AUDIO_AMP_EN);
        sleepFor(0, 10);
        gpioDev_clear(INT_SPK_MUTE);
    }
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    uint32_t path = PATH(source, sink);

    if(sink == SINK_SPK)
    {
        gpioDev_set(INT_SPK_MUTE);
        gpioDev_clear(AUDIO_AMP_EN);
    }

    switch(path)
    {
        case PATH(SOURCE_MIC, SINK_SPK):
        case PATH(SOURCE_MIC, SINK_RTX):
        case PATH(SOURCE_MIC, SINK_MCU):
            gpioDev_clear(MIC_PWR_EN);
            gpioDev_clear(INT_MIC_SEL);
            break;

        case PATH(SOURCE_RTX, SINK_SPK):
            radio_disableAfOutput();
            // Fallthrough

        case PATH(SOURCE_MCU, SINK_SPK):
            gpioDev_clear(AF_MUTE);
            break;

        default:
            break;
    }
}

bool audio_checkPathCompatibility(const enum AudioSource p1Source,
                                  const enum AudioSink   p1Sink,
                                  const enum AudioSource p2Source,
                                  const enum AudioSink   p2Sink)

{
    if(p1Source == p2Source)
        return false;

    if(p1Sink == p2Sink)
        return false;

    return true;
}
