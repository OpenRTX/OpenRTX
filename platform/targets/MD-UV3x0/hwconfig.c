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
#include <hwconfig.h>
#include <pinmap.h>
#include <spi_stm32.h>

const struct spiConfig spiC6000Cfg =
{
    .clk       = { DMR_CLK  },
    .mosi      = { DMR_MOSI },
    .miso      = { DMR_MISO },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags     = SPI_FLAG_CPHA
};

SPI_BITBANG_DEVICE_DEFINE(c6000_spi, NULL, spiC6000Cfg)
SPI_STM32_DEVICE_DEFINE(nvm_spi, SPI1, NULL)
