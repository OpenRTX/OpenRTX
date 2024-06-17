/***************************************************************************
 *   Copyright (C) 2024 Silvano Seva IU2KWO                                *
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

#include <stm32g4xx.h>

/* PTT control */
#define PTT_IN        GPIOC,13
#define PTT_OUT       GPIOA,6

/* Audio */
#define AUDIO_IN      GPIOA,2   // ADC1_IN3
#define AUDIO_OUT     GPIOA,5   // DAC1_OUT2
#define BASEBAND_IN   GPIOA,1   // ADC12_IN2
#define BASEBAND_OUT  GPIOA,4   // DAC1_OUT1
#define BASEBAND_BYP  GPIOA,7
#define AUDIOOUT_MUTE GPIOB,1

/* I2C for MCP4451 */
#define I2C_SDA       GPIOB,9
#define I2C_SCL       GPIOA,15

/* Serial ports */
#define MCU_TX        GPIOA,9
#define MCU_RX        GPIOA,10
#define USART3_TX     GPIOB,10
#define USART3_RX     GPIOB,11

/* Misc */
#define AIN_HWVER     GPIOA,3   // ADC1_IN4

#endif /* PINMAP_H */
