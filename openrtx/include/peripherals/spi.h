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

#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumeration type for SPI configuration flags
 */
enum SPIFlags
{
    SPI_FLAG_CPOL   = 1,  ///< Clock line is high when idle.
    SPI_FLAG_CPHA   = 2,  ///< Data lines are sampled on clock falling edge.
    SPI_LSB_FIRST   = 4,  ///< Transfer data LSB first.
    SPI_HALF_DUPLEX = 8   ///< Half-duplex data transfer mode, on a single data line
};

struct spiDevice;

/**
 * Transfer data on the SPI bus.
 *
 * @param dev: SPI device handle.
 * @param txBuf: pointer to TX buffer, can be NULL.
 * @param txSize: number of bytes to send.
 * @param rxBuf: pointer to RX buffer, can be NULL.
 * @param rxSize: number of bytes to receive.
 * @return zero on success, a negative error code otherwise.
 */
typedef int (*spi_transfer_impl)(const struct spiDevice *dev, const void *txBuf,
                                 const size_t txSize, void *rxBuf, const size_t rxSize);

/**
 * SPI peripheral descriptor.
 */
struct spiDevice
{
    const spi_transfer_impl transfer; ///< Device-specific implementation of the SPI transfer function
    const void              *priv;    ///< Pointer to device driver private data
    const pthread_mutex_t   *mutex;   ///< Pointer to mutex for exclusive access
};


/**
 * Acquire exclusive ownership on an SPI peripheral.
 *
 * @param dev: pointer to peripheral descriptor.
 * @return zero on success, a negative error code on failure.
 */
static inline int spi_acquire(const struct spiDevice *dev)
{
    if(dev->mutex == NULL)
        return 0;

    return pthread_mutex_lock((pthread_mutex_t *) dev->mutex);
}

/**
 * Attempt to acquire exclusive ownership on an SPI peripheral.
 *
 * @param dev: pointer to peripheral descriptor.
 * @return zero on success, a negative error code on failure.
 */
static inline int spi_tryAcquire(const struct spiDevice *dev)
{
    if(dev->mutex == NULL)
        return 0;

    return pthread_mutex_trylock((pthread_mutex_t *) dev->mutex);
}

/**
 * Release exclusive ownership on an SPI peripheral.
 *
 * @param dev: pointer to peripheral descriptor.
 * @return zero on success, a negative error code on failure.
 */
static inline int spi_release(const struct spiDevice *dev)
{
    if(dev->mutex == NULL)
        return 0;

    return pthread_mutex_unlock((pthread_mutex_t *) dev->mutex);
}

/**
 * Transfer data on the SPI bus.
 *
 * @param txBuf: pointer to TX buffer, can be NULL.
 * @param txSize: number of bytes to send.
 * @param rxBuf: pointer to RX buffer, can be NULL.
 * @param rxSize: number of bytes to receive.
 * @return zero on success, a negative error code otherwise.
 */
static inline int spi_transfer(const struct spiDevice *dev, const void *txBuf,
                               const size_t txSize, void *rxBuf, const size_t rxSize)
{
    return dev->transfer(dev, txBuf, txSize, rxBuf, rxSize);
}

/**
 * Send data on the SPI bus.
 *
 * @param txBuf: pointer to TX buffer.
 * @param txSize: number of bytes to send.
 * @return zero on success, a negative error code otherwise.
 */
static inline int spi_send(const struct spiDevice *dev, const void *txBuf,
                           const size_t txSize)
{
    return dev->transfer(dev, txBuf, txSize, NULL, 0);
}

/**
 * Receive data from the SPI bus.
 *
 * @param rxBuf: pointer to RX buffer.
 * @param rxSize: number of bytes to receive.
 * @return zero on success, a negative error code otherwise.
 */
static inline int spi_receive(const struct spiDevice *dev, void *rxBuf,
                              const size_t rxSize)
{
    return dev->transfer(dev, NULL, 0, rxBuf, rxSize);
}

#ifdef __cplusplus
}
#endif

#endif /* SPI */
