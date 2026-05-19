/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "interfaces/audio.h"
#include "interfaces/radio.h"
#include "peripherals/gpio.h"
#include "hwconfig.h"
#include "core/threads.h"
#include "core/state.h"
#include "drivers/tones/toneGenerator_MDx.h"
#include "stm32_pwm.h"
#include "stm32_adc.h"

#if defined(PLATFORM_MDUV3x0) || defined (PLATFORM_DM1701)
#include "drivers/baseband/HR_C6000.h"
#include "Cx000_dac.h"
#endif


#define PATH(x,y) ((x << 4) | y)

static const uint8_t pathCompatibilityMatrix[9][9] =
{
    // MIC-SPK MIC-RTX MIC-MCU RTX-SPK RTX-RTX RTX-MCU MCU-SPK MCU-RTX MCU-MCU
    {    0   ,   0   ,   0   ,   1   ,   0   ,   1   ,   1   ,   0   ,   1   },  // MIC-RTX
    {    0   ,   0   ,   0   ,   0   ,   1   ,   1   ,   0   ,   0   ,   1   },  // MIC-SPK
    {    0   ,   0   ,   0   ,   1   ,   1   ,   0   ,   1   ,   1   ,   0   },  // MIC-MCU
    {    0   ,   1   ,   1   ,   0   ,   0   ,   0   ,   0   ,   1   ,   1   },  // RTX-SPK
    {    1   ,   0   ,   1   ,   0   ,   0   ,   0   ,   1   ,   0   ,   1   },  // RTX-RTX
    {    1   ,   1   ,   0   ,   0   ,   0   ,   0   ,   1   ,   1   ,   0   },  // RTX-MCU
    {    0   ,   1   ,   1   ,   0   ,   1   ,   1   ,   0   ,   0   ,   0   },  // MCU-SPK
    {    0   ,   0   ,   1   ,   1   ,   0   ,   1   ,   0   ,   0   ,   0   },  // MCU-RTX
    {    1   ,   1   ,   0   ,   1   ,   1   ,   0   ,   0   ,   0   ,   0   }   // MCU-MCU
};


static void stm32pwm_startCbk()
{
    toneGen_lockBeep();
    TIM3->CCER |= TIM_CCER_CC3E;
    TIM3->CR1  |= TIM_CR1_CEN;
}

static void stm32pwm_stopCbk()
{
    TIM3->CCER &= ~TIM_CCER_CC3E;
    toneGen_unlockBeep();
}

static const struct PwmChannelCfg stm32pwm_cfg =
{
    &(TIM3->CCR3),
    stm32pwm_startCbk,
    stm32pwm_stopCbk
};

#if defined(PLATFORM_MDUV3x0) || defined (PLATFORM_DM1701)
static void *audio_thread(void *arg)
{
    (void) arg;

    static uint8_t oldVolume = 0xFF;
    unsigned long long now = getTick();

    Cx000dac_init(&C6000);

    while(state.devStatus != SHUTDOWN)
    {
        Cx000dac_task();

        if(state.volume != oldVolume)
        {
            // Apply new volume level, map 0 - 255 range into -31 to 31
            int8_t gain = ((int8_t) (state.volume / 4)) - 32;
            C6000.setDacGain(gain);

            oldVolume = state.volume;
        }

        now += 4;
        sleepUntil(now);
    }

    Cx000dac_terminate();

    return NULL;
}
#endif

const struct audioDevice outputDevices[] =
{
    {NULL,                    NULL,          0, SINK_MCU},
    #if defined(PLATFORM_MDUV3x0) || defined (PLATFORM_DM1701)
    {&Cx000_dac_audio_driver, NULL,          0, SINK_SPK},
    #else
    {&stm32_pwm_audio_driver, &stm32pwm_cfg, 0, SINK_SPK},
    #endif
    {&stm32_pwm_audio_driver, &stm32pwm_cfg, 0, SINK_RTX},
};

const struct audioDevice inputDevices[] =
{
    {NULL,                    0,                 0,              SOURCE_MCU},
    {&stm32_adc_audio_driver, (const void *) 13, STM32_ADC_ADC2, SOURCE_RTX},
    {&stm32_adc_audio_driver, (const void *) 3,  STM32_ADC_ADC2, SOURCE_MIC},
};

void audio_init()
{
    gpio_setMode(AIN_MIC,  ANALOG);
    gpio_setMode(SPK_MUTE, OUTPUT);
    #ifndef PLATFORM_MD9600
    gpio_setMode(AIN_RTX,      ANALOG);
    gpio_setMode(AUDIO_AMP_EN, OUTPUT);
    #ifndef MDx_ENABLE_SWD
    gpio_setMode(MIC_PWR,      OUTPUT);
    #endif
    #endif

    gpio_setMode(BEEP_OUT, INPUT);

    gpio_setPin(SPK_MUTE);          // Speaker muted
    #ifndef PLATFORM_MD9600
    gpio_clearPin(AUDIO_AMP_EN);    // Audio PA off
    #ifndef MDx_ENABLE_SWD
    gpio_clearPin(MIC_PWR);         // Mic preamp. off
    #endif
    #endif

    stm32pwm_init();
    stm32adc_init(STM32_ADC_ADC2);

    #if defined(PLATFORM_MDUV3x0) || defined (PLATFORM_DM1701)
    gpio_setMode(DMR_CLK,  OUTPUT);
    gpio_setMode(DMR_MOSI, OUTPUT);
    gpio_setMode(DMR_MISO, INPUT);
    spi_init((const struct spiDevice *) &c6000_spi);
    C6000.init();

    pthread_attr_t attr;
    pthread_t      thread;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, AUDIO_THREAD_STKSIZE);

    param.sched_priority = THREAD_PRIO_RT;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&thread, &attr, audio_thread, NULL);
    #endif
}

void audio_terminate()
{
    gpio_setPin(SPK_MUTE);          // Speaker muted
    #ifndef PLATFORM_MD9600
    gpio_clearPin(AUDIO_AMP_EN);    // Audio PA off
    #ifndef MDx_ENABLE_SWD
    gpio_clearPin(MIC_PWR);         // Mic preamp. off
    #endif
    #endif

    stm32pwm_terminate();
    stm32adc_terminate();
}

void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
    uint32_t path = PATH(source, sink);

    switch(path)
    {
        case PATH(SOURCE_MIC, SINK_SPK):
        case PATH(SOURCE_MIC, SINK_RTX):
        case PATH(SOURCE_MIC, SINK_MCU):
            #if !defined(PLATFORM_MD9600) && !defined(MDx_ENABLE_SWD)
            gpio_setPin(MIC_PWR);
            #endif
            break;

        case PATH(SOURCE_RTX, SINK_SPK):
            radio_enableAfOutput();
            break;

        // MD-UV380 uses HR_C6000 for MCU->SPK audio output. Switching between
        // incoming FM audio and DAC output is done automatically inside the IC.
        #if !defined(PLATFORM_MDUV3x0) && !defined(PLATFORM_DM1701)
        case PATH(SOURCE_MCU, SINK_SPK):
        #endif
        case PATH(SOURCE_MCU, SINK_RTX):
            gpio_setMode(BEEP_OUT, ALTERNATE | ALTERNATE_FUNC(2));
            break;

        default:
            break;
    }

    if(sink == SINK_SPK)
    {
        // Anti-pop: unmute speaker after 10ms from amp. power on
        #ifndef PLATFORM_MD9600
        gpio_setPin(AUDIO_AMP_EN);
        #endif
        sleepFor(0, 10);
        gpio_clearPin(SPK_MUTE);
    }
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    uint32_t path = PATH(source, sink);

    if(sink == SINK_SPK)
    {
        gpio_setPin(SPK_MUTE);
        #ifndef PLATFORM_MD9600
        gpio_clearPin(AUDIO_AMP_EN);
        #endif
    }

    switch(path)
    {
        case PATH(SOURCE_MIC, SINK_SPK):
        case PATH(SOURCE_MIC, SINK_RTX):
        case PATH(SOURCE_MIC, SINK_MCU):
            #if !defined(PLATFORM_MD9600) && !defined(MDx_ENABLE_SWD)
            gpio_clearPin(MIC_PWR);
            #endif
            break;

        case PATH(SOURCE_RTX, SINK_SPK):
            radio_disableAfOutput();
            break;

        #if !defined(PLATFORM_MDUV3x0) && !defined(PLATFORM_DM1701)
        case PATH(SOURCE_MCU, SINK_SPK):
        #endif
        case PATH(SOURCE_MCU, SINK_RTX):
            gpio_setMode(BEEP_OUT, INPUT);  // Set output to Hi-Z
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
    uint8_t p1Index = (p1Source * 3) + p1Sink;
    uint8_t p2Index = (p2Source * 3) + p2Sink;

    return pathCompatibilityMatrix[p1Index][p2Index] == 1;
}
