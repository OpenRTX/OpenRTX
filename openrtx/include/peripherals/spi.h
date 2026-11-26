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
enum SPIFlags {
    SPI_FLAG_CPOL = 1,   ///< Clock line is high when idle.
    SPI_FLAG_CPHA = 2,   ///< Data lines are sampled on clock falling edge.
    SPI_LSB_FIRST = 4,   ///< Transfer data LSB first.
    SPI_HALF_DUPLEX = 8, ///< Half-duplex data transfer mode
};

struct spiBus;

/**
 * SPI bus access API
 */
struct spiOps {
    /**
     * Transfer data on the SPI bus.
     *
     * @param bus: SPI device handle.
     * @param txBuf: pointer to TX buffer, can be NULL.
     * @param rxBuf: pointer to RX buffer, can be NULL.
     * @param size: number of bytes to transfer.
     * @return zero on success, a negative error code otherwise.
     */
    int (*transfer)(const struct spiBus *bus, const void *txBuf, void *rxBuf,
                    const size_t size);

    /**
     * Transmit data on the SPI bus.
     *
     * @param bus: SPI device handle.
     * @param txBuf: pointer to TX buffer, can be NULL.
     * @param size: number of bytes to transfer.
     * @return zero on success, a negative error code otherwise.
     */
    int (*send)(const struct spiBus *bus, const void *txBuf, const size_t size);

    /**
     * Receive data from the SPI bus.
     *
     * @param bus: SPI device handle.
     * @param rxBuf: pointer to RX buffer, can be NULL.
     * @param size: number of bytes to transfer.
     * @return zero on success, a negative error code otherwise.
     */
    int (*receive)(const struct spiBus *bus, void *rxBuf, const size_t size);
};

/**
 * SPI bus descriptor.
 */
struct spiBus {
    const void *priv;         ///< Pointer to device driver private data
    const struct spiOps *ops; ///< Bus access functions
    pthread_mutex_t *mutex;   ///< Pointer to mutex for exclusive access
};

/**
 *  Instantiate a SPI bus.
 *
 * @param name: bus name.
 * @param _ops: pointer to bus access API.
 * @param _priv: pointer to driver private data.
 * @param _mutex: pointer to mutex, or NULL.
 */
#define SPI_BUS_DEFINE(name, _ops, _priv, _mutex) \
    const struct spiBus name = {                  \
        .priv = _priv,                            \
        .ops = _ops,                              \
        .mutex = _mutex,                          \
    }

/**
 * Perform basic initialization of a generic SPI device.
 *
 * @param bus: pointer to bus descriptor.
 */
static inline void spiBus_init(const struct spiBus *bus)
{
    if (bus->mutex != NULL)
        pthread_mutex_init(bus->mutex, NULL);
}

/**
 * Perform basic initialization of a generic SPI device.
 *
 * @param bus: pointer to bus descriptor.
 */
static inline void spiBus_terminate(const struct spiBus *bus)
{
    if (bus->mutex != NULL)
        pthread_mutex_destroy(bus->mutex);
}

/**
 * Acquire exclusive ownership on an SPI bus.
 *
 * @param bus: pointer to bus descriptor.
 * @return zero on success, a negative error code on failure.
 */
static inline int spiBus_lock(const struct spiBus *bus)
{
    if (bus->mutex == NULL)
        return 0;

    return pthread_mutex_lock(bus->mutex);
}

/**
 * Attempt to acquire exclusive ownership on an SPI bus.
 *
 * @param bus: pointer to bus descriptor.
 * @return zero on success, a negative error code on failure.
 */
static inline int spiBus_tryLock(const struct spiBus *bus)
{
    if (bus->mutex == NULL)
        return 0;

    return pthread_mutex_trylock(bus->mutex);
}

/**
 * Release exclusive ownership on an SPI bus.
 *
 * @param bus: pointer to bus descriptor.
 * @return zero on success, a negative error code on failure.
 */
static inline int spiBus_release(const struct spiBus *bus)
{
    if (bus->mutex == NULL)
        return 0;

    return pthread_mutex_unlock(bus->mutex);
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
static inline int spiBus_transfer(const struct spiBus *bus, const void *txBuf,
                                  void *rxBuf, const size_t size)
{
    return bus->ops->transfer(bus, txBuf, rxBuf, size);
}

/**
 * Send data on the SPI bus.
 *
 * @param txBuf: pointer to TX buffer.
 * @param txSize: number of bytes to send.
 * @return zero on success, a negative error code otherwise.
 */
static inline int spiBus_send(const struct spiBus *bus, const void *txBuf,
                              const size_t txSize)
{
    return bus->ops->send(bus, txBuf, txSize);
}

/**
 * Receive data from the SPI bus.
 *
 * @param rxBuf: pointer to RX buffer.
 * @param rxSize: number of bytes to receive.
 * @return zero on success, a negative error code otherwise.
 */
static inline int spiBus_receive(const struct spiBus *bus, void *rxBuf,
                                 const size_t rxSize)
{
    return bus->ops->receive(bus, rxBuf, rxSize);
}

#ifdef __cplusplus
}
#endif

#endif /* SPI */
