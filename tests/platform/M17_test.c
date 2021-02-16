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
#include <os.h>
#include <inttypes.h>
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <interfaces/rtx.h>
#include <interfaces/platform.h>
#include "hwconfig.h"
#include "toneGenerator_MDx.h"
#include "HR-C5000_MD3x0.h"

OS_MUTEX mutex;
OS_ERR err;

extern int16_t m17_buf[];

uint16_t pos = 0;

// void __attribute__((used)) TIM7_IRQHandler()
// {
//     OSIntEnter();
//
//     TIM7->SR  = 0;
//
//     int16_t sample = ((int16_t *) m17_buf)[pos] + 32768;
//     uint16_t value = ((uint16_t) sample);
// //     DAC->DHR12R2 = value >> 4;
//     TIM3->CCR3 = value >> 8;
//
//     pos++;
//     if(pos > 46072) pos = 0;
//     if(pos == 0)
//         GPIOB->BSRRL = 1 << 3;
//     else
//         GPIOB->BSRRH = 1 << 3;
//
//     OSIntExit();
// }

void __attribute__((used)) TIM3_IRQHandler()
{
    OSIntEnter();

    TIM3->SR = 0;

    int16_t sample = ((int16_t *) m17_buf)[pos] + 32768;
    uint16_t value = ((uint16_t) sample);
    TIM3->CCR3 = value >> 8;

    pos++;
    if(pos > 46072) pos = 0;
    if(pos == 0)
        GPIOB->BSRRL = 1 << 3;
    else
        GPIOB->BSRRH = 1 << 3;

    OSIntExit();
}


int main(void)
{
    platform_init();
//     toneGen_init();

//     toneGen_setBeepFreq(2400.0f);
//     toneGen_beepOn();

    gpio_setMode(GPIOB, 3, OUTPUT);
    gpio_clearPin(GPIOB, 3);

    gpio_setMode(BEEP_OUT,  ALTERNATE);
    gpio_setAlternateFunction(BEEP_OUT,  2);

    /*
     * Timer configuration:
     * - APB1 frequency = 42MHz but timer run at twice of this frequency: with
     * 1:20 prescaler we have Ftick = 4.2MHz
     * - ARR = 255 (8-bit PWM), gives an update rate of 16.406kHz
     * - Nominal update rate is 16.384kHz -> error = +22.25Hz
     */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    __DSB();

    TIM3->ARR   = 0xFF;
    TIM3->PSC   = 6;
    TIM3->CCMR2 = TIM_CCMR2_OC3M_2  /* The same for CH3                   */
                | TIM_CCMR2_OC3M_1
                | TIM_CCMR2_OC3PE;
    TIM3->DIER |= TIM_DIER_UIE;     /* Interrupt on counter update        */
    TIM3->CR1  |= TIM_CR1_ARPE;     /* Enable auto preload on reload      */

    TIM3->CCER |= TIM_CCER_CC3E;
    TIM3->CR1  |= TIM_CR1_CEN;

    NVIC_SetPriority(TIM3_IRQn, 10);
    NVIC_EnableIRQ(TIM3_IRQn);


//     RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
//     __DSB();
//
//     TIM7->CNT  = 0;
//     TIM7->PSC  = 0;
//     TIM7->ARR  = 1749;//(84000000/48000) - 1;
//     TIM7->EGR  = TIM_EGR_UG;
//     TIM7->DIER = TIM_DIER_UIE;
//     TIM7->CR1  = TIM_CR1_CEN;
//
//     NVIC_ClearPendingIRQ(TIM7_IRQn);
//     NVIC_SetPriority(TIM7_IRQn, 10);
//     NVIC_EnableIRQ(TIM7_IRQn);


    OSMutexCreate(&mutex, "", &err);
    rtx_init(&mutex);
    rtxStatus_t cfg;

    /* Take mutex and update the RTX configuration */
    OSMutexPend(&mutex, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

    cfg.opMode = FM;
    cfg.bandwidth = BW_25;
    cfg.rxFrequency = 435000000;
    cfg.txFrequency = 435000000;
    cfg.txPower = 1.0f;
    cfg.sqlLevel = 3;
    cfg.rxTone = 0;
    cfg.txTone = 0;

    OSMutexPost(&mutex, OS_OPT_POST_NONE, &err);

    /* After mutex has been released, post the new configuration */
    rtx_configure(&cfg);


    while (1)
    {
        rtx_taskFunc();
        OSTimeDlyHMSM(0u, 0u, 0u, 10u, OS_OPT_TIME_HMSM_STRICT, &err);
    }

    return 0;
}
