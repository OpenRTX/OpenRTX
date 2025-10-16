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
extern "C" {
#endif

enum adcChannel
{
    ADC_VOL_CH   = 0,
    ADC_VBAT_CH  = 1,
    ADC_VOX_CH   = 3,
    ADC_RSSI_CH  = 8
};

extern const struct gpsDevice gps;
extern const struct spiDevice nvm_spi;
extern const struct spiCustomDevice pll_spi;
extern const struct spiCustomDevice c5000_spi;
extern const struct sky73210 pll;
extern const struct Adc adc1;

/* Device has a working real time clock */
#define CONFIG_RTC

/* Device supports an optional GPS chip */
#define CONFIG_GPS
#define CONFIG_GPS_STM32_USART3
#define CONFIG_NMEA_RBUF_SIZE 128

/* Device has a channel selection knob */
#define CONFIG_KNOB_ABSOLUTE

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

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
