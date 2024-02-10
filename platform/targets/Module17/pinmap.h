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

#ifndef PINMAP_H
#define PINMAP_H

#include <stm32f4xx.h>

/* Signalling LEDs */
#define PTT_LED     GPIOC,8
#define SYNC_LED    GPIOC,9
#define ERR_LED     GPIOA,8

/* Display */
#define LCD_RST         GPIOC,7
#define LCD_DC          GPIOC,6
#define LCD_CS          GPIOB,14
#define SPI2_SCK        GPIOB,13
#define SPI2_MISO       GPIOB,9     // UNUSED
#define SPI2_MOSI       GPIOB,15

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

#define AIN_HWVER   GPIOA,3
#define POWER_SW    GPIOA,15

/* I2C for MCP4551 */
#define I2C1_SDA    GPIOB,7
#define I2C1_SCL    GPIOB,6
#define SOFTPOT_RX  0x2E
#define SOFTPOT_TX  0x2F

#endif /* PINMAP_H */
