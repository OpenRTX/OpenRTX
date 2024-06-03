/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
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

enum AdcChannels
{
    ADC_VOL_CH  = 15,  /* PC5  */
    ADC_VBAT_CH = 6,   /* PA6  */
    ADC_MIC_CH  = 3,   /* PA3  */
    ADC_RSSI_CH = 8,   /* PB0  */
    ADC_RTX_CH  = 7,   /* PA7  */
};

extern const struct Adc adc1;
extern const struct spiCustomDevice spiSr;
extern const struct spiCustomDevice flash_spi;
extern const struct gpioDev extGpio;

/* Screen dimensions */
#define CONFIG_SCREEN_WIDTH 160
#define CONFIG_SCREEN_HEIGHT 128

/* Screen pixel format */
#define CONFIG_PIX_FMT_RGB565

/* Screen has adjustable brightness */
#define CONFIG_SCREEN_BRIGHTNESS

/* Battery type */
#define CONFIG_BAT_LIPO_2S

/* Device supports M17 mode */
#define CONFIG_M17

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
