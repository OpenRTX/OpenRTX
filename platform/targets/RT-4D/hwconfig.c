/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <drivers/ADC/adc_at32.h>
#include "drivers/SPI/spi_bitbang.h"
#include <hwconfig.h>
#include <pthread.h>

static pthread_mutex_t adcMutex;

const struct spiConfig spi_display_config = {
    .clk = { LCD_CLK },
    .mosi = { LCD_DAT },
    .miso = { LCD_DAT },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags = SPI_HALF_DUPLEX,
};
const struct spiConfig spi_flash_config = {
    .clk = { EFLASH_SCK },
    .mosi = { EFLASH_MOSI },
    .miso = { EFLASH_MISO },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags = 0,
};

SPI_BITBANG_DEVICE_DEFINE(display_spi, spi_display_config, NULL)
SPI_BITBANG_DEVICE_DEFINE(flash_spi, spi_flash_config, NULL)
ADC_AT32_DEVICE_DEFINE(adc1, ADC1, &adcMutex, 3300000)
