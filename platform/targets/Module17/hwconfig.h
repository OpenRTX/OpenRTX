/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Frederik Saraci IU2NRO,                  *
 *                                Silvano Seva IU2KWO                      *
 *                                Mathis Schmieder DB9MAT                  *
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

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include <stm32f4xx.h>
#include <stdint.h>
#include <stdbool.h>
#include "pinmap.h"

/* Screen dimensions */
#define CONFIG_SCREEN_WIDTH 128
#define CONFIG_SCREEN_HEIGHT 64

/* Screen pixel format */
#define CONFIG_PIX_FMT_BW

/* Device has no battery */
#define CONFIG_BAT_NONE

/* Device supports M17 mode */
#define CONFIG_M17

/* Hardware revision */
#define CONFIG_HWVER_CNT_MARG   300

#define CONFIG_HWVER_0_1_D      0
#define CONFIG_HWVER_0_1_D_CNT  0     /* pulled to ground */

#define CONFIG_HWVER_0_1_E      1
#define CONFIG_HWVER_0_1_E_CNT  3300  /* pulled to VCC */

#define CONFIG_HWVER_1_0        2
#define CONFIG_HWVER_1_0_CNT    1650  /* VCC/2 */

#define CONFIG_HMI_HWVER_1_0        1
#define CONFIG_HMI_HWVER_1_0_CNT    1688  /* VCC/1.95 */

typedef struct {
    bool    HMI_present;
    uint8_t HMI_hw_version;
} Mod17_HwInfo_t;

#endif
