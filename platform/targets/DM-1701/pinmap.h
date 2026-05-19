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
#define LCD_BKLIGHT GPIOD,8

/* Signalling LEDs */
#define GREEN_LED  GPIOE,0
#define RED_LED    GPIOE,1

/* Analog inputs */
#define AIN_VOLUME GPIOA,0
#define AIN_VBAT   GPIOA,1
#define AIN_MIC    GPIOA,3
#define AIN_RTX    GPIOC,3

/* Channel selection rotary encoder */
#define CH_SELECTOR_0 GPIOE,14
#define CH_SELECTOR_1 GPIOB,11

/* Push-to-talk switch */
#define PTT_SW  GPIOE,11
#define PTT_EXT GPIOE,12

#define KB_ROW1 GPIOA,6 /* K1 */
#define KB_ROW2 GPIOD,2 /* K2 */
#define KB_ROW3 GPIOD,3 /* K3 */
#define MONI_SW LCD_D6
#define FUNC_SW LCD_D7

/* Tone generator  */
#define CTCSS_OUT GPIOC,7   /* System "beep" */
#define BEEP_OUT  GPIOC,8   /* CTCSS tone    */

/* External flash */
#define FLASH_CS  &GpioD,7
#define FLASH_CLK GPIOB,3
#define FLASH_SDO GPIOB,4
#define FLASH_SDI GPIOB,5

/* Audio control */
#define AUDIO_AMP_EN GPIOB,9
#define SPK_MUTE     GPIOB,8
#define MIC_PWR      GPIOA,13

/* RTX stage control */
#define VHF_LNA_EN GPIOA,5
#define UHF_LNA_EN GPIOA,2
#define TX_PA_EN   GPIOC,5
#define RF_APC_SW  GPIOC,4
#define PA_SEL_SW  GPIOC,6
#define APC_REF    GPIOA,4

/* I2C for AT1846S */
#define I2C_SDA GPIOC,9
#define I2C_SCL GPIOA,8

/* HR_C6000 control interface */
#define DMR_SLEEP GPIOE,6
#define DMR_CS    &GpioE,2
#define DMR_CLK   GPIOE,3
#define DMR_MOSI  GPIOE,4
#define DMR_MISO  GPIOE,5

#endif /* PINMAP_H */
