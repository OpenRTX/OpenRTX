/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO                             *
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

#include <spi_bitbang.h>
#include <spi_custom.h>
#include <spi_stm32.h>
#include <adc_stm32.h>
#include <SKY72310.h>
#include <hwconfig.h>
#include <pinmap.h>

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
