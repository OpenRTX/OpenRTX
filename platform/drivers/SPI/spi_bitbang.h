/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SPI_BITBANG_H
#define SPI_BITBANG_H

#include "peripherals/gpio.h"
#include "peripherals/spi.h"
#include "spi_custom.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCK_PERIOD_FROM_FREQ(x) (1000000/x)

/**
 * Data structure collecting the configuration data for the SPI bitbang driver.
 *
 * The driver uses the MCU native gpio driver for two reasons:
 * 1) the faster the gpios are driver, the faster the SPI bitbang can work
 * 2) using an external port expander to do SPI bitbang does not make sense...
 */
struct spiConfig
{
    const struct gpio clk;          ///< SPI clock
    const struct gpio mosi;         ///< SPI data from MCU to peripherals
    const struct gpio miso;         ///< SPI data from peripherals to MCU
    const uint32_t    clkPeriod;    ///< Clock period, in us
    const uint8_t     flags;        ///< SPI configuration flags
};

/**
 *  Instantiate an SPI bitbang device.
 *
 * @param name: device name.
 * @param cfg: driver configuration data.
 * @param mutx: pointer to mutex, or NULL.
 */
#define SPI_BITBANG_DEVICE_DEFINE(name, cfg, mutx)           \
uint8_t spiBitbang_sendRecv(const void *priv, uint8_t data); \
const struct spiCustomDevice name =                          \
{                                                            \
    .transfer = spiCustom_transfer,                          \
    .priv     = &cfg,                                        \
    .mutex    = mutx,                                        \
    .spiFunc  = spiBitbang_sendRecv                          \
};

/**
 * Initialise a bitbang SPI driver.
 *
 * @param dev: SPI bitbang device descriptor.
 * @return zero on success, a negative error code otherwise.
 */
int spiBitbang_init(const struct spiCustomDevice *dev);

/**
 * Shut down a bitbang SPI driver.
 *
 * @param dev: SPI bitbang device descriptor.
 */
void spiBitbang_terminate(const struct spiCustomDevice *dev);

#ifdef __cplusplus
}
#endif

#endif /* SPI_BITBANG_H */
