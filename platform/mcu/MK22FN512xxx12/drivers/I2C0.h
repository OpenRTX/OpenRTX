/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef I2C0_H
#define I2C0_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * Acquire exclusive ownership on the I2C peripheral by locking an internal
 * mutex. This function is nonblocking and returs true if mutex has been
 * successfully locked by the caller.
 * @return true if device has been locked.
 */
bool i2c0_lockDevice();

/**
 * Acquire exclusive ownership on the I2C peripheral by locking an internal
 * mutex. In case mutex is already locked, this function blocks the execution
 * flow until it becomes free again.
 */
void i2c0_lockDeviceBlocking();

/**
 * Release exclusive ownership on the I2C peripheral.
 */
void i2c0_releaseDevice();

#ifdef __cplusplus
}
#endif

#endif /* I2C0_H */
