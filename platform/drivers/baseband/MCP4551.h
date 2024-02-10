/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                Mathis Schmieder DB9MAT                  *
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

#ifndef MCP4551_H
#define MCP4551_H

#include <stdint.h>
#include <stdbool.h>
#include <datatypes.h>
#include <peripherals/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the MCP4551 device.
 *
 * @param i2c: driver managing the I2C bus the chip is connected to.
 * @param devAddr: I2C device address of the chip.
 * @return zero on success, a negative error code otherwise.
 */
int mcp4551_init(const struct i2cDevice *i2c, const uint8_t devAddr);

/**
 * Set the MCP4551 wiper to a given position.
 *
 * @param i2c: driver managing the I2C bus the chip is connected to.
 * @param devAddr: I2C device address of the chip.
 * @param value: new wiper position.
 * @return zero on success, a negative error code otherwise.
 */
int mcp4551_setWiper(const struct i2cDevice *i2c, const uint8_t devAddr,
                     const uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* MCP4551_H */
