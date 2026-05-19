/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PINMAP_H
#define PINMAP_H

#include "stm32f4xx.h"

/* Power keep switch */
#define PWR_SW GPIOA,7

/* Display */
#define LCD_D0  GPIOD,14
#define LCD_D1  GPIOD,15
#define LCD_D2  GPIOD,0
#define LCD_D3  GPIOD,1
#define LCD_D4  GPIOE,7
#define LCD_D5  GPIOE,8
#define LCD_D6  GPIOE,9
#define LCD_D7  GPIOE,10
#define LCD_WR  GPIOD,5
#define LCD_RD  GPIOD,4
#define LCD_CS  GPIOD,6
#define LCD_RS  GPIOD,12
#define LCD_RST GPIOD,13
#define LCD_BKLIGHT GPIOC,6

/* Signalling LEDs */
#define GREEN_LED  GPIOE,0
#define RED_LED    GPIOE,1

/* Analog inputs */
#define AIN_VOLUME GPIOA,0
#define AIN_VBAT   GPIOA,1
#define AIN_MIC    GPIOA,3
#define AIN_RSSI   GPIOB,0
#define AIN_RTX    GPIOC,3

/* Channel selection rotary encoder */
#define CH_SELECTOR_0 GPIOE,14
#define CH_SELECTOR_1 GPIOE,15
#define CH_SELECTOR_2 GPIOB,10
#define CH_SELECTOR_3 GPIOB,11

/* Push-to-talk switch */
#define PTT_SW  GPIOE,11
#define PTT_EXT GPIOE,12

/*
 * Keyboard. Here we define only rows, since coloumn lines are the same as
 * LCD_Dx. See also: https://www.qsl.net/dl4yhf/RT3/md380_hw.html#keyboard
 *
 * "Monitor" and "Function" buttons, on the other hand, are connected to
 * keyboard row 3 and on LCD_D7 and LCD_D6. See also the schematic.
 */
#define KB_ROW1 GPIOA,6 /* K1 */
#define KB_ROW2 GPIOD,2 /* K2 */
#define KB_ROW3 GPIOD,3 /* K3 */
#define MONI_SW LCD_D7
#define FUNC_SW LCD_D6

/* Tone generator  */
#define CTCSS_OUT GPIOC,7   /* System "beep" */
#define BEEP_OUT  GPIOC,8   /* CTCSS tone    */

/* External flash */
#define FLASH_CS  &GpioD,7
#define FLASH_CLK GPIOB,3
#define FLASH_SDO GPIOB,4
#define FLASH_SDI GPIOB,5

/* PLL */
#define PLL_CS  &GpioD,11
#define PLL_CLK GPIOE,4
#define PLL_DAT GPIOE,5     /* WARNING: this line is also HR_C5000 MOSI */
#define PLL_LD  GPIOD,10

/* HR_C5000 */
#define DMR_CS    &GpioE,2
#define DMR_CLK   GPIOC,13
#define DMR_MOSI  PLL_DAT
#define DMR_MISO  GPIOE,3
#define DMR_SLEEP GPIOE,6
#define V_CS      GPIOB,12

/* RTX control */
#define PLL_PWR   GPIOA,8
#define VCOVCC_SW GPIOA,9
#define DMR_SW    GPIOA,10
#define FM_SW     GPIOB,2
#define WN_SW     GPIOA,13
#define RF_APC_SW GPIOC,4
#define TX_STG_EN GPIOC,5
#define RX_STG_EN GPIOC,9
#define APC_TV    GPIOA,4
#define MOD2_BIAS GPIOA,5

/* Audio control */
#define AUDIO_AMP_EN GPIOB,9
#define SPK_MUTE     GPIOB,8
#define FM_MUTE      GPIOE,13
#define MIC_PWR      GPIOA,14

/* GPS, for the devices who have it */
#define GPS_EN   GPIOD,8
#define GPS_DATA GPIOD,9

#endif /* PINMAP_H */
