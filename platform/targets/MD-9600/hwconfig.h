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

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include <stm32f4xx.h>

/* Device has a working real time clock */
#define HAS_RTC

/* Supported radio bands */
#define BAND_VHF
#define BAND_UHF

/* Band limits in Hz */
#define FREQ_LIMIT_VHF_LO 136000000
#define FREQ_LIMIT_VHF_HI 174000000
#define FREQ_LIMIT_UHF_LO 400000000
#define FREQ_LIMIT_UHF_HI 480000000

/* Screen dimensions */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

/* Screen has adjustable contrast */
#define SCREEN_CONTRAST

/* Screen pixel format */
#define PIX_FMT_BW

/* Battery type */
#define BAT_NONE

#endif
