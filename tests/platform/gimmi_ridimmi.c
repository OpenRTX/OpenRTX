/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "interfaces/delays.h"
#include "interfaces/gpio.h"
#include "interfaces/platform.h"
#include "rtx/rtx.h"
#include "hwconfig.h"

/*
 * Uncomment this directive to sample audio coming from RTX stage instead of the
 * one from microphone.
 */
// #define SAMPLE_RTX_AUDIO

static const size_t numSamples = 35*1024;       // 70kB

void recordMic(uint16_t *buffer)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    __DSB();

    /*
     * TIM2 for conversion triggering via TIM2_TRGO, that is counter reload.
     * AP1 frequency is 42MHz but timer runs at 84MHz, configure for 8kHz interrupt rate.
     */
    TIM2->PSC = 83;     /* Tick rate 1MHz     */
    TIM2->ARR = 124;    /* Overflow rate 8kHz */
    TIM2->CNT = 0;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR2 = TIM_CR2_MMS_1;
    TIM2->CR1 = TIM_CR1_CEN;

    /* DMA2 Stream 0 configuration:
     * - channel 0: ADC1
     * - low priority
     * - half-word transfer, both memory and peripheral
     * - increment memory
     * - peripheral-to-memory transfer
     * - no interrupts
     */
    DMA2_Stream0->PAR = ((uint32_t) &(ADC1->DR));
    DMA2_Stream0->M0AR = ((uint32_t) buffer);
    DMA2_Stream0->NDTR = numSamples;
    DMA2_Stream0->CR = DMA_SxCR_MSIZE_0     /* Memory size: 16 bit     */
                     | DMA_SxCR_PSIZE_0     /* Peripheral size: 16 bit */
                     | DMA_SxCR_PL_0        /* Medium priority         */
                     | DMA_SxCR_MINC        /* Increment memory        */
                     | DMA_SxCR_EN;

    /*
     * ADC clock is APB2 frequency divided by 8, giving 10.5MHz.
     * A conversion takes 12 cycles.
     */
    ADC->CCR |= ADC_CCR_ADCPRE;
    ADC1->SMPR2 = ADC_SMPR2_SMP0
                | ADC_SMPR2_SMP1
                | ADC_SMPR2_SMP3
                | ADC_SMPR2_SMP8;

    ADC1->SQR1 = 0;    /* One channel to be converted */

    #ifdef SAMPLE_RTX_AUDIO
    ADC1->SQR3 = 13;   /* CH13, audio from RTX on PC3  */
    #else
    ADC1->SQR3 = 3;    /* CH3, vox level on PA3        */
    #endif

    /*
     * No overrun interrupt, 12-bit resolution, no analog watchdog, no
     * discontinuous mode, enable scan mode, no end of conversion interrupts,
     * enable continuous conversion (free-running).
     */
    ADC1->CR1 |= ADC_CR1_DISCEN;
    ADC1->CR2 |= ADC_CR2_EXTEN_0    /* Trigger on rising edge        */
              |  ADC_CR2_EXTSEL_1
              |  ADC_CR2_EXTSEL_2   /* 0b0110 TIM2_TRGO trig. source */
              |  ADC_CR2_DDS        /* Enable DMA data transfer      */
              |  ADC_CR2_DMA
              |  ADC_CR2_ADON;      /* Enable ADC                    */

    /* Wait until DMA transfer ends, meanwhile blink the green led */
    while((DMA2_Stream0->CR & DMA_SxCR_EN) == 1)
    {
        gpio_togglePin(GREEN_LED);
        delayMs(250);
    }
}

void playbackSpk(uint16_t *buffer)
{
    /*
     * Enable peripherals
     */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN
                 |  RCC_APB1ENR_TIM7EN;
    __DSB();

    /*
     * PWM for tone generator time base: 328kHz
     */
    TIM3->ARR   = 0xFF;
    TIM3->PSC   = 0;
    TIM3->CCMR2 = TIM_CCMR2_OC3M_2
                | TIM_CCMR2_OC3M_1
                | TIM_CCMR2_OC3PE;
    TIM3->CR1  |= TIM_CR1_ARPE;

    TIM3->CCER |= TIM_CCER_CC3E;
    TIM3->CR1  |= TIM_CR1_CEN;

    /*
     * Timebase for 8kHz sample rate
     */
    TIM7->CNT  = 0;
    TIM7->PSC  = 0;
    TIM7->ARR  = (84000000/8000) - 1;
    TIM7->EGR  = TIM_EGR_UG;
    TIM7->DIER = TIM_DIER_UDE;
    TIM7->CR1  = TIM_CR1_CEN;

    /*
     * DMA stream for sample transfer
     */
    DMA1_Stream2->NDTR = numSamples;
    DMA1_Stream2->PAR  = ((uint32_t) &(TIM3->CCR3));
    DMA1_Stream2->M0AR = ((uint32_t) buffer);
    DMA1_Stream2->CR = DMA_SxCR_CHSEL_0       /* Channel 1                   */
                     | DMA_SxCR_PL            /* Very high priority          */
                     | DMA_SxCR_MSIZE_0
                     | DMA_SxCR_PSIZE_0
                     | DMA_SxCR_MINC          /* Increment source pointer    */
                     | DMA_SxCR_CIRC          /* Circular mode               */
                     | DMA_SxCR_DIR_0         /* Memory to peripheral        */
                     | DMA_SxCR_EN;           /* Start transfer              */


    /* Turn on audio amplifier and unmute speaker */
    gpio_setMode(AUDIO_AMP_EN, OUTPUT);
    gpio_setMode(SPK_MUTE,     OUTPUT);

    gpio_setPin(AUDIO_AMP_EN);
    delayMs(10);
    gpio_clearPin(SPK_MUTE);
}

int main()
{
//     platform_init();

    uint16_t *sampleBuf = ((uint16_t *) malloc(numSamples * sizeof(uint16_t)));

    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED,   OUTPUT);
    gpio_setMode(MIC_PWR,   OUTPUT);
    gpio_setPin(MIC_PWR);

    #ifdef SAMPLE_RTX_AUDIO
    gpio_setMode(GPIOC, 3, INPUT_ANALOG);
    #else
    gpio_setMode(AIN_MIC, INPUT_ANALOG);
    #endif

    /* AF2 is TIM3 channel 3 */
    gpio_setMode(BEEP_OUT, ALTERNATE);
    gpio_setAlternateFunction(BEEP_OUT, 2);

    gpio_setPin(GREEN_LED);

    delayMs(3000);

    recordMic(sampleBuf);

    /* Convert samples from 12 bit to 8 bit, as required by PWM audio */
    for(size_t i = 0; i < numSamples; i++)
    {
        sampleBuf[i] >>= 4;
    }

    /* End of recording, play sound */
    gpio_clearPin(GREEN_LED);
    gpio_setPin(RED_LED);

    delayMs(2000);

    playbackSpk(sampleBuf);

    while(1) ;

    return 0;
}
