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
 *  Instantiate an STM32 SPI bus.
 *
 * @param name: device name.
 * @param peripheral: underlying MCU peripheral.
 * @param mutx: pointer to mutex, or NULL.
 */
#define STM32_SPI_BUS_DEFINE(name, peripheral, mutx) \
    extern const struct spiOps stm32_spi_ops;        \
    SPI_BUS_DEFINE(name, &stm32_spi_ops, peripheral, mutx)

/**
 * Initialise an SPI peripheral and driver.
 * Is left to application code to change the operating mode and alternate function
 * mapping of the corresponding gpio lines.
 *
 * @param bus: SPI device descriptor.
 * @param speed: SPI clock speed.
 * @param flags: SPI configuration flags.
 * @return zero on success, a negative error code otherwise.
 */
int spiStm32_init(const struct spiBus *bus, const uint32_t speed,
                  const uint8_t flags);

/**
 * Shut down an SPI peripheral and driver.
 *
 * @param bus: SPI device descriptor.
 */
void spiStm32_terminate(const struct spiBus *bus);

#ifdef __cplusplus
}
#endif

#endif /* SPI_STM32_H */
