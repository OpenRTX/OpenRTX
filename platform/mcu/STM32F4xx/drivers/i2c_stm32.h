/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                         Morgan Diepart ON4MOD                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef I2C_STM32_H
#define I2C_STM32_H

#include <peripherals/i2c.h>

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
