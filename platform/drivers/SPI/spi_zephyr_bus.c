/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include "spi_zephyr_bus.h"

int zephyrSpiBus_transfer(const struct device *bus, uint32_t frequency,
                          bool halfDuplex, const void *txBuf, void *rxBuf,
                          size_t size)
{
    const struct spi_config cfg = {
        .frequency = frequency,
        .operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8)
                   | (halfDuplex ? SPI_HALF_DUPLEX : 0),
    };

    struct spi_buf txSpiBuf = { .buf = (void *)txBuf, .len = size };
    struct spi_buf_set txBufSet = { .buffers = &txSpiBuf, .count = 1 };

    struct spi_buf rxSpiBuf = { .buf = rxBuf, .len = size };
    struct spi_buf_set rxBufSet = { .buffers = &rxSpiBuf, .count = 1 };

    int ret = spi_transceive(bus, &cfg, (txBuf != NULL) ? &txBufSet : NULL,
                             (rxBuf != NULL) ? &rxBufSet : NULL);

    return (ret < 0) ? ret : 0;
}

int zephyrSpiBus_isReady(const struct device *bus)
{
    return device_is_ready(bus) ? 0 : -ENODEV;
}
