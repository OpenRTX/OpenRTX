/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SPI_H
#define SPI_H

#include <errno.h>
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
 * @param rxBuf: pointer to RX buffer, can be NULL.
 * @param size: number of bytes to transfer.
 * @return zero on success, a negative error code otherwise.
 */
typedef int (*spi_transfer_impl)(const struct spiDevice *dev, const void *txBuf,
                                 void *rxBuf, const size_t size);

/**
 * SPI peripheral descriptor.
 */
struct spiDevice
{
    spi_transfer_impl transfer; ///< Device-specific implementation of the SPI transfer function
    const void        *priv;    ///< Pointer to device driver private data
    pthread_mutex_t   *mutex;   ///< Pointer to mutex for exclusive access
};

/**
 * Perform basic initialization of a generic SPI device.
 *
 * @param dev: pointer to peripheral descriptor.
 */
static inline void spi_init(const struct spiDevice *dev)
{
    if(dev->mutex != NULL)
        pthread_mutex_init(dev->mutex, NULL);
}

/**
 * Perform basic initialization of a generic SPI device.
 *
 * @param dev: pointer to peripheral descriptor.
 */
static inline void spi_terminate(const struct spiDevice *dev)
{
    if(dev->mutex != NULL)
        pthread_mutex_destroy(dev->mutex);
}

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

    return pthread_mutex_lock(dev->mutex);
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

    return pthread_mutex_trylock(dev->mutex);
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

    return pthread_mutex_unlock(dev->mutex);
}

/**
 * Transfer data on the SPI bus. Only symmetric transfers (same TX and RX size)
 * are allowed!
 *
 * @param txBuf: pointer to TX buffer, can be NULL.
 * @param rxBuf: pointer to RX buffer, can be NULL.
 * @param size: number of bytes to transfer.
 * @return zero on success, a negative error code otherwise.
 */
static inline int spi_transfer(const struct spiDevice *dev, const void *txBuf,
                               void *rxBuf, const size_t size)
{
    return dev->transfer(dev, txBuf, rxBuf, size);
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
    return dev->transfer(dev, txBuf, NULL, txSize);
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
    return dev->transfer(dev, NULL, rxBuf, rxSize);
}

#ifdef __cplusplus
}
#endif

#endif /* SPI */
