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

uint8_t spiBitbang_sendRecv(const void *priv, uint8_t data)
{
    const struct spiConfig *cfg = (const struct spiConfig *) priv;
    uint8_t incoming = 0;

    for(uint8_t cnt = 0; cnt < 8; cnt++)
    {
        // Clock inactive edge
        if((cfg->flags & SPI_FLAG_CPHA) != 0)
            gpio_setPin(cfg->clk.port, cfg->clk.pin);
        else
            gpio_clearPin(cfg->clk.port, cfg->clk.pin);

        // Setup data output
        if((data & 0x80) != 0)
            gpio_setPin(cfg->mosi.port, cfg->mosi.pin);
        else
            gpio_clearPin(cfg->mosi.port, cfg->mosi.pin);

        // Sample data input
        data     <<= 1;
        incoming <<= 1;
        incoming |= gpio_readPin(cfg->miso.port, cfg->miso.pin);

        delayUs(cfg->clkPeriod);

        // Clock active edge
        if((cfg->flags & SPI_FLAG_CPHA) != 0)
            gpio_clearPin(cfg->clk.port, cfg->clk.pin);
        else
            gpio_setPin(cfg->clk.port, cfg->clk.pin);

        delayUs(cfg->clkPeriod);
    }

    return incoming;
}


int spiBitbang_init(const struct spiCustomDevice *dev)
{
    const struct spiConfig *cfg = (const struct spiConfig *) dev->priv;

    // Basic driver init
    spi_init((const struct spiDevice *) dev);

    // Setup MOSI and clock lines as output, low level
    gpio_setMode(cfg->clk.port,   cfg->clk.pin,  OUTPUT);
    gpio_setMode(cfg->mosi.port,  cfg->mosi.pin, OUTPUT);
    gpio_clearPin(cfg->mosi.port, cfg->mosi.pin);
    gpio_clearPin(cfg->clk.port,  cfg->clk.pin);

    // Set MISO line as input only for full-duplex operation
    if((cfg->flags & SPI_HALF_DUPLEX) == 0)
        gpio_setMode(cfg->miso.port, cfg->miso.pin, INPUT);

    return 0;
}

void spiBitbang_terminate(const struct spiCustomDevice* dev)
{
    const struct spiConfig *cfg = (const struct spiConfig *) dev->priv;

    // Set clock and MOSI back to Hi-Z state
    gpio_setMode(cfg->clk.port,  cfg->clk.pin,  INPUT);
    gpio_setMode(cfg->mosi.port, cfg->mosi.pin, INPUT);

    // Final driver cleanup
    spi_terminate((const struct spiDevice *) dev);
}
