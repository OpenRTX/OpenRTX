/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "peripherals/gpio.h"
#include "drivers/SPI/spi_bitbang.h"
#include "drivers/SPI/spi_custom.h"
#include "drivers/SPI/spi_stm32.h"
#include "drivers/ADC/adc_stm32.h"
#include "drivers/GPS/gps_stm32.h"
#include "drivers/baseband/SKY72310.h"
#include "hwconfig.h"
#include "pinmap.h"
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

const struct spiConfig spiPllCfg =
{
    .clk       = { PLL_CLK },
    .mosi      = { PLL_DAT },
    .miso      = { PLL_DAT },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags     = SPI_HALF_DUPLEX
};

const struct spiConfig spiC5000Cfg =
{
    .clk       = { DMR_CLK  },
    .mosi      = { DMR_MOSI },
    .miso      = { DMR_MISO },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags     = SPI_FLAG_CPHA
};

static pthread_mutex_t adc1Mutex;

SPI_STM32_DEVICE_DEFINE(nvm_spi, SPI1, NULL)
SPI_BITBANG_DEVICE_DEFINE(pll_spi, spiPllCfg, NULL)
SPI_BITBANG_DEVICE_DEFINE(c5000_spi, spiC5000Cfg, NULL)
ADC_STM32_DEVICE_DEFINE(adc1, ADC1, &adc1Mutex, ADC_COUNTS_TO_UV(3300000, 12))

const struct sky73210 pll =
{
    .spi    = (const struct spiDevice *) &pll_spi,
    .cs     = { PLL_CS },
    .refClk = 16800000
};

const struct gpsDevice gps =
{
    .priv = NULL,
    .enable = gpsEnable,
    .disable = gpsDisable,
    .getSentence = gpsStm32_getNmeaSentence
};
