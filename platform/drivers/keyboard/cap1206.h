/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CAP1206_H
#define CAP1206_H

#include "peripherals/i2c.h"
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

