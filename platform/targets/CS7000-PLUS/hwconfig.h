/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include "stm32h7xx.h"
#include "pinmap.h"

#ifdef __cplusplus

// Export the HR_C6000 driver only for C++ sources
#include "drivers/baseband/HR_C6000.h"

extern HR_C6000 C6000;

extern "C" {
#endif

enum AdcChannels
{
    ADC_VOL_CH   = 8,   /* PC5  */
    ADC_VBAT_CH  = 3,   /* PA6  */
    ADC_RTX_CH   = 15,  /* PA3  */
    ADC_RSSI_CH  = 9,   /* PB0  */
    ADC_MIC_CH   = 7,   /* PA7  */
    ADC_CTCSS_CH = 2,   /* PA2  */
    ADC_VOX_CH   = 4,   /* PC4  */
};

extern const struct Adc adc1;
extern const struct spiCustomDevice spiSr;
extern const struct spiCustomDevice det_spi;
extern const struct spiCustomDevice pll_spi;
extern const struct spiDevice flash_spi;
extern const struct spiDevice c6000_spi;
extern const struct gpioDev extGpio;
extern const struct ak2365a detector;
extern const struct sky73210 pll;
extern const struct gpsDevice gps;

/* Screen dimensions */
#define CONFIG_SCREEN_WIDTH 160
#define CONFIG_SCREEN_HEIGHT 128

/* Screen pixel format */
#define CONFIG_PIX_FMT_RGB565

/* Screen has adjustable brightness */
#define CONFIG_SCREEN_BRIGHTNESS

/* Battery type */
#define CONFIG_BAT_LIION
#define CONFIG_BAT_NCELLS 2

/* Device supports M17 mode */
#define CONFIG_M17

/* Device has a GPS chip */
#define CONFIG_GPS
#define CONFIG_GPS_STM32_USART6
#define CONFIG_NMEA_RBUF_SIZE 128

/* Device supports USB CDC serial */
#define CONFIG_USB_SERIAL

/* Miosix sets NVIC_SetPriorityGrouping(7), disabling interrupt nesting.
 * In the resulting flat 0-15 scheme, 14 is near-lowest priority,
 * consistent with other peripheral drivers. */
#define USB_OTG_FS_IRQ_PRIORITY  14

/* tinyUSB root-hub port index. USB OTG FS is port 0 on all STM32H7 targets. */
#define USB_SERIAL_RHPORT        0

/* Bytes in the software TX ring buffer. Must be a power of two so that the
 * modulo arithmetic reduces to a bitmask at -O2. */
#define USB_SERIAL_TX_BUF_SIZE   512

/* Use extended addressing for external flash memory */
#define CONFIG_W25Qx_EXT_ADDR

/* Device has a vibration motor */
#define CONFIG_VIBRATION

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
