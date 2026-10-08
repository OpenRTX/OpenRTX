/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef I2C_BITBANG_H
#define I2C_BITBANG_H

#include <stdbool.h>
#include "peripherals/gpio.h"
#include "peripherals/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mutable, runtime state for the I2C bitbang driver: one instance is
 * needed per bus, kept separate from the (const) configuration data.
 */
struct i2cBitbangState {
    bool held; ///< True if the previous transfer left the bus without a stop condition
};

/**
 * I2C-like bitbang driver built on top of the portable gpioPin/gpioDev
 * interface of peripherals/gpio.h.
 *
 * Some devices (e.g. the BK1080 FM tuner) implement a serial bus that only
 * superficially resembles I2C: the chip has a single, fixed "device ID"
 * byte (not a hardware-managed 7-bit address + R/W cycle), and the actual
 * read/write direction is embedded in a later, ordinary data byte. Once
 * that direction byte has been sent, the chip starts driving (for reads)
 * or expects more data (for writes) immediately, within the very same
 * start/stop frame: no repeated start or second addressing byte ever
 * occurs. Standard I2C controllers, hardware or software, always
 * (re)generate an address + R/W byte at the start of every message and
 * thus cannot reproduce this behaviour.
 *
 * This driver bridges that gap while still exposing the generic
 * struct i2cDevice interface: "addr" is driven on the bus, shifted left by
 * one with the direction bit set, as a literal first byte ONLY when the
 * bus is currently idle. Calling i2c_write()/i2c_read() with stop = false
 * leaves the bus "open": the next call on the same device, of either
 * direction, continues the ongoing frame directly, with no new start
 * condition or address byte, exactly mirroring the BK1080's documented
 * protocol.
 *
 * Because the "open" state is tracked across independent read()/write()
 * calls, a single instance of this driver must not be shared between
 * multiple, concurrently-accessed logical devices.
 */
struct i2cBitbangCfg {
    const struct gpioPin sck;      ///< Serial clock
    const struct gpioPin sda;      ///< Serial data, bidirectional
    const uint32_t clkPeriod;      ///< Clock half-period, in us
    struct i2cBitbangState *state; ///< Pointer to mutable runtime state
};

/**
 * Driver API for a gpioPin-based I2C bitbang device.
 */
extern const struct i2cApi i2cBitbang_driver;

/**
 *  Instantiate a gpioPin-based I2C bitbang device.
 *
 * @param name: device name.
 * @param cfg: driver configuration data.
 * @param mutx: pointer to mutex, or NULL.
 */
#define I2C_BITBANG_DEVICE_DEFINE(name, cfg, mutx) \
    const struct i2cDevice name = {                \
        .driver = &i2cBitbang_driver,              \
        .periph = &cfg,                            \
        .mutex = mutx,                             \
    };

/**
 * Initialise a gpioPin-based I2C bitbang driver.
 *
 * @param dev: I2C bitbang device descriptor.
 */
void i2cBitbang_init(const struct i2cDevice *dev);

/**
 * Shut down a gpioPin-based I2C bitbang driver.
 *
 * @param dev: I2C bitbang device descriptor.
 */
void i2cBitbang_terminate(const struct i2cDevice *dev);

#ifdef __cplusplus
}
#endif

#endif /* I2C_BITBANG_H */
