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
#include "gpio.h"
#include "delays.h"
#include "rtx.h"
#include "platform.h"
#include "hwconfig.h"
#include "toneGenerator_MDx.h"

int main(void)
{
    platform_init();
    toneGen_init();

    rtx_init();

    freq_t rptFreq  = 430100000;
    freq_t rptShift = 1600000;
    tone_t ctcss    = 719;

    /*
     * Allocate a configuration message for RTX.
     * This memory chunk is then freed inside the driver, so you must not call
     * free() in this pointer!
     */
    rtxConfig_t *cfg = ((rtxConfig_t *) malloc(sizeof(rtxConfig_t)));

    cfg->opMode = FM;
    cfg->bandwidth = BW_25;
    cfg->rxFrequency = rptFreq;
    cfg->txFrequency = rptFreq + rptShift;
    cfg->txPower = 1.0f;
    cfg->sqlLevel = 3;
    cfg->rxTone = 0;
    cfg->txTone = ctcss;
    rtx_configure(cfg);

    while (1)
    {

        rtx_taskFunc();

        OS_ERR err;
        OSTimeDlyHMSM(0u, 0u, 0u, 10u, OS_OPT_TIME_HMSM_STRICT, &err);
    }

    return 0;
}
