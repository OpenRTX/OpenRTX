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
#include <stdbool.h>
#include <stdlib.h>
#include <os.h>
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <interfaces/rtx.h>
#include <interfaces/platform.h>
#include "hwconfig.h"
#include "toneGenerator_MDx.h"

OS_MUTEX mutex;
OS_ERR err;

static const freq_t rptFreq  = 430100000;
static const freq_t rptShift = 1600000;
static const tone_t ctcss    = 719;

int main(void)
{
    platform_init();
    toneGen_init();

    OSMutexCreate(&mutex, "", &err);

    rtx_init(&mutex);


    rtxStatus_t cfg;

    /* Take mutex and update the RTX configuration */
    OSMutexPend(&mutex, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

    cfg.opMode = OPMODE_FM;
    cfg.bandwidth = BW_25;
    cfg.rxFrequency = rptFreq;
    cfg.txFrequency = rptFreq + rptShift;
    cfg.txPower = 1.0f;
    cfg.sqlLevel = 3;
    cfg.rxTone = 0;
    cfg.txTone = ctcss;

    OSMutexPost(&mutex, OS_OPT_POST_NONE, &err);

    /* After mutex has been released, post the new configuration */
    rtx_configure(&cfg);

    while (1)
    {
        rtx_taskFunc();

        /* You can also resync the cfg */
        cfg = rtx_getCurrentStatus();

        OSTimeDlyHMSM(0u, 0u, 0u, 10u, OS_OPT_TIME_HMSM_STRICT, &err);
    }

    return 0;
}
