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

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include "MK22F51212.h"

/* Supported radio bands */
#define BAND_VHF
#define BAND_UHF

/* Band limits in Hz */
#define FREQ_LIMIT_VHF_LO 136000000
#define FREQ_LIMIT_VHF_HI 174000000
#define FREQ_LIMIT_UHF_LO 400000000
#define FREQ_LIMIT_UHF_HI 470000000

/* Screen dimensions */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

/* Screen pixel format */
#define PIX_FMT_BW

/* Battery type */
#define BAT_LIPO_2S

/* Display */
#define LCD_BKLIGHT GPIOC,4
#define LCD_CS      GPIOC,8
#define LCD_RST     GPIOC,9
#define LCD_RS      GPIOC,10
#define LCD_CLK     GPIOC,11
#define LCD_DAT     GPIOC,12

/* Signalling LEDs */
#define GREEN_LED  GPIOB,18
#define RED_LED    GPIOC,14

/* Keyboard */
#define KB_ROW0 GPIOB,19
#define KB_ROW1 GPIOB,20
#define KB_ROW2 GPIOB,21
#define KB_ROW3 GPIOB,22
#define KB_ROW4 GPIOB,23

#define KB_COL0 GPIOC,0
#define KB_COL1 GPIOC,1
#define KB_COL2 GPIOC,2
#define KB_COL3 GPIOC,3

#define PTT_SW   GPIOA,1
#define FUNC_SW  GPIOA,2
#define FUNC2_SW GPIOB,1
#define MONI_SW  GPIOB,9

#endif
