/***************************************************************************
 *   Copyright (C) 2024 by Morgan Diepart ON4MOD                           *
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

#ifndef CAP1206_H
#define CAP1206_H

#include <peripherals/i2c.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the CAP1206 device.
 *
 * @param i2c: driver managing the I2C bus the chip is connected to.
 * @return zero on success, a negative error code otherwise.
 */
int cap1206_init(const struct i2cDevice *i2c);

/**
 * Read the status of the touch keys connected to the CAP1206 device.
 *
 * @param i2c: driver managing the I2C bus the chip is connected to.
 * @return a bitmap representing the status of the keys or a negative error code.
 */
int cap1206_readkeys(const struct i2cDevice *i2c);

#ifdef __cplusplus
}
#endif

#endif /* CAP1206_H */

