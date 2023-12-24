/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <at32f421.h>
#include "pinmap.h"

// Screen dimensions and pixel format
#define CONFIG_SCREEN_WIDTH  160
#define CONFIG_SCREEN_HEIGHT 96
#define PIX_FMT_RGB565
#define NO_FRAMEBUF
#define NO_RADIO
#define NO_BATTERY
#define NO_DMR
#define NO_M17
#define NO_VOICE_PROMPTS

/* Keyboard */
#define KB_ROW0 GPIOA,10
#define KB_ROW1 GPIOB,1
#define KB_ROW2 GPIOB,2
#define KB_ROW3 GPIOA,9
#define KB_COL0 GPIOB,15
#define KB_COL1 GPIOB,14
#define KB_COL2 GPIOB,13
#define KB_COL3 GPIOA,8


// Battery type
#define BAT_LIPO_2S

#endif
