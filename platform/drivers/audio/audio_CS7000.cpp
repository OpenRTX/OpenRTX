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
#include "drivers/baseband/HR_C6000.h"
#include "drivers/SPI/spi_stm32.h"
#include "stm32_dac.h"
#include "stm32_adc.h"
#include "Cx000_dac.h"
#include "C6000_symbols.h"
#include "C6000_fmcopy.h"
#include "C6000_modem.h"

#define PATH(x,y) ((x << 4) | y)

// Analog baseband paths, when M17 does not go through the HR_C6000's modem
static const struct C6000modemPath rtxOutAnalog =
{
    &stm32_dac_audio_driver, NULL
};

static const struct C6000modemPath rtxInAnalog =
{
    &stm32_adc_audio_driver, (const void *) ADC_RTX_CH
};

const struct audioDevice outputDevices[] =
{
    {NULL,                          NULL,          0,             SINK_MCU},
    {&C6000_modem_out_audio_driver, &rtxOutAnalog, STM32_DAC_CH2, SINK_RTX},
    {&Cx000_dac_audio_driver,       NULL,          0,             SINK_SPK},
};

const struct audioDevice inputDevices[] =
{
    {NULL,                         0,                          0,              SOURCE_MCU},
    {&C6000_modem_in_audio_driver, &rtxInAnalog,               STM32_ADC_ADC2, SOURCE_RTX},
    {&stm32_adc_audio_driver,      (const void *) ADC_MIC_CH,  STM32_ADC_ADC2, SOURCE_MIC},
};

HR_C6000 C6000((const struct spiDevice *) &c6000_spi, { C6K_CS });
static bool spkEnabled = false;

static inline void selectSpk()
{
    if(spkEnabled == false)
    {
        gpioDev_set(INT_SPK_MUTE);
        gpioDev_set(EXT_SPK_MUTE);
        return;
    }

    // Phone detect pin is active low
    if(gpio_readPin(PHONE_DETECT) == 0)
        gpioDev_clear(EXT_SPK_MUTE);
    else
        gpioDev_clear(INT_SPK_MUTE);
}

// HR_C6000 timeslot interrupt (DMR_TS_INT, PC0), falling edge
void __attribute__((used)) EXTI0_IRQHandler()
{
    #ifdef STM32H743xx
    EXTI_D1->PR1 = EXTI_PR1_PR0;
    #else
    EXTI->PR = EXTI_PR_PR0;
    #endif
    C6000symbols_timeslotIrq();
}

/*
 * HR_C6000 system interrupt (DMR_SYS_INT), falling edge: PC3 on the
 * CS7000-PLUS, as in its schematic, PC1 on the CS7000
 */
#ifdef STM32H743xx
void __attribute__((used)) EXTI3_IRQHandler()
{
    EXTI_D1->PR1 = EXTI_PR1_PR3;
    C6000fmcopy_systemIrq();
}
#else
void __attribute__((used)) EXTI1_IRQHandler()
{
    EXTI->PR = EXTI_PR_PR1;
    C6000fmcopy_systemIrq();
}
#endif

/**
 * Route the HR_C6000 timeslot and system interrupts to their EXTI lines,
 * falling edge.
 */
static void chipModemInit()
{
    C6000symbols_init(&C6000);
    C6000fmcopy_init(&C6000);

    gpio_setMode(DMR_TS_INT,  INPUT);
    gpio_setMode(DMR_SYS_INT, INPUT);

    #ifdef STM32H743xx
    RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
    __DSB();
    SYSCFG->PMCR &= ~SYSCFG_PMCR_PC3SO;     // PC3_C pad to the PC3 input
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0]
                      & ~(SYSCFG_EXTICR1_EXTI0 | SYSCFG_EXTICR1_EXTI3))
                      | SYSCFG_EXTICR1_EXTI0_PC | SYSCFG_EXTICR1_EXTI3_PC;
    EXTI->FTSR1   |= EXTI_FTSR1_TR0 | EXTI_FTSR1_TR3;
    EXTI_D1->PR1   = EXTI_PR1_PR0 | EXTI_PR1_PR3;
    EXTI_D1->IMR1 |= EXTI_IMR1_IM0 | EXTI_IMR1_IM3;
    NVIC_SetPriority(EXTI3_IRQn, 10);
    NVIC_EnableIRQ(EXTI3_IRQn);
    #else
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    __DSB();
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0]
                      & ~(SYSCFG_EXTICR1_EXTI0 | SYSCFG_EXTICR1_EXTI1))
                      | SYSCFG_EXTICR1_EXTI0_PC | SYSCFG_EXTICR1_EXTI1_PC;
    EXTI->FTSR |= EXTI_FTSR_TR0 | EXTI_FTSR_TR1;
    EXTI->PR    = EXTI_PR_PR0 | EXTI_PR_PR1;
    EXTI->IMR  |= EXTI_IMR_MR0 | EXTI_IMR_MR1;
    NVIC_SetPriority(EXTI1_IRQn, 10);
    NVIC_EnableIRQ(EXTI1_IRQn);
    #endif

    NVIC_SetPriority(EXTI0_IRQn, 10);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

static void *audio_thread(void *arg)
{
    (void) arg;

    unsigned long long now = getTick();

    Cx000dac_init(&C6000);

    while(state.devStatus != SHUTDOWN)
    {
        selectSpk();
        Cx000dac_task();
        C6000symbols_task();

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
    gpio_setMode(PHONE_DETECT, INPUT_PULL_UP);

    stm32dac_init(STM32_DAC_CH2, 2048);
    stm32adc_init(STM32_ADC_ADC2);

    gpioDev_set(INT_SPK_MUTE);
    gpioDev_set(EXT_SPK_MUTE);
    gpioDev_clear(INT_MIC_SEL);
    gpioDev_clear(EXT_MIC_SEL);

    gpioDev_set(RX_PWR_EN);
    gpioDev_clear(C6K_SLEEP);
    delayMs(10);
    spiStm32_init(&c6000_spi, 12500000, SPI_FLAG_CPHA);
    C6000.init();
    C6000.fmMode();
    chipModemInit();

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
            // Phone detect pin is active low
            if(gpio_readPin(PHONE_DETECT) == 0)
                gpioDev_set(EXT_MIC_SEL);
            else
                gpioDev_set(INT_MIC_SEL);

            gpioDev_set(MIC_PWR_EN);
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
        spkEnabled = true;
    }
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    uint32_t path = PATH(source, sink);

    if(sink == SINK_SPK)
    {
        gpioDev_clear(AUDIO_AMP_EN);
        spkEnabled = false;
    }

    switch(path)
    {
        case PATH(SOURCE_MIC, SINK_SPK):
        case PATH(SOURCE_MIC, SINK_RTX):
        case PATH(SOURCE_MIC, SINK_MCU):
            gpioDev_clear(MIC_PWR_EN);
            gpioDev_clear(INT_MIC_SEL);
            gpioDev_clear(EXT_MIC_SEL);
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
