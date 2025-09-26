/***************************************************************************
 * Copyright (C) 2024 - 2025 by Federico Amedeo Izzo IU2NUO,               *
 *                              Niccolò Izzo IU2KIN                        *
 *                              Frederik Saraci IU2NRO                     *
 *                              Silvano Seva IU2KWO                        *
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

#include "peripherals/gpio.h"
#include "hwconfig.h"
#include "drivers/SPI/spi_stm32.h"
#include "drivers/ADC/adc_stm32.h"
#include "drivers/GPS/gps_stm32.h"
#include <pthread.h>
#include "core/gps.h"

static void gpsEnable(void *priv)
{
    gpio_setPin(GPS_EN);
    gpsStm32_enable(priv);
}

static void gpsDisable(void *priv)
{
    gpio_clearPin(GPS_EN);
    gpsStm32_disable(priv);
}

static pthread_mutex_t spi2Mutex;
static pthread_mutex_t adcMutex;

SPI_STM32_DEVICE_DEFINE(spi2, SPI2, &spi2Mutex)
ADC_STM32_DEVICE_DEFINE(adc1, ADC1, &adcMutex, ADC_COUNTS_TO_UV(3300000, 12))

const struct gpsDevice gps =
{
    .priv = NULL,
    .enable = gpsEnable,
    .disable = gpsDisable,
    .getSentence = gpsStm32_getNmeaSentence
};
