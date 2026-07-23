/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * I2C master driver for the HR_C7000's DesignWare DW_apb_i2c blocks (I2C0..2),
 * implementing the standard peripherals/i2c.h interface.  One driver serves
 * every instance; the bus is selected by the i2cDevice's `periph` (an
 * I2C_TypeDef base).
 *
 * HR_C7000 quirk (manual 5.1.6.27): unlike stock DesignWare, the master does
 * NOT auto-start when the TX FIFO has data -- it waits for an explicit IC_START
 * (+0xa0) write.  This driver therefore treats `stop`:
 *   - false -> queue the bytes into the TX FIFO but do NOT trigger yet;
 *   - true  -> queue (last byte flagged STOP) and trigger the whole accumulated
 *              transfer with one IC_START.
 * That reproduces the combined repeated-START register read (write the pointer
 * with stop=false, then read with stop=true) as a single hardware transfer.
 */

#ifndef I2C_HRC7000_H
#define I2C_HRC7000_H

#include "peripherals/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Device driver API for the HR_C7000 DesignWare I2C master. */
extern const struct i2cApi i2cHrc7000_driver;

/**
 * Instantiate an HR_C7000 I2C master device.
 *
 * @param name: device name.
 * @param peripheral: underlying I2C base (e.g. I2C2).
 * @param mutx: pointer to mutex, or NULL.
 */
#define I2C_HRC7000_DEVICE_DEFINE(name, peripheral, mutx) \
    const struct i2cDevice name = {                       \
        .driver = &i2cHrc7000_driver,                     \
        .periph = peripheral,                             \
        .mutex = mutx,                                    \
    };

/**
 * Initialise an HR_C7000 I2C peripheral.  The timing is the fixed vendor
 * fast-mode setup (the `speed` argument is accepted for interface conformance
 * but not yet used).  Does not configure any pads -- the RTC's I2C2 is an
 * internal bus; external instances must have their pads muxed by app code.
 *
 * @param dev: device driver handle.
 * @param speed: bus speed (see enum I2CSpeed); currently ignored.
 * @return zero on success, a negative error code otherwise.
 */
int i2c_init(const struct i2cDevice *dev, const uint8_t speed);

/**
 * Shut down an HR_C7000 I2C peripheral.
 *
 * @param dev: device driver handle.
 */
void i2c_terminate(const struct i2cDevice *dev);

#ifdef __cplusplus
}
#endif

#endif /* I2C_HRC7000_H */
