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

void __attribute__((used)) TIM7_IRQHandler()
{
    int16_t sample = ((int16_t *) m17_buf)[pos] + 32768;
    uint16_t value = ((uint16_t) sample);
    DAC->DHR12R2 = value >> 4;

    pos++;
    if(pos > 46072) pos = 0;
}


int main(void)
{
    platform_init();
    toneGen_init();

    OSMutexCreate(&mutex, "", &err);
    rtx_init(&mutex);

    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    __DSB();

    TIM7->CNT  = (42000000/48000) - 1;
    TIM7->DIER = TIM_DIER_UIE;

    NVIC_ClearPendingIRQ(TIM7_IRQn);
    NVIC_SetPriority(TIM7_IRQn, 5);
    NVIC_EnableIRQ(TIM7_IRQn);

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

    TIM7->CR1 |= TIM_CR1_CEN;

    while (1)
    {
        rtx_taskFunc();
        OSTimeDlyHMSM(0u, 0u, 0u, 100u, OS_OPT_TIME_HMSM_STRICT, &err);
    }

    return 0;
}
