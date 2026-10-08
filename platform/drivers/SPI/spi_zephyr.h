/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SPI_ZEPHYR_H
#define SPI_ZEPHYR_H

#include <stdbool.h>
#include "peripherals/spi.h"
#include "spi_zephyr_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SPI driver wrapping a Zephyr-native SPI controller device, exposed to the
 * rest of OpenRTX through the generic struct spiDevice interface of
 * peripherals/spi.h.
 *
 * Chip select, when toggled directly by the device driver rather than by
 * the bus controller (e.g. for 3-wire, half-duplex devices), must be left
 * unconfigured in the devicetree node ("cs-gpios" property absent): Zephyr's
 * SPI subsystem then skips any built-in CS handling, leaving full control
 * to the caller.
 */
struct spiZephyrCfg {
    const struct device *bus; ///< Zephyr SPI controller device
    const uint32_t frequency; ///< Bus clock frequency, in Hz
    const uint8_t flags;      ///< SPI configuration flags, see enum SPIFlags
};

/**
 *  Instantiate a Zephyr-backed SPI device.
 *
 * @param name: device name.
 * @param cfg: driver configuration data.
 * @param mutx: pointer to mutex, or NULL.
 */
#define SPI_ZEPHYR_DEVICE_DEFINE(name, cfg, mutx)                          \
    int spiZephyr_transfer(const struct spiDevice *dev, const void *txBuf, \
                           void *rxBuf, const size_t size);                \
    const struct spiDevice name = {                                        \
        .transfer = spiZephyr_transfer,                                    \
        .priv = &cfg,                                                      \
        .mutex = mutx,                                                     \
    };

/**
 * Transfer data on a Zephyr-backed SPI bus.
 *
 * @param dev: SPI device handle.
 * @param txBuf: pointer to TX buffer, can be NULL.
 * @param rxBuf: pointer to RX buffer, can be NULL.
 * @param size: number of bytes to transfer.
 * @return zero on success, a negative error code otherwise.
 */
int spiZephyr_transfer(const struct spiDevice *dev, const void *txBuf,
                       void *rxBuf, const size_t size);

/**
 * Initialise a Zephyr-backed SPI driver, checking that the underlying bus
 * controller is ready.
 *
 * @param dev: SPI device descriptor.
 * @return zero on success, a negative error code otherwise.
 */
int spiZephyr_init(const struct spiDevice *dev);

/**
 * Shut down a Zephyr-backed SPI driver.
 *
 * @param dev: SPI device descriptor.
 */
void spiZephyr_terminate(const struct spiDevice *dev);

#ifdef __cplusplus
}
#endif

#endif /* SPI_ZEPHYR_H */
