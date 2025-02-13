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

#include <at32f421.h>

// LCD display
#define LCD_CLK     GPIOA,0
#define LCD_DAT     GPIOA,4
#define LCD_PWR     GPIOA,5
#define LCD_RST     GPIOF,0
#define LCD_CS      GPIOC,15
#define LCD_DC      GPIOF,1

// LEDs
#define GREEN_LED   GPIOA,14
#define RED_LED     GPIOA,13

// Analog inputs
#define AIN_VBAT    GPIOB,12

// Push-to-talk
#define PTT_SW      GPIOB,7

// Keyboard and side keys
#define KBD_ROW0    GPIOA,10
#define KBD_ROW1    GPIOB,1
#define KBD_ROW2    GPIOB,2
#define KBD_ROW3    GPIOA,9
#define KBD_COL0    GPIOB,15
#define KBD_COL1    GPIOB,14
#define KBD_COL2    GPIOB,13
#define KBD_COL3    GPIOA,8
#define SKB_1       GPIOA,15
#define SKB_2       GPIOF,7

// External flash
#define EFLASH_MISO GPIOB,3
#define EFLASH_MOSI GPIOA,7
#define EFLASH_SCK  GPIOB,4     // Shared with BK1080 DAT
#define EFLASH_CS   GPIOB,0

// BK1080
#define BK1080_CLK  GPIOC,14
#define BK1080_DAT  GPIOB,4     // Shared with external flash SCK
#define BK1080_CS   GPIOC,13

// BK4819
#define BK4819_CLK  GPIOB,9
#define BK4819_DAT  GPIOB,5
#define BK4819_CS   GPIOB,8

// Audio control
#define SPK_EN      GPIOA,1
#define AF_OUT      GPIOA,6

// RF stage
#define TX_LDO      GPIOB,10
#define TX_PA_EN    GPIOB,11

// USART
#define USART1_TX   GPIOB,6

#endif
