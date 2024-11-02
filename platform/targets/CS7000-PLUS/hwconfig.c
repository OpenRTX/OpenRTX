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

#include <gpio_shiftReg.h>
#include <spi_bitbang.h>
#include <spi_stm32.h>
#include <adc_stm32.h>
#include <hwconfig.h>
#include <pthread.h>
#include <SKY72310.h>
#include <AK2365A.h>

static const struct spiConfig spiSrConfig =
{
    .clk  = {GPIOEXT_CLK},
    .miso = {GPIOEXT_DAT},
    .mosi = {GPIOEXT_DAT},
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags = SPI_HALF_DUPLEX
};

static const struct spiConfig spiDetCfg =
{
    .clk       = { DET_CLK },
    .mosi      = { DET_DAT },
    .miso      = { DET_DAT },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags     = SPI_HALF_DUPLEX
};

static const struct spiConfig spiPllCfg =
{
    .clk       = { PLL_CLK },
    .mosi      = { PLL_DAT },
    .miso      = { PLL_DAT },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags     = SPI_HALF_DUPLEX
};

static const struct gpioPin shiftRegStrobe = { GPIOEXT_STR };
static pthread_mutex_t adc1Mutex;
static pthread_mutex_t c6000_mutex;

SPI_BITBANG_DEVICE_DEFINE(spiSr, spiSrConfig, NULL)
SPI_BITBANG_DEVICE_DEFINE(det_spi, spiDetCfg, NULL)
SPI_BITBANG_DEVICE_DEFINE(pll_spi, spiPllCfg, NULL)
SPI_STM32_DEVICE_DEFINE(flash_spi, SPI4, NULL)
SPI_STM32_DEVICE_DEFINE(c6000_spi, SPI2, &c6000_mutex)
GPIO_SHIFTREG_DEVICE_DEFINE(extGpio, (const struct spiDevice *) &spiSr, shiftRegStrobe, 24)
ADC_STM32_DEVICE_DEFINE(adc1, ADC1, &adc1Mutex, ADC_COUNTS_TO_UV(3300000, 16))

const struct ak2365a detector =
{
    .spi = (const struct spiDevice *) &det_spi,
    .cs  = { DET_CS  },
    .res = { DET_RST }
};

const struct sky73210 pll =
{
    .spi    = (const struct spiDevice *) &pll_spi,
    .cs     = { PLL_CS },
    .refClk = 16800000
};
