/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#ifndef I2C0_H
#define I2C0_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Initialise I2C peripheral with a bus clock frequency of ~100kHz.
 * NOTE: this driver does not configure the I2C GPIOs, which have to be put in
 * alternate open drain mode by application code.
 */
void i2c0_init();

/**
 * Shut down I2C peripheral.
 * NOTE: is left to application code to change the operating mode of the I2C
 * GPIOs
 */
void i2c0_terminate();

/**
 * Write data to an I2C peripheral.
 * @param addr: address of target peripheral.
 * @param buf: pointer to buffer containing data to be sent.
 * @param len: number of bytes to be sent.
 * @param sendStop: set to true to generate a stop condition on transfer end.
 */
void i2c0_write(uint8_t addr, void *buf, size_t len, bool sendStop);

/**
 * Read data from an I2C peripheral.
 * @param addr: address of target peripheral.
 * @param buf: pointer to a buffer in which received data are written.
 * @param len: number of bytes to be transferred.
 */
void i2c0_read(uint8_t addr, void *buf, size_t len);

/**
 * Check if I2C bus is already in use.
 * @return true if bus is busy.
 */
bool i2c0_busy();

#endif /* I2C0_H */
