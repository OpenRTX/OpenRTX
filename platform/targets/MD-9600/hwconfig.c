/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2020-2025 OpenRTX Contributors
 *
 * This file is part of OpenRTX.
 */

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
