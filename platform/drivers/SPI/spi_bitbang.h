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

#ifndef SPI_BITBANG_H
#define SPI_BITBANG_H

#include <peripherals/gpio.h>
#include <peripherals/spi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCK_PERIOD_FROM_FREQ(x) (1000000/x)

/**
 * Data structure collecting the configuration data for the SPI bitbang driver.
 *
 * The driver uses the MCU native gpio driver for two reasons:
 * 1) the faster the gpios are driver, the faster the SPI bitbang can work
 * 2) using an external port expander to do SPI bitbang does not make sense...
 */
struct spiConfig
{
    const struct gpio clk;          ///< SPI clock
    const struct gpio mosi;         ///< SPI data from MCU to peripherals
    const struct gpio miso;         ///< SPI data from peripherals to MCU
    const uint32_t    clkPeriod;    ///< Clock period, in us
    const uint8_t     flags;        ///< SPI configuration flags
};

/**
 *  Instantiate an SPI bitbang device.
 *
 * @param name: device name.
 * @param mutx: pointer to mutex, or NULL.
 * @param cfg: driver configuration data.
 */
#define SPI_BITBANG_DEVICE_DEFINE(name, mutx, cfg)                          \
int spiBitbang_impl(const struct spiDevice *dev, const void *txBuf,         \
                    const size_t txSize, void *rxBuf, const size_t rxSize); \
const struct spiDevice name =                                               \
{                                                                           \
    .transfer = spiBitbang_impl,                                            \
    .priv     = &cfg,                                                       \
    .mutex    = mutx,                                                       \
};

/**
 * Initialise a bitbang SPI driver.
 * Is left to application code to change the operating mode and alternate function
 * mapping of the corresponding gpio lines.
 *
 * @param dev: SPI bitbang device descriptor.
 * @param speed: SPI clock speed.
 * @param flags: SPI configuration flags.
 * @return zero on success, a negative error code otherwise.
 */
int spiBitbang_init(const struct spiDevice *dev);

/**
 * Shut down a bitbang SPI driver.
 *
 * @param dev: SPI bitbang device descriptor.
 */
void spiBitbang_terminate(const struct spiDevice *dev);


#ifdef __cplusplus
}
#endif

#endif /* SPI_BITBANG_H */
