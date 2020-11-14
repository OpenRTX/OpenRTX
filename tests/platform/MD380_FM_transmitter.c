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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <os.h>
#include "gpio.h"
#include "delays.h"
#include "rtx.h"
#include "ADC1_MDxx380.h"
#include "graphics.h"
#include "platform.h"
#include "hwconfig.h"
#include "HR-C5000_MD3x0.h"
#include "toneGenerator_MDxx380.h"

int main(void)
{
    platform_init();
    toneGen_init();
    toneGen_setToneFreq(77.0f);

    gpio_setMode(SPK_MUTE, OUTPUT);
    gpio_setMode(AMP_EN,   OUTPUT);
    gpio_setMode(FM_MUTE,  OUTPUT);

    gpio_setPin(AMP_EN);        /* Turn on audio amplifier              */
    gpio_setPin(FM_MUTE);       /* Unmute path from AF_out to amplifier */
    gpio_clearPin(SPK_MUTE);    /* Unmute speaker                       */

    rtx_init();
    rtx_setTxFreq(432950000.0f);
    rtx_setBandwidth(BW_25);
    rtx_setOpmode(FM);
    rtx_setFuncmode(OFF);

    gpio_setMode(GPIOE, 0,  OUTPUT);
    gpio_setMode(GPIOA, 14, OUTPUT);
    gpio_setPin(GPIOA, 14);

    while (1)
    {
        OS_ERR err;
        gpio_setPin(GPIOE, 0);
        rtx_setFuncmode(TX);
        C5000_activateAnalogTx();
//         toneGen_toneOn();
        OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &err);
//         toneGen_toneOff();
        C5000_shutdownAnalogTx();
        rtx_setFuncmode(OFF);
        gpio_clearPin(GPIOE, 0);

        OSTimeDlyHMSM(0u, 0u, 3u, 0u, OS_OPT_TIME_HMSM_STRICT, &err);
    }

    return 0;
}
