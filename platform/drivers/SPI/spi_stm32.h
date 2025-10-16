/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SPI_STM32_H
#define SPI_STM32_H

#include "peripherals/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Instantiate an STM32 SPI master device.
 *
 * @param name: device name.
 * @param peripheral: underlying MCU peripheral.
 * @param mutx: pointer to mutex, or NULL.
 */
#define SPI_STM32_DEVICE_DEFINE(name, peripheral, mutx)                      \
extern int spiStm32_transfer(const struct spiDevice *dev, const void *txBuf, \
                             void *rxBuf, const size_t size);                \
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
int spiStm32_init(const struct spiDevice *dev, const uint32_t speed, const uint8_t flags);

/**
 * Shut down an SPI peripheral and driver.
 *
 * @param dev: SPI device descriptor.
 */
void spiStm32_terminate(const struct spiDevice *dev);


#ifdef __cplusplus
}
#endif

#endif /* SPI_STM32_H */
