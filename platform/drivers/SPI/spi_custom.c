/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include "spi_custom.h"

int spiCustom_transfer(const struct spiDevice *dev, const void *txBuf,
                       void *rxBuf, const size_t size)
{
    const struct spiCustomDevice *spiDev = (const struct spiCustomDevice *) dev;
    const uint8_t *txData = (const uint8_t *) txBuf;
    uint8_t       *rxData = (uint8_t *) rxBuf;

    if(spiDev->spiFunc == NULL)
        return -EIO;

    // Send only
    if(rxBuf == NULL)
    {
        for(size_t i = 0; i < size; i++)
            spiDev->spiFunc(dev->priv, txData[i]);

        return 0;
    }

    // Receive only
    if(txBuf == NULL)
    {
        for(size_t i = 0; i < size; i++)
            rxData[i] = spiDev->spiFunc(dev->priv, 0x00);

        return 0;
    }

    // Transmit and receive, only symmetric transfer is supported
    for(size_t i = 0; i < size; i++)
        rxData[i] = spiDev->spiFunc(dev->priv, txData[i]);

    return 0;
}
