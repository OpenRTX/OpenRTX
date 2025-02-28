/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO,                            *
 *                         Andrej Antunovikj K8TUN                         *
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
#include <spi_gd32.h>
#include <hwconfig.h>
#include <spi_gd32.h>

static const struct spiConfig spiFlashCfg =
{
    .clk       = { FLASH_CLK },
    .mosi      = { FLASH_SDO },
    .miso      = { FLASH_SDI },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags     = 0
};

// Check with Andrej, is nvm not spi0 instead of bitbang?
// SPI_BITBANG_DEVICE_DEFINE(nvm_spi, spiFlashCfg, NULL)
//SPI_BITBANG_DEVICE_DEFINE(bk4819, spiFlashCfg, NULL)
SPI_GD32_DEVICE_DEFINE(nvm_spi0, SPI0, NULL)
SPI_GD32_DEVICE_DEFINE(st7735s_spi1, SPI1, NULL)
