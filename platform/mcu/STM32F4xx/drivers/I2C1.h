/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#ifndef I2C1_H
#define I2C1_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise I2C1 peripheral with a clock frequency of ~100kHz.
 * NOTE: this driver does not configure the I2C GPIOs, which have to be put in
 * alternate mode by application code.
 * @param slow: Set to true to initialize I2C in extra-slow mode (about 20 kHz)
 */
void i2c1_init(bool slow, uint32_t timeout);

/**
 * Shut down I2C1 peripheral.
 * NOTE: is left to application code to change the operating mode of the I2C
 * GPIOs
 */
void i2c1_terminate();

/**
 * Acquire exclusive ownership on the I2C peripheral by locking an internal
 * mutex. This function is nonblocking and returs true if mutex has been
 * successfully locked by the caller.
 * @return true if device has been locked.
 */
bool i2c1_lockDevice();

/**
 * Acquire exclusive ownership on the I2C peripheral by locking an internal
 * mutex. In case mutex is already locked, this function blocks the execution
 * flow until it becomes free again.
 */
void i2c1_lockDeviceBlocking();

/**
 * Release exclusive ownership on the I2C peripheral.
 */
void i2c1_releaseDevice();

/**
 * Send bytes over the I2C bus.
 * @param addr: 7 bits device address to send the bytes to
 * @param bytes: array containing the bytes to send
 * @param length: number of bytes to send
 * @param stop: whether to send a stop bit or not after the transmission
 */
error_t i2c1_write_bytes(uint8_t addr, uint8_t *bytes, uint8_t length, bool stop);

/**
 * Receives bytes over the I2C bus.
 * @param addr: 7 bits device address to receive the bytes from
 * @param bytes: array in which to store the received bytes
 * @param length: number of bytes to receive
 * @param stop: whether to send a stop bit or not after the reception
 */
error_t i2c1_read_bytes(uint8_t addr, uint8_t *bytes, uint8_t length, bool stop);

#ifdef __cplusplus
}
#endif

#endif /* I2C1_H */