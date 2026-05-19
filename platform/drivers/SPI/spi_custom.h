/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SPI_CUSTOM_H
#define SPI_CUSTOM_H

#include "peripherals/spi.h"

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
