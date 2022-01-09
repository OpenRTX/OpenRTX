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
#define BAT_MOD17

/* Signalling LEDs */
#define PTT_LED     GPIOC,8
#define SYNC_LED    GPIOC,9
#define ERR_LED     GPIOA,8

/* Display */
#define LCD_RST     GPIOC,7
#define LCD_RS      GPIOC,6
#define LCD_CS      GPIOB,14
#define SPI2_CLK    GPIOB,13
#define SPI2_SDO    GPIOB,9     // UNUSED
#define SPI2_SDI    GPIOB,15
//#define LCD_BKLIGHT GPIOE,15

/* Keyboard */
#define ESC_SW      GPIOB,8
#define RIGHT_SW    GPIOB,11
#define UP_SW       GPIOB,10
#define DOWN_SW     GPIOC,2
#define LEFT_SW     GPIOC,3
#define ENTER_SW    GPIOB,12

#define PTT_SW      GPIOC,13
#define PTT_OUT     GPIOD,2

/* Audio */
#define AUDIO_MIC   GPIOA,2
#define AUDIO_SPK   GPIOA,5
#define BASEBAND_RX GPIOA,1
#define BASEBAND_TX GPIOA,4
#define SPK_MUTE    GPIOB,1
#define MIC_MUTE    GPIOC,4
#define MIC_GAIN    GPIOC,5

#define AIN_VBAT   GPIOA,3

/* I2C for MCP4551 */
#define I2C_SDA GPIOB,7
#define I2C_SCL GPIOB,6
#define SOFTPOT_RX 0x2E
#define SOFTPOT_TX 0x2F

/* M17 demodulation */
#define M17_RX_SAMPLE_RATE 24000

#endif
