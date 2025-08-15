/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Frederik Saraci IU2NRO,                  *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include <stm32f4xx.h>
#include "pinmap.h"

#ifdef __cplusplus
extern "C" {
#endif

enum adcChannel
{
    ADC_VOL_CH   = 0,
    ADC_VBAT_CH  = 1,
    ADC_VOX_CH   = 3,
    ADC_RSSI_CH  = 8,
    ADC_SW1_CH   = 7,
    ADC_SW2_CH   = 6,
    ADC_RSSI2_CH = 9,
    ADC_HTEMP_CH = 15
};

extern const struct gpsDevice gps;
extern const struct spiDevice spi2;
extern const struct Adc adc1;

/* Device has a working real time clock */
#define CONFIG_RTC

/* Device supports an optional GPS chip */
#define CONFIG_GPS
#define CONFIG_GPS_STM32_USART1
#define CONFIG_NMEA_RBUF_SIZE 128

/* Screen dimensions */
#define CONFIG_SCREEN_WIDTH 128
#define CONFIG_SCREEN_HEIGHT 64

/* Screen has adjustable contrast */
#define CONFIG_SCREEN_CONTRAST
#define CONFIG_DEFAULT_CONTRAST 91

/* Screen has adjustable brightness */
#define CONFIG_SCREEN_BRIGHTNESS

/* Screen pixel format */
#define CONFIG_PIX_FMT_BW

/* Battery type */
#define CONFIG_BAT_NONE

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
