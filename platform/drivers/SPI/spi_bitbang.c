/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#include <interfaces/delays.h>
#include "spi_bitbang.h"

typedef uint8_t (*sendRecv_impl)(struct spiConfig *cfg, uint8_t data);

static uint8_t sendRecv_clkRising(struct spiConfig *cfg, uint8_t data)
{
    uint8_t incoming = 0;

    for(uint8_t cnt = 0; cnt < 8; cnt++)
    {
        // Setup new output towards the peripheral
        if((data & 0x80) != 0)
            gpio_setPin(cfg->mosi.port, cfg->mosi.pin);
        else
            gpio_clearPin(cfg->mosi.port, cfg->mosi.pin);

        // Sample the current incoming bit from the peripheral
        data     <<= 1;
        incoming <<= 1;
        incoming |= gpio_readPin(cfg->miso.port, cfg->miso.pin);

        // One clock cycle, peripheral reads the new bit and updates its output
        // bit
        delayUs(cfg->clkPeriod);
        gpio_setPin(cfg->clk.port, cfg->clk.pin);
        delayUs(cfg->clkPeriod);
        gpio_clearPin(cfg->clk.port, cfg->clk.pin);
    }

    return incoming;
}

static uint8_t sendRecv_clkFalling(struct spiConfig *cfg, uint8_t data)
{
    uint8_t incoming = 0;

    for(uint8_t cnt = 0; cnt < 8; cnt++)
    {
        // Clock rising edge (inactive edge)
        gpio_setPin(cfg->clk.port, cfg->clk.pin);

        // Setup data output
        if((data & 0x80) != 0)
            gpio_setPin(cfg->mosi.port, cfg->mosi.pin);
        else
            gpio_clearPin(cfg->mosi.port, cfg->mosi.pin);

        // Sample data input
        data     <<= 1;
        incoming <<= 1;
        incoming |= gpio_readPin(cfg->miso.port, cfg->miso.pin);

        // Clock falling edge, peripheral reads new data and updates its output
        // line
        delayUs(cfg->clkPeriod);
        gpio_clearPin(cfg->clk.port, cfg->clk.pin);
        delayUs(cfg->clkPeriod);
    }

    return incoming;
}


int spiBitbang_init(const struct spiDevice* dev)
{
    struct spiConfig *cfg = (struct spiConfig *) dev->priv;

    // Setup MOSI and clock lines as output, low level
    gpio_setMode(cfg->clk.port,   cfg->clk.pin,  OUTPUT);
    gpio_setMode(cfg->mosi.port,  cfg->mosi.pin, OUTPUT);
    gpio_clearPin(cfg->mosi.port, cfg->mosi.pin);
    gpio_clearPin(cfg->clk.port,  cfg->clk.pin);

    // Set MISO line as input only for full-duplex operation
    if((cfg->flags & SPI_HALF_DUPLEX) == 0)
        gpio_setMode(cfg->miso.port, cfg->miso.pin, INPUT);

    // Set initial state of clock line to high if requested
    if((cfg->flags & SPI_FLAG_CPOL) != 0)
        gpio_clearPin(cfg->clk.port, cfg->clk.pin);

    if(dev->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *) dev->mutex, NULL);

    return 0;

}

void spiBitbang_terminate(const struct spiDevice* dev)
{
    struct spiConfig *cfg = (struct spiConfig *) dev->priv;

    // Set clock and MOSI back to Hi-Z state
    gpio_setMode(cfg->clk.port,  cfg->clk.pin,  INPUT);
    gpio_setMode(cfg->mosi.port, cfg->mosi.pin, INPUT);

    if(dev->mutex != NULL)
        pthread_mutex_destroy((pthread_mutex_t *) dev->mutex);
}

int spiBitbang_impl(const struct spiDevice *dev, const void *txBuf,
                    const size_t txSize, void *rxBuf, const size_t rxSize)
{
    struct spiConfig *cfg = (struct spiConfig *) dev->priv;
    const uint8_t *txData = (const uint8_t *) txBuf;
    uint8_t       *rxData = (uint8_t *) rxBuf;
    sendRecv_impl spi_sendRecv;

    if((cfg->flags & SPI_FLAG_CPHA) != 0)
        spi_sendRecv = &sendRecv_clkFalling;
    else
        spi_sendRecv = &sendRecv_clkRising;

    // Send only
    if((rxBuf == NULL) || (rxSize == 0))
    {
        for(size_t i = 0; i < txSize; i++)
            spi_sendRecv(cfg, txData[i]);

        return 0;
    }

    // Receive only
    if((txBuf == NULL) || (txSize == 0))
    {
        for(size_t i = 0; i < rxSize; i++)
            rxData[i] = spi_sendRecv(cfg, 0x00);

        return 0;
    }

    // Transmit and receive
    size_t txRxSize = (txSize < rxSize) ? txSize : rxSize;
    for(size_t i = 0; i < txRxSize; i++)
        rxData[i] = spi_sendRecv(cfg, txData[i]);

    // Still something to send?
    if(txSize > txRxSize)
    {
        for(size_t i = 0; i < (txSize - txRxSize); i++)
        {
            size_t pos = txRxSize + i;
            spi_sendRecv(cfg, txData[pos]);
        }
    }

    // Still something to receive?
    if(rxSize > txRxSize)
    {
        for(size_t i = 0; i < (rxSize - txRxSize); i++)
        {
            size_t pos = txRxSize + i;
            rxData[pos] = spi_sendRecv(cfg, 0x00);
        }
    }

    return 0;
}
