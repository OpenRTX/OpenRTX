/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SPI_MK22_H
#define SPI_MK22_H

#include "peripherals/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Instantiate an MK22 SPI master device.
 *
 * @param name: device name.
 * @param peripheral: underlying MCU peripheral.
 * @param mutx: pointer to mutex, or NULL.
 */
#define SPI_MK22_DEVICE_DEFINE(name, peripheral, mutx)                      \
extern int spiMk22_transfer(const struct spiDevice *dev, const void *txBuf, \
                            void *rxBuf, const size_t size);                \
const struct spiDevice name =                                               \
{                                                                           \
    .transfer = &spiMk22_transfer,                                          \
    .priv     = peripheral,                                                 \
    .mutex    = mutx                                                        \
};

/**
 * Initialise an SPI peripheral and driver.
 * Is left to application code to change the operating mode and alternate function
 * mapping of the corresponding gpio lines.
 *
 * @param dev: SPI device descriptor.
 * @param pbr: value for the baud rate prescaler (see reference manual).
 * @param br: value for the baud rate scaler (see reference manual).
 * @param flags: SPI configuration flags.
 * @return zero on success, a negative error code otherwise.
 */
int spiMk22_init(const struct spiDevice *dev, const uint8_t pbr, const uint8_t br,
                 const uint8_t flags);

/**
 * Shut down an SPI peripheral and driver.
 *
 * @param dev: SPI device descriptor.
 */
void spiMk22_terminate(const struct spiDevice *dev);


#ifdef __cplusplus
}
#endif

#endif /* SPI_MK22_H */
