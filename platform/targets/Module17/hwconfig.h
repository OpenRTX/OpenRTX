/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO                             *
 *                         Mathis Schmieder DB9MAT                         *
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

/* Screen dimensions */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

/* Screen has adjustable contrast */
#define SCREEN_CONTRAST
#define DEFAULT_CONTRAST 91

/* Screen pixel format */
#define PIX_FMT_BW

/* Device has no battery */
#define BAT_NONE

/* Signalling LEDs */
#define PTT_LED     GPIOE,13
#define SYNC_LED    GPIOE,14
#define ERR_LED     GPIOE,15

/* Display */
#define LCD_RST     GPIOB,11
#define LCD_RS      GPIOB,12
#define LCD_CS      GPIOB,10
#define LCD_BKLIGHT GPIOE,15
#define SPI2_CLK    GPIOB,13
#define SPI2_SDO    GPIOB,14
#define SPI2_SDI    GPIOB,15

/* Keyboard */
#define ESC_SW      GPIOD,1
#define RIGHT_SW    GPIOD,2
#define UP_SW       GPIOD,3
#define DOWN_SW     GPIOD,4
#define LEFT_SW     GPIOB,9
#define ENTER_SW    GPIOB,8

#define PTT_SW      GPIOD,8
#define PTT_OUT     GPIOD,10

/* Audio */
#define AUDIO_MIC   GPIOA,2
#define AUDIO_SPK   GPIOA,5
#define BASEBAND_RX GPIOA,1
#define BASEBAND_TX GPIOA,4

#endif
