/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GPIO_SHIFTREG_H
#define GPIO_SHIFTREG_H

#include "peripherals/gpio.h"
#include "peripherals/spi.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver to use a chain of shift registers (e.g. 74HC595) as GPIO outputs.
 * It requires an SPI interface, of which only the MOSI and SCK lines are used
 * and an MCU gpio for shift register strobe signal.
 */

/**
 * Private data of shift register gpio driver.
 */
struct gpioShiftRegPriv
{
    const struct spiDevice *spi;        ///< SPI interface device driver
    const struct gpioPin   strobe;      ///< Gpio for output strobe line
    const size_t           numOutputs;  ///< Number of actual outputs
    uint8_t *const         outData;     ///< Pointer to gpio state storage
};

/**
 *  Instantiate a shift register gpio device driver.
 *
 * @param name: device name.
 * @param spiDev: pointer to device driver managing the SPI bus.
 * @param strb: gpioPin struct for strobe line management.
 * @param numOuts: number of gpio outputs.
 */
#define GPIO_SHIFTREG_DEVICE_DEFINE(name, spiDev, strb, numOuts)  \
extern const struct gpioApi gpioShiftReg_ops;                     \
static uint8_t srData_##name[(numOuts + 8 - 1) / 8];              \
static const struct gpioShiftRegPriv srGpioPriv_##name =          \
{                                                                 \
    .spi        = spiDev,                                         \
    .strobe     = strb,                                           \
    .numOutputs = numOuts,                                        \
    .outData    = srData_##name                                   \
};                                                                \
const struct gpioDev name =                                       \
{                                                                 \
    .api  = &gpioShiftReg_ops,                                    \
    .priv = &srGpioPriv_##name                                    \
};

/**
 * Initialize the GPIO shift register driver.
 */
void gpioShiftReg_init(const struct gpioDev *dev);

/**
 * Shut down the GPIO shift register driver. Shift register outputs keep the last
 * assigned state.
 */
void gpioShiftReg_terminate(const struct gpioDev *dev);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_SHIFTREG_H */
