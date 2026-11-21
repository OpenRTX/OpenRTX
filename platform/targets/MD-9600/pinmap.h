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
#define AIN_VBAT    GPIOA,1
#define AIN_MIC     GPIOA,3
#define AIN_SW2     GPIOA,6
#define AIN_SW1     GPIOA,7
#define AIN_RSSI_U  GPIOB,0
#define AIN_RSSI_V  GPIOA,0
#define AIN_NOISE_U GPIOB,1
#define AIN_NOISE_V GPIOC,5
/*
 * Channel selection rotary encoder, in the order that makes the position
 * count up when turned clockwise, as on the MD-UV3x0
 */
#define CH_SELECTOR_0 GPIOB,11
#define CH_SELECTOR_1 GPIOB,10

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
#define SPK_MUTE  GPIOB,6
#define U_V_AF_SW GPIOC,13
#define FM_MUTE   GPIOE,7
#define FM_SW     GPIOB,2

/* GPS, for the devices who have it */
#define GPS_EN   GPIOA,9
#define GPS_DATA GPIOA,10

/* HR_C6000 control interface */
#define DMR_CS    GPIOE,2
#define DMR_CLK   GPIOE,3
#define DMR_MISO  GPIOE,4
#define DMR_MOSI  GPIOE,5
#define DMR_PWD   GPIOE,6

/* HRC600 voice interface */
#define VOICE_CS  GPIOD,7
#define VOICE_CLK GPIOB,3
#define VOICE_SDO GPIOB,4
#define VOICE_SDI GPIOB,5

/* Radio Interface */
#define U_V_MOD2_SW GPIOB,7
#define RX_5V_V_SW  GPIOA,14
#define RX_5V_U_SW  GPIOA,13
#define TX_5V_V_SW  GPIOD,5
#define TX_5V_U_SW  GPIOD,6

#define C5_V_SW     GPIOC,9
#define C5_U_SW     GPIOA,8

#define VCO_U_SW    GPIOE,8
#define VCO_V_SW    GPIOE,9

/* PLL */
#define PLL_V_CS  GPIOD,8
#define PLL_U_CS  GPIOD,9
#define PLL_DATA  GPIOD,10
#define PLL_CLK   GPIOD,11

/* IF */
#define IF_V_CS   GPIOE,11
#define IF_U_CS   GPIOE,12
#define IF_DATA   GPIOE,13
#define IF_CLK    GPIOE,14
#define IF_RESET  GPIOE,15

/* Other */
#define RF_APC_SW GPIOC,4
#define APC_TV_V  GPIOA,5
#define TV_U      GPIOA,4
#define QT_SQT_IN GPIOA,2
#define RX_2T_5T  GPIOC,3

/* Interrupt Pins */
#define TIME_SLOT_INTERRUPT GPIOC,0
#define SYS_INTERRUPT       GPIOC,1
#define RF_TX_INTERRUPT     GPIOC,2

#ifdef __cplusplus
}
#endif

#endif /* PINMAP_H */
