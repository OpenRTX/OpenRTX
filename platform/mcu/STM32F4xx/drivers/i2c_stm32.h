/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef I2C_STM32_H
#define I2C_STM32_H

#include "peripherals/i2c.h"

/**
 * Device driver API for STM32 I2C peripheral.
 */
extern const struct i2cApi i2cStm32_driver;

/**
 *  Instantiate an STM32 I2C master device.
 *
 * @param name: device name.
 * @param peripheral: underlying MCU peripheral.
 * @param mutx: pointer to mutex, or NULL.
 */
#define I2C_STM32_DEVICE_DEFINE(name, peripheral, mutx) \
const struct i2cDevice name =                           \
{                                                       \
    .driver = &i2cStm32_driver,                         \
    .periph = peripheral,                               \
    .mutex  = mutx                                      \
};


/**
 * Initialise an STM32 I2C peripheral with a given bus speed.
 * This driver does not configure the I2C GPIOs, which have to be put in
 * the appropriate mode by application code.
 *
 * @param dev: device driver handle.
 * @param speed: bus speed.
 * @return zero on success, a negative error code otherwise.
 */
int i2c_init(const struct i2cDevice *dev, const uint8_t speed);

/**
 * Shut down an STM32 I2C peripheral.
 * Is left to application code to change the operating mode of the I2C GPIOs.
 *
 * @param dev: device driver handle.
 */
void i2c_terminate(const struct i2cDevice *dev);

#endif /* I2C_STM32_H */
