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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dsp.h>
#include <interfaces/delays.h>
#include <interfaces/gpio.h>
#include <hwconfig.h>
#include <interfaces/platform.h>
#include <inttypes.h>
#include <rtx.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Uncomment this directive to sample audio coming from RTX stage instead of the
 * one from microphone.
 */
// #define SAMPLE_RTX_AUDIO

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
    platform_init();

    static const size_t numSamples = 45*1024;       // 80kB
    uint16_t *sampleBuf = ((uint16_t *) malloc(numSamples * sizeof(uint16_t)));

    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED,   OUTPUT);

    gpio_setMode(GPIOC, 3, INPUT_ANALOG);

    delayMs(3000);

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    __DSB();

    /*
     * TIM2 for conversion triggering via TIM2_TRGO, that is counter reload.
     * AP1 frequency is 42MHz but timer runs at 84MHz, configure for 48kHz interrupt rate.
     */
    TIM2->PSC = 9;      /* Tick rate 8.4MHz     */
    TIM2->ARR = 174;    /* Overflow rate 48kHz  */
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
    DMA2_Stream0->M0AR = ((uint32_t) sampleBuf);
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
    ADC1->SQR3 = 13;   /* CH13, audio from RTX on PC13 */

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
              |  ADC_CR2_ALIGN
              |  ADC_CR2_ADON;      /* Enable ADC                    */

    /*
     * Baseband setup
     */
    pthread_mutex_t rtx_mutex;
    pthread_mutex_init(&rtx_mutex, NULL);

    rtx_init(&rtx_mutex);
    rtxStatus_t cfg;

    /* Take mutex and update the RTX configuration */
    pthread_mutex_lock(&rtx_mutex);

    cfg.opMode = FM;
    cfg.bandwidth = BW_25;
    cfg.rxFrequency = 435000000;
    cfg.txFrequency = 435000000;
    cfg.txPower = 1.0f;
    cfg.sqlLevel = 3;
    cfg.rxTone = 0;
    cfg.txTone = 0;

    pthread_mutex_unlock(&rtx_mutex);

    /* After mutex has been released, post the new configuration */
    rtx_configure(&cfg);

    while((DMA2_Stream0->CR & DMA_SxCR_EN) == 1)
    {
        rtx_taskFunc();
        gpio_togglePin(GREEN_LED);
        delayMs(250);
    }

    gpio_clearPin(GREEN_LED);
    gpio_setPin(RED_LED);

    delayMs(10000);

    for(size_t i = 0; i < numSamples; i++)
    {
        printUnsignedInt(sampleBuf[i]);
    }

    while(1) ;

    return 0;
}
