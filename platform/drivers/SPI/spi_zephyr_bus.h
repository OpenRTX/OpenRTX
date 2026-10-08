/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SPI_ZEPHYR_BUS_H
#define SPI_ZEPHYR_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This header purposefully avoids including any Zephyr header (in
 * particular <zephyr/drivers/spi.h>): Zephyr's own SPI API defines a
 * function named "spi_release", with a signature different from the one
 * of the generic OpenRTX SPI interface of peripherals/spi.h. Including
 * both headers in the same translation unit would thus result in a
 * compile error due to the conflicting declarations.
 *
 * Keeping all the Zephyr-specific code confined to spi_zephyr_bus.c, which
 * never includes peripherals/spi.h, allows spi_zephyr.c to bridge the two
 * APIs without ever exposing both of them to the same compilation unit.
 */
struct device;

/**
 * Perform a transfer on a Zephyr-native SPI bus, wrapping Zephyr's own
 * spi_transceive() API.
 *
 * @param bus: Zephyr SPI controller device.
 * @param frequency: bus clock frequency, in Hz.
 * @param halfDuplex: true if the bus has a single, bidirectional data line.
 * @param txBuf: pointer to TX buffer, can be NULL.
 * @param rxBuf: pointer to RX buffer, can be NULL.
 * @param size: number of bytes to transfer.
 * @return zero on success, a negative error code otherwise.
 */
int zephyrSpiBus_transfer(const struct device *bus, uint32_t frequency,
                          bool halfDuplex, const void *txBuf, void *rxBuf,
                          size_t size);

/**
 * Check whether the underlying Zephyr SPI bus device is ready for use.
 *
 * @param bus: Zephyr SPI controller device.
 * @return zero if ready, a negative error code otherwise.
 */
int zephyrSpiBus_isReady(const struct device *bus);

#ifdef __cplusplus
}
#endif

#endif /* SPI_ZEPHYR_BUS_H */
