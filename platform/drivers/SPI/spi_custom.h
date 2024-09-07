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

#ifndef SPI_CUSTOM_H
#define SPI_CUSTOM_H

#include <peripherals/spi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This module allows to create an SPI device with a custom implementation of
 * the bus transfer function. An example usage is to create an SPI device using
 * an high-performance bitbanging routine.
 */

/**
 * Transfer single byte on the SPI bus.
 *
 * @param dev: SPI device handle.
 * @param tx: byte to be sent.
 * @return byte received from remote device.
 */
typedef uint8_t (*spi_func)(const void *priv, uint8_t tx);

/**
 * Implementation of the SPI transfer function.
 *
 * @param dev: SPI device handle.
 * @param txBuf: pointer to TX buffer, can be NULL.
 * @param rxBuf: pointer to RX buffer, can be NULL.
 * @param size: number of bytes to transfer.
 * @return zero on success, a negative error code otherwise.
 */
int spiCustom_transfer(const struct spiDevice *dev, const void *txBuf,
                       void *rxBuf, const size_t size);

/**
 * Custom SPI device descriptor.
 */
struct spiCustomDevice
{
    spi_transfer_impl transfer; ///< Device-specific implementation of the SPI transfer function
    const void        *priv;    ///< Pointer to device driver private data
    pthread_mutex_t   *mutex;   ///< Pointer to mutex for exclusive access
    spi_func          spiFunc;  ///< Pointer to driver configuration data.
};

/**
 *  Instantiate a custom SPI device.
 *
 * @param name: device name.
 * @param impl: user-provided implementation of the single byte transfer function.
 * @param args: pointer to user-defined arguments.
 * @param mutx: pointer to mutex, or NULL.
 */
#define SPI_CUSTOM_DEVICE_DEFINE(name, impl, args, mutx) \
const struct spiCustomDevice name =                      \
{                                                        \
    .transfer = spiCustom_transfer,                      \
    .priv     = args,                                    \
    .mutex    = mutx,                                    \
    .spiFunc  = impl                                     \
};

#ifdef __cplusplus
}
#endif

#endif /* SPI_CUSTOM_H */
