/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include "stm32f4xx.h"
#include "pinmap.h"

#ifdef __cplusplus

// Export the HR_C6000 driver only for C++ sources
#include "drivers/baseband/HR_C6000.h"

extern HR_C6000 C6000;

extern "C" {
#endif

enum AdcChannels
{
    ADC_VOL_CH  = 15,  /* PC5  */
    ADC_VBAT_CH = 6,   /* PA6  */
    ADC_MIC_CH  = 3,   /* PA3  */
    ADC_RSSI_CH = 8,   /* PB0  */
    ADC_RTX_CH  = 7,   /* PA7  */
    ADC_CTCSS_CH = 2,  /* PA2  */
};

extern const struct Adc adc1;
extern const struct spiCustomDevice spiSr;
extern const struct spiCustomDevice flash_spi;
extern const struct spiCustomDevice det_spi;
extern const struct spiCustomDevice pll_spi;
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

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
