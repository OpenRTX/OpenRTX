/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#ifdef __cplusplus
extern "C" {
#endif

/* Screen dimensions */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

/* Screen pixel format */
#define PIX_FMT_BW

/* Screen has adjustable contrast */
#define SCREEN_CONTRAST
#define DEFAULT_CONTRAST 71

/* Screen has adjustable brightness */
#define SCREEN_BRIGHTNESS

/* Battery type */
#define BAT_LIPO_2S

/* No rtxlink interface */
#define RTXLINK_DEV NULL

/* Power keep switch */
#define PWR_SW GPIOE,26

/* Display */
#define LCD_BKLIGHT GPIOC,4
#define LCD_CS      GPIOC,8
#define LCD_RST     GPIOC,9
#define LCD_RS      GPIOC,10
#define LCD_CLK     GPIOC,11
#define LCD_DAT     GPIOC,12

/* Signalling LEDs */
#define GREEN_LED  GPIOA,17
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

/* External flash */
#define FLASH_CS  GPIOE,6
#define FLASH_CLK GPIOE,5
#define FLASH_SDO GPIOE,4
#define FLASH_SDI GPIOA,19

/* I2C for EEPROM and AT1846S */
#define I2C_SDA GPIOE,25
#define I2C_SCL GPIOE,24

/* RTX stage control */
#define VHF_LNA_EN GPIOC,13
#define UHF_LNA_EN GPIOC,15
#define VHF_PA_EN  GPIOE,0
#define UHF_PA_EN  GPIOE,1

/* Audio control */
#define AUDIO_AMP_EN GPIOB,0
#define RX_AUDIO_MUX GPIOC,5
#define TX_AUDIO_MUX GPIOC,6

/* HR_C6000 control interface */
#define DMR_RESET GPIOE,2
#define DMR_SLEEP GPIOE,3
#define DMR_CS    GPIOD,0
#define DMR_CLK   GPIOD,1
#define DMR_MOSI  GPIOD,2
#define DMR_MISO  GPIOD,3

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
