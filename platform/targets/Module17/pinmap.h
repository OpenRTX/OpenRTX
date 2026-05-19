/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PINMAP_H
#define PINMAP_H

#include "stm32f4xx.h"

/* Signalling LEDs */
#define PTT_LED         GPIOC,8
#define SYNC_LED        GPIOC,9
#define ERR_LED         GPIOA,8

/* Display */
#define LCD_RST         GPIOC,7
#define LCD_DC          GPIOC,6
#define LCD_CS          GPIOB,14
#define SPI2_SCK        GPIOB,13
#define SPI2_MISO       GPIOB,9     // UNUSED
#define SPI2_MOSI       GPIOB,15

/* Keyboard */
#define ESC_SW          GPIOB,8
#define RIGHT_SW        GPIOB,11
#define UP_SW           GPIOB,10
#define DOWN_SW         GPIOC,2
#define LEFT_SW         GPIOC,3
#define ENTER_SW        GPIOB,12

#define HMI_SMCLK       UP_SW
#define HMI_SMDATA      RIGHT_SW
#define HMI_SMBA        ENTER_SW
#define HMI_AIN_HWVER   LEFT_SW

#define PTT_SW          GPIOC,13
#define PTT_OUT         GPIOD,2

/* Audio */
#define AUDIO_MIC       GPIOA,2
#define AUDIO_SPK       GPIOA,5
#define BASEBAND_RX     GPIOA,1
#define BASEBAND_TX     GPIOA,4
#define SPK_MUTE        GPIOB,1
#define MIC_MUTE        GPIOC,4
#define MIC_GAIN        GPIOC,5

#define AIN_HWVER       GPIOA,3
#define POWER_SW        GPIOA,15

/* I2C for MCP4551 */
#define I2C1_SDA        GPIOB,7
#define I2C1_SCL        GPIOB,6
#define SOFTPOT_RX      0x2E
#define SOFTPOT_TX      0x2F

#endif /* PINMAP_H */
