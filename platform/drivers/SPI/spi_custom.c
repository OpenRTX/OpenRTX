/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include "spi_custom.h"

static int transfer_impl(const struct spiBus *bus, const void *txBuf,
                         void *rxBuf, const size_t size)
{
    const struct spiBusCustom *spiDev = to_custom_spi(bus);
    const uint8_t *txData = (const uint8_t *)txBuf;
    uint8_t *rxData = (uint8_t *)rxBuf;

    if (spiDev->spi_func == NULL)
        return -EIO;

    for (size_t i = 0; i < size; i++)
        rxData[i] = spiDev->spi_func(bus->priv, txData[i]);

    return 0;
}

static int send_impl(const struct spiBus *bus, const void *txBuf,
                     const size_t size)
{
    const struct spiBusCustom *spiDev = to_custom_spi(bus);
    const uint8_t *txData = (const uint8_t *)txBuf;

    if (spiDev->spi_func == NULL)
        return -EIO;

    for (size_t i = 0; i < size; i++)
        spiDev->spi_func(bus->priv, txData[i]);

    return 0;
}

static int receive_impl(const struct spiBus *bus, void *rxBuf,
                        const size_t size)
{
    const struct spiBusCustom *spiDev = to_custom_spi(bus);
    uint8_t *rxData = (uint8_t *)rxBuf;

    if (spiDev->spi_func == NULL)
        return -EIO;

    for (size_t i = 0; i < size; i++)
        rxData[i] = spiDev->spi_func(bus->priv, 0x00);

    return 0;
}

const struct spiOps custom_spi_ops = {
    .transfer = transfer_impl,
    .send = send_impl,
    .receive = receive_impl,
};
