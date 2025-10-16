/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SKY73210_H
#define SKY73210_H

#include "peripherals/gpio.h"
#include "peripherals/spi.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SKY73210 device data.
 */
struct sky73210
{
    const struct spiDevice *spi;      ///< SPI bus device driver
    const struct gpioPin   cs;        ///< Chip select gpio
    const uint32_t         refClk;    ///< Reference clock frequency, in Hz
};

/**
 * Initialise the PLL.
 *
 * @param dev: pointer to device data.
 */
void SKY73210_init(const struct sky73210 *dev);

/**
 * Terminate PLL driver.
 *
 * @param dev: pointer to device data.
 */
void SKY73210_terminate(const struct sky73210 *dev);

/**
 * Change VCO frequency.
 *
 * @param dev: pointer to device data.
 * @param freq: new VCO frequency, in Hz.
 * @param clkDiv: reference clock division factor.
 */
void SKY73210_setFrequency(const struct sky73210 *dev, const uint32_t freq,
                           uint8_t clkDiv);

#ifdef __cplusplus
}
#endif

#endif /* SKY73210_H */
