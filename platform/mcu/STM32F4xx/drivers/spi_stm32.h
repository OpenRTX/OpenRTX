/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#ifndef SPI_STM32_H
#define SPI_STM32_H

#include <peripherals/spi.h>

/**
 *  Instantiate an STM32 I2C master device.
 *
 * @param name: device name.
 * @param peripheral: underlying MCU peripheral.
 * @param mutx: pointer to mutex, or NULL.
 */
#define SPI_STM32_DEVICE_DEFINE(name, peripheral, mutx)                      \
extern int spiStm32_transfer(const struct spiDevice *dev, const void *txBuf, \
                             const size_t txSize, void *rxBuf,               \
                             const size_t rxSize);                           \
const struct spiDevice name =                                                \
{                                                                            \
    .transfer = &spiStm32_transfer,                                          \
    .priv     = peripheral,                                                  \
    .mutex    = mutx                                                         \
};

/**
 * Initialise an SPI peripheral and driver.
 * Is left to application code to change the operating mode and alternate function
 * mapping of the corresponding gpio lines.
 *
 * @param dev: SPI device descriptor.
 * @param speed: SPI clock speed.
 * @param flags: SPI configuration flags.
 * @return zero on success, a negative error code otherwise.
 */
int spi_init(const struct spiDevice *dev, const uint32_t speed, const uint8_t flags);

/**
 * Shut down an SPI peripheral and driver.
 *
 * @param dev: SPI device descriptor.
 */
void spi_terminate(const struct spiDevice *dev);

#endif /* SPI_STM32_H */
