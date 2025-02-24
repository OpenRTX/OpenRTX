/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PINMAP_H
#define PINMAP_H

#include <at32f423.h>

// LCD Display
#define LCD_CLK GPIOB, 3
#define LCD_DAT GPIOB, 5
#define LCD_RST GPIOF, 6
#define LCD_CS GPIOA, 15
#define LCD_CD GPIOB, 4
#define LCD_BACKLIGHT GPIOA, 3

// LEDs
#define RED_LED GPIOA, 13
#define GREEN_LED GPIOA, 14

// Analog inputs
#define AIN_VBAT GPIOB, 2

// Push-to-talk
#define PTT_SW GPIOA, 12 // Shared with UART RX

// Keyboard and side keys
#define KBD_ROW1 GPIOF, 1
#define KBD_ROW2 GPIOA, 8
#define KBD_ROW3 GPIOA, 9
#define KBD_ROW4 GPIOB, 11
#define KBD_COL1 GPIOA, 0
#define KBD_COL2 GPIOA, 1
#define KBD_COL3 GPIOF, 0
#define KBD_COL4 GPIOB, 0

// External Flash
#define EFLASH_MISO GPIOB, 14
#define EFLASH_MOSI GPIOB, 15
#define EFLASH_SCK GPIOB, 13
#define EFLASH_CS GPIOB, 12

// BK1080
#define BK1080_SCK GPIOC, 13
#define BK1080_SDA GPIOC, 14

// BK4819
#define BK4819_CLK GPIOB, 9
#define BK4819_DAT GPIOB, 7
#define BK4819_CS GPIOB, 8

// Radio
#define FM_PWR_EN GPIOC, 15
#define AF_MUTE_4898 GPIOA, 2
#define TONe_AUDIO GPIOA, 4
#define TX_DTFM GPIOA, 5
#define DIG_RXD GPIOA, 6
#define DIG_TXD GPIOA, 7
#define DIG_PWR_EN GPIOB, 1
#define DIG_ANA_SW GPIOB, 10
#define DIG_ANA_AF_SW GPIOF, 8
#define UV_APCOUT_SW GPIOA, 10
#define DCS_DET GPIOB, 6

#define USART6_TX GPIOA, 11
#define USART6_RX GPIOA, 12 // Shared with PTT

#endif                      /* PINMAP_H */
