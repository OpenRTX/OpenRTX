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

    static const size_t numSamples = 45*1024;       // 90kB
    uint16_t *sampleBuf = ((uint16_t *) malloc(numSamples * sizeof(uint16_t)));

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
    rtx_taskFunc();

    delayMs(3000);

    /*
     * ADC and DMA Setup
     */
    gpio_setMode(GPIOC, 3, INPUT_ANALOG);

    RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    __DSB();

    /*
     * TIM2 for conversion triggering via TIM2_TRGO, that is counter reload.
     * AP1 frequency is 42MHz but timer runs at 84MHz, configure for 48kHz
     * interrupt rate.
     */
    TIM2->PSC = 9;      /* Tick rate 8.4MHz     */
    TIM2->ARR = 174;    /* Overflow rate 48kHz  */
    TIM2->CNT = 0;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR2 = TIM_CR2_MMS_1;
    TIM2->CR1 = TIM_CR1_CEN;

    /* DMA2 Stream 2 configuration:
     * - channel 1: ADC2
     * - high priority
     * - half-word transfer, both memory and peripheral
     * - increment memory
     * - peripheral-to-memory transfer
     * - no interrupts
     */
    DMA2_Stream2->PAR = ((uint32_t) &(ADC2->DR));
    DMA2_Stream2->M0AR = ((uint32_t) sampleBuf);
    DMA2_Stream2->NDTR = numSamples;
    DMA2_Stream2->CR = DMA_SxCR_CHSEL_0     /* ADC2 is on channel 1    */
                     | DMA_SxCR_MSIZE_0     /* Memory size: 16 bit     */
                     | DMA_SxCR_PSIZE_0     /* Peripheral size: 16 bit */
                     | DMA_SxCR_PL_1        /* High priority           */
                     | DMA_SxCR_MINC        /* Increment memory        */
                     | DMA_SxCR_EN;

    /*
     * ADC clock is APB2 frequency divided by 8, giving 10.5MHz.
     * A conversion takes 12 cycles.
     */
    ADC->CCR |= ADC_CCR_ADCPRE;
    ADC2->SMPR1 = ADC_SMPR1_SMP13;

    ADC2->SQR1 = 0;    /* One channel to be converted */
    ADC2->SQR3 = 13;   /* CH13, audio from RTX on PC3 */

    /*
     * No overrun interrupt, 12-bit resolution, no analog watchdog, conversion
     * start on external trigger, trigger conversion on TIM2_TRGO rising edge,
     * request DMA transfer.
     */
    ADC2->CR1 |= ADC_CR1_DISCEN;
    ADC2->CR2 |= ADC_CR2_EXTEN_0    /* Trigger on rising edge        */
              |  ADC_CR2_EXTSEL_1
              |  ADC_CR2_EXTSEL_2   /* 0b0110 TIM2_TRGO trig. source */
              |  ADC_CR2_DMA        /* Enable DMA data transfer      */
              |  ADC_CR2_ADON;      /* Enable ADC                    */

    /* Flash LED while DMA is running */
    while((DMA2_Stream2->CR & DMA_SxCR_EN) == 1)
    {
        gpio_togglePin(GREEN_LED);
        delayMs(100);
    }

    /* Dump samples */
    platform_ledOff(GREEN);
    platform_ledOn(RED);
    delayMs(10000);

    for(size_t i = 0; i < numSamples; i++)
    {
        printUnsignedInt(sampleBuf[i]);
    }

    platform_ledOff(RED);

    while(1) ;

    return 0;
}
