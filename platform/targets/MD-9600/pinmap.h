/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PINMAP_H
#define PINMAP_H

#include "stm32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Power keep switch */
#define PWR_SW GPIOD,15

/* Display */
#define LCD_BKLIGHT GPIOC,6
#define LCD_RST     GPIOD,12
#define LCD_RS      GPIOD,13
#define LCD_CS      GPIOD,14

/* Analog inputs */
#define AIN_VOLUME GPIOA,0
#define AIN_VBAT   GPIOA,1
#define AIN_MIC    GPIOA,3
#define AIN_SW2    GPIOA,6
#define AIN_SW1    GPIOA,7
#define AIN_RSSI   GPIOB,0
#define AIN_RSSI2  GPIOB,1
#define AIN_HTEMP  GPIOC,5

/* Channel selection rotary encoder */
#define CH_SELECTOR_0 GPIOB,10
#define CH_SELECTOR_1 GPIOB,11

/* Push-to-talk switch */
#define PTT_SW GPIOE,10

/* Keyboard */
#define KB_ROW1 GPIOD,2
#define KB_ROW2 GPIOD,3
#define KB_ROW3 GPIOD,4
#define KB_COL1 GPIOD,0
#define KB_COL2 GPIOD,1
#define KB_COL3 GPIOE,0
#define KB_COL4 GPIOE,1

/* Tone generator  */
#define CTCSS_OUT GPIOC,7   /* System "beep" */
#define BEEP_OUT  GPIOC,8   /* CTCSS tone    */

/* SPI2, connected to external flash and LCD */
#define FLASH_CS &GpioB,12
#define SPI2_CLK GPIOB,13
#define SPI2_SDO GPIOB,14
#define SPI2_SDI GPIOB,15

/* Audio control */
#define SPK_MUTE GPIOB,6

/* GPS, for the devices who have it */
#define GPS_EN   GPIOA,9
#define GPS_DATA GPIOA,10

/* HR_C6000 control interface */
#define DMR_CS    GPIOE,2
#define DMR_CLK   GPIOE,3
#define DMR_MOSI  GPIOE,4
#define DMR_MISO  GPIOE,5

#ifdef __cplusplus
}
#endif

#endif /* PINMAP_H */
