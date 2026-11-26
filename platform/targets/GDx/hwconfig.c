/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/SPI/spi_bitbang.h"
#include "drivers/SPI/spi_mk22.h"
#include "hwconfig.h"

static const struct spiConfig spiFlashCfg =
{
    .clk       = { FLASH_CLK },
    .mosi      = { FLASH_SDO },
    .miso      = { FLASH_SDI },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags     = 0
};

BITBANG_SPI_BUS_DEFINE(nvm_spi, spiFlashCfg, NULL);
MK22_SPI_BUS_DEFINE(c6000_spi, SPI0, NULL);
