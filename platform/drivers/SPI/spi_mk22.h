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
 *  Instantiate an MK22 SPI bus.
 *
 * @param name: bus name.
 * @param peripheral: underlying MCU peripheral.
 * @param mutx: pointer to mutex, or NULL.
 */
#define MK22_SPI_BUS_DEFINE(name, peripheral, mutx) \
    extern const struct spiOps mk22_spi_ops;        \
    SPI_BUS_DEFINE(name, &mk22_spi_ops, peripheral, mutx)

/**
 * Initialise an SPI peripheral and driver.
 * Is left to application code to change the operating mode and alternate function
 * mapping of the corresponding gpio lines.
 *
 * @param bus: SPI device descriptor.
 * @param pbr: value for the baud rate prescaler (see reference manual).
 * @param br: value for the baud rate scaler (see reference manual).
 * @param flags: SPI configuration flags.
 * @return zero on success, a negative error code otherwise.
 */
int spiMk22_init(const struct spiBus *bus, const uint8_t pbr, const uint8_t br,
                 const uint8_t flags);

/**
 * Shut down an SPI peripheral and driver.
 *
 * @param bus: SPI device descriptor.
 */
void spiMk22_terminate(const struct spiBus *bus);

#ifdef __cplusplus
}
#endif

#endif /* SPI_MK22_H */
