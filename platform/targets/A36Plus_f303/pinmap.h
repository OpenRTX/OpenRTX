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

#include <gd32f30x.h>

// LCD display
#define LCD_CLK GPIOB, 13
#define LCD_DAT GPIOB, 15
#define LCD_PWR GPIOB, 7
#define LCD_RST GPIOB, 12
#define LCD_CS GPIOB, 2
#define LCD_DC GPIOB, 10

// LEDs
#define GREEN_LED GPIOF, 9
#define RED_LED GPIOF, 10

// Analog inputs
#define AIN_VBAT GPIOA, 1

// Push-to-talk
#define PTT_SW GPIOA, 10

// Keyboard and side keys
#define KBD_K0 GPIOB, 3
#define KBD_K1 GPIOB, 4
#define KBD_K2 GPIOB, 5
#define KBD_K3 GPIOB, 6
#define KBD_DB3 GPIOB, 11
#define KBD_DB2 GPIOB, 9
#define KBD_DB1 GPIOB, 14
#define KBD_DB0 GPIOB, 8

// External flash
#define FLASH_SDI GPIOA, 7
#define FLASH_SDO GPIOA, 6
#define FLASH_CLK GPIOA, 5
//#define FLASH_CS GPIOA, 4
#define FLASH_CS  &GpioA,4

// BK1080
#define BK1080_CLK GPIOD, 6
#define BK1080_DAT GPIOA, 3  // Shared with external flash SCK
#define BK1080_EN GPIOA, 8

// BK4819
#define BK4819_CLK GPIOA, 2
#define BK4819_DAT GPIOA, 3
#define BK4819_CS GPIOC, 13

// Audio control
#define MIC_SPK_EN GPIOA, 12

// RF stage
#define RF_AM_AGC GPIOB, 1
#define RFV3R_EN GPIOB, 0

#define RFV3T_EN GPIOC, 15
#define RFU3R_EN GPIOC, 14

// Power button
#define PWR_SW GPIOF,7


//[Tristan]Not for production! For testing with gd32f303 board. PB2 not reachable
//Uncomment #define GD303_DEV_BOARD to enable
//#define GD303_DEV_BOARD
#ifdef GD303_DEV_BOARD
    // Undefine the previous LCD_CS definition and redefine it for debug mode
    #undef LCD_CS
    #undef RF_AM_AGC
    #define LCD_CS GPIOB, 1
    #define RF_AM_AGC GPIOB, 2
#endif


#endif