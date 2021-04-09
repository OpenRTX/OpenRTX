/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <interfaces/platform.h>
#include <rtx.h>
#include <hwconfig.h>

/* Uncomment this to transmit 187.5Hz tone instead of M17 signal */
// #define CW_TEST

extern int16_t m17_buf[];
extern uint16_t nSamples;

/*
 * Sine table for CW test: 256 samples of a full period of a 187.5Hz sinewave,
 * sampled at 48kHz
 */
const uint16_t sine[] =
{
    128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,176,179,182,
    185,188,190,193,196,198,201,203,206,208,211,213,215,218,220,222,224,226,228,
    230,232,234,235,237,238,240,241,243,244,245,246,248,249,250,250,251,252,253,
    253,254,254,254,255,255,255,255,255,255,255,254,254,254,253,253,252,251,250,
    250,249,248,246,245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,
    220,218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,176,173,
    170,167,165,162,158,155,152,149,146,143,140,137,134,131,128,124,121,118,115,
    112,109,106,103,100,97,93,90,88,85,82,79,76,73,70,67,65,62,59,57,54,52,49,47,
    44,42,40,37,35,33,31,29,27,25,23,21,20,18,17,15,14,12,11,10,9,7,6,5,5,4,3,2,
    2,1,1,1,0,0,0,0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,17,18,20,21,23,
    25,27,29,31,33,35,37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,79,82,85,
    88,90,93,97,100,103,106,109,112,115,118,121,124
};

int main(void)
{
    platform_init();

    /* AF2 is TIM3 channel 3 */
    gpio_setMode(BEEP_OUT, ALTERNATE);
    gpio_setAlternateFunction(BEEP_OUT, 2);


    /*
     * Prepare buffer for 8-bit waveform samples
     */
    #ifndef CW_TEST
    uint16_t *buf = ((uint16_t *) malloc(nSamples * sizeof(uint16_t)));
    for(size_t i = 0; i < nSamples; i++)
    {
        int16_t sample = 32768 - m17_buf[i];
        buf[i] = ((uint16_t) sample) >> 8;
    }
    #endif

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
     * Timebase for 48kHz sample rate
     */
    TIM7->CNT  = 0;
    TIM7->PSC  = 0;
    TIM7->ARR  = 1749;//(84000000/48000) - 1;
    TIM7->EGR  = TIM_EGR_UG;
    TIM7->DIER = TIM_DIER_UDE
               | TIM_DIER_UIE;
    TIM7->CR1  = TIM_CR1_CEN;

    /*
     * DMA stream for sample transfer
     */
    #ifdef CW_TEST
    DMA1_Stream2->NDTR = 256;
    DMA1_Stream2->M0AR = ((uint32_t) &sine);
    #else
    DMA1_Stream2->NDTR = nSamples;
    DMA1_Stream2->M0AR = ((uint32_t) buf);
    #endif
    DMA1_Stream2->PAR  = ((uint32_t) &(TIM3->CCR3));
    DMA1_Stream2->CR = DMA_SxCR_CHSEL_0       /* Channel 1                   */
                     | DMA_SxCR_PL            /* Very high priority          */
                     | DMA_SxCR_MSIZE_0
                     | DMA_SxCR_PSIZE_0
                     | DMA_SxCR_MINC          /* Increment source pointer    */
                     | DMA_SxCR_CIRC          /* Circular mode               */
                     | DMA_SxCR_DIR_0         /* Memory to peripheral        */
                     | DMA_SxCR_EN;           /* Start transfer              */
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

    while(1)
    {
        rtx_taskFunc();
        delayMs(10);
    }

    return 0;
}
