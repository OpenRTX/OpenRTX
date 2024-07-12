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

#include <gd32f3x0.h>

// LCD display
#define LCD_CLK     GPIOB,13    
#define LCD_DAT     GPIOB,15
#define LCD_PWR     GPIOB,7
#define LCD_RST     GPIOB,12
#define LCD_CS      GPIOB,2
#define LCD_DC      GPIOB,10                

// LEDs
#define GREEN_LED   GPIOB,1
#define RED_LED     GPIOF,2

// Analog inputs
#define AIN_VBAT    GPIOA,1

// Push-to-talk
#define PTT_SW      GPIOA,10

// Keyboard and side keys
#define KBD_ROW0    GPIOB,3
#define KBD_ROW1    GPIOB,4
#define KBD_ROW2    GPIOB,5
#define KBD_ROW3    GPIOB,6
#define KBD_COL0    GPIOB,11
#define KBD_COL1    GPIOB,9
#define KBD_COL2    GPIOB,14
#define KBD_COL3    GPIOB,8
#define SKB_1       GPIOB,11
#define SKB_2       GPIOF,7

// External flash
#define EFLASH_MISO GPIOA,6
#define EFLASH_MOSI GPIOA,7
#define EFLASH_SCK  GPIOB,5
#define EFLASH_CS   GPIOB,4

// BK1080
#define BK1080_CLK  GPIOC,14
#define BK1080_DAT  GPIOB,4     // Shared with external flash SCK
#define BK1080_CS   GPIOC,13

// BK4819
#define BK4819_CLK  GPIOA,2
#define BK4819_DAT  GPIOA,3
#define BK4819_CS   GPIOC,13

// Audio control
#define SPK_EN      GPIOA,1
#define AF_OUT      GPIOA,6

// RF stage
#define TX_LDO      GPIOB,10
#define TX_PA_EN    GPIOB,11
#endif
