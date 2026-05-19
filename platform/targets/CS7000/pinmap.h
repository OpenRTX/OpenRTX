/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PINMAP_H
#define PINMAP_H

#include "stm32f4xx.h"

/* Power control */
#define BAT_DETECT    GPIOB,2
#define MAIN_PWR_DET  GPIOA,6
#define MAIN_PWR_SW   &extGpio,20

/* Display */
#define LCD_D0        GPIOD,0
#define LCD_D1        GPIOD,1
#define LCD_D2        GPIOD,2
#define LCD_D3        GPIOD,3
#define LCD_D4        GPIOD,4
#define LCD_D5        GPIOD,5
#define LCD_D6        GPIOD,6
#define LCD_D7        GPIOD,7
#define LCD_WR        GPIOD,13
#define LCD_RD        GPIOD,14
#define LCD_DC        GPIOD,12
#define LCD_RST       GPIOE,0
#define LCD_CS        GPIOE,1
#define LCD_BKLIGHT   GPIOC,9

/*
 * Keyboard. Rows and columns, except for column 5, are shared with the LCD
 * data lines. Commens reports the resistor number in the schematic for reference.
 */
#define KB_ROW1       LCD_D0    // R905
#define KB_ROW2       LCD_D1    // R906
#define KB_ROW3       LCD_D2    // R907
#define KB_ROW4       LCD_D3    // R908
#define KB_COL1       LCD_D7    // R902
#define KB_COL2       LCD_D6    // R903
#define KB_COL3       LCD_D5    // R904
#define KB_COL4       LCD_D4    // R909
#define KB_COL5       GPIOB,7   // R926
#define KBD_BKLIGHT   &extGpio,16

/* Push-to-talk and side keys */
#define PTT_SW        GPIOA,8
#define PTT_EXT       GPIOE,11
#define SIDE_KEY1     GPIOD,15
#define SIDE_KEY2     GPIOE,12
#define SIDE_KEY3     GPIOE,13
#define ALARM_KEY     GPIOE,10

/* Channel selection rotary encoder */
#define CH_SELECTOR_0 GPIOE,14
#define CH_SELECTOR_1 GPIOE,15
#define CH_SELECTOR_2 GPIOB,10
#define CH_SELECTOR_3 GPIOB,11

/* LEDs */
#define GREEN_LED     &extGpio,12
#define RED_LED       &extGpio,13

/* Analog inputs */
#define AIN_VOLUME    GPIOC,5
#define AIN_VBAT      GPIOC,4   // BATT
#define AIN_MIC       GPIOA,3   // VOX
#define AIN_RSSI      GPIOB,0
#define AIN_NOISE     GPIOB,1
#define AIN_RTX       GPIOA,7   // 2T/5T
#define AIN_CTCSS     GPIOA,2   // QT_DQT_IN
#define AIN_TEMP      GPIOA,7   // Batt. temp.

/* Tone generator */
#define CTCSS_OUT     GPIOC,8   // CTCSS tone
#define BEEP_OUT      GPIOA,5   // System "beep"

/* External flash */
#define FLASH_CS      &GpioE,2
#define FLASH_CLK     GPIOE,3
#define FLASH_SDO     GPIOE,4
#define FLASH_SDI     GPIOE,5

/* PLL */
#define PLL_CS        &GpioD,9
#define PLL_CLK       GPIOD,10
#define PLL_DAT       GPIOD,8
#define PLL_LD        GPIOD,11

/* HR_C6000 */
#define C6K_CS        &GpioB,12
#define C6K_CLK       GPIOB,13
#define C6K_MISO      GPIOB,14
#define C6K_MOSI      GPIOB,15
#define C6K_SLEEP     &extGpio,7

#define VOC_CS        GPIOB,6
#define VOC_CLK       GPIOB,3
#define VOC_MOSI      GPIOB,4
#define VOC_MISO      GPIOB,5

#define I2S_FS        GPIOA,15
#define I2S_CLK       GPIOC,10
#define I2S_RX        GPIOC,11
#define I2S_TX        GPIOC,12

#define DMR_TS_INT    GPIOC,0
#define DMR_SYS_INT   GPIOC,1
#define DMR_TX_INT    GPIOC,2

/* AK2365 */
#define DET_PDN       &extGpio,6
#define DET_CS        &extGpio,0
#define DET_CLK       GPIOE,6
#define DET_DAT       GPIOC,13
#define DET_RST       &GpioC,14

/* RTX control */
#define RF_APC_SW     &extGpio,3
#define VCOVCC_SW     &extGpio,8
#define TX_PWR_EN     &extGpio,9
#define RX_PWR_EN     &extGpio,15
#define VCO_PWR_EN    &extGpio,11
#define CTCSS_AMP_EN  &extGpio,22
#define APC_TV        GPIOA,4

/* Audio control */
#define AUDIO_AMP_EN  &extGpio,14
#define INT_SPK_MUTE  &extGpio,17
#define EXT_SPK_MUTE  &extGpio,19
#define MIC_PWR_EN    &extGpio,18
#define INT_MIC_SEL   &extGpio,10
#define EXT_MIC_SEL   &extGpio,5
#define AF_MUTE       &extGpio,23
#define PHONE_DETECT  GPIOA,13

/* GPS */
#define GPS_TXD       GPIOC,6
#define GPS_RXD       GPIOC,7

/* Accessory connector */
#define PHONE_TXD     GPIOA,0
#define PHONE_RXD     GPIOA,1

/* SN74HC595 gpio extenders */
#define GPIOEXT_CLK   GPIOE,7
#define GPIOEXT_DAT   GPIOE,9
#define GPIOEXT_STR   &GpioE,8

/* Bluetooth module */
#define BLTH_DETECT   GPIOA,14
#define BLTH_PWR_EN   &extGpio,4
#define BLTH_RXD      GPIOA,9
#define BLTH_TXD      GPIOA,10

/* ALPU-MP */
#define ALPU_SDA      GPIOB,9
#define ALPU_SCL      GPIOB,8

#endif /* PINMAP_H */
