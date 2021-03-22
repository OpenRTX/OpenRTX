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

extern int16_t m17_buf[];
extern uint16_t nSamples;

// void __attribute__((used)) _Z23DMA1_Stream2_IRQHandlerv()
// {
//     GPIOE->BSRRL = 1;
//     DMA1->LIFCR |= DMA_LIFCR_CTCIF2 | DMA_LIFCR_CTEIF2;
//     GPIOB->BSRRL = 1 << 3;
//     delayUs(20);
//     GPIOB->BSRRH = 1 << 3;
//     GPIOE->BSRRH = 1;
// }

int main(void)
{
    platform_init();

//     gpio_setMode(GPIOB, 3, OUTPUT);
//     gpio_clearPin(GPIOB, 3);

    /* AF2 is TIM3 channel 3 */
    gpio_setMode(BEEP_OUT, ALTERNATE);
    gpio_setAlternateFunction(BEEP_OUT, 2);


    /*
     * Prepare buffer for 8-bit waveform samples
     */
    uint16_t *buf = ((uint16_t *) malloc(nSamples * sizeof(uint16_t)));
    for(size_t i = 0; i < nSamples; i++)
    {
        int16_t sample = 32768 - m17_buf[i];
        buf[i] = ((uint16_t) sample) >> 8;
    }

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
    DMA1_Stream2->NDTR = nSamples;
    DMA1_Stream2->PAR  = ((uint32_t) &(TIM3->CCR3));
    DMA1_Stream2->M0AR = ((uint32_t) buf);
    DMA1_Stream2->CR = DMA_SxCR_CHSEL_0       /* Channel 1                   */
                     | DMA_SxCR_PL            /* Very high priority          */
                     | DMA_SxCR_MSIZE_0
                     | DMA_SxCR_PSIZE_0
                     | DMA_SxCR_MINC          /* Increment source pointer    */
                     | DMA_SxCR_CIRC          /* Circular mode               */
                     | DMA_SxCR_DIR_0         /* Memory to peripheral        */
                     | DMA_SxCR_EN;           /* Start transfer              */

//                      | DMA_SxCR_TCIE          /* Transfer complete interrupt */
//                      | DMA_SxCR_TEIE          /* Transfer error interrupt    */

//     NVIC_ClearPendingIRQ(DMA1_Stream2_IRQn);
//     NVIC_SetPriority(DMA1_Stream2_IRQn, 10);
//     NVIC_EnableIRQ(DMA1_Stream2_IRQn);

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
