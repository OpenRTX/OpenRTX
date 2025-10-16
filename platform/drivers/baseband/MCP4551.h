/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MCP4551_H
#define MCP4551_H

#include <stdint.h>
#include <stdbool.h>
#include "core/datatypes.h"
#include "peripherals/i2c.h"

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
