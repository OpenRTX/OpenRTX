/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "spi_zephyr.h"

int spiZephyr_transfer(const struct spiDevice *dev, const void *txBuf,
                       void *rxBuf, const size_t size)
{
    const struct spiZephyrCfg *cfg = (const struct spiZephyrCfg *)dev->priv;
    const bool halfDuplex = (cfg->flags & SPI_HALF_DUPLEX) != 0;

    return zephyrSpiBus_transfer(cfg->bus, cfg->frequency, halfDuplex, txBuf,
                                 rxBuf, size);
}

int spiZephyr_init(const struct spiDevice *dev)
{
    const struct spiZephyrCfg *cfg = (const struct spiZephyrCfg *)dev->priv;

    spi_init(dev);

    return zephyrSpiBus_isReady(cfg->bus);
}

void spiZephyr_terminate(const struct spiDevice *dev)
{
    spi_terminate(dev);
}
