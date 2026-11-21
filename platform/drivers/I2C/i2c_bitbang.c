/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include "interfaces/delays.h"
#include "i2c_bitbang.h"

static inline void halfDelay(const struct i2cBitbangCfg *cfg)
{
    delayUs(cfg->clkPeriod);
}

static void i2cBus_start(const struct i2cBitbangCfg *cfg)
{
    gpioPin_set(&cfg->sda);
    halfDelay(cfg);
    gpioPin_set(&cfg->sck);
    halfDelay(cfg);
    gpioPin_clear(&cfg->sda);
    halfDelay(cfg);
    gpioPin_clear(&cfg->sck);
    halfDelay(cfg);
}

static void i2cBus_stop(const struct i2cBitbangCfg *cfg)
{
    gpioPin_clear(&cfg->sda);
    halfDelay(cfg);
    gpioPin_clear(&cfg->sck);
    halfDelay(cfg);
    gpioPin_set(&cfg->sck);
    halfDelay(cfg);
    gpioPin_set(&cfg->sda);
    halfDelay(cfg);
}

/**
 * \internal
 * Write a single byte on the bus and check for the slave's ACK.
 *
 * @param cfg: driver configuration data.
 * @param data: byte to be sent.
 * @return zero if the byte was ACKed, a negative error code otherwise.
 */
static int i2cBus_writeByte(const struct i2cBitbangCfg *cfg, uint8_t data)
{
    int ret = -EIO;

    gpioPin_clear(&cfg->sck);
    halfDelay(cfg);

    for (uint8_t i = 0; i < 8; i++) {
        if ((data & 0x80) == 0)
            gpioPin_clear(&cfg->sda);
        else
            gpioPin_set(&cfg->sda);

        data <<= 1;
        halfDelay(cfg);
        gpioPin_set(&cfg->sck);
        halfDelay(cfg);
        gpioPin_clear(&cfg->sck);
        halfDelay(cfg);
    }

    // Release SDA and poll for the slave's ACK (SDA driven low)
    gpioPin_setMode(&cfg->sda, INPUT_PULL_UP);
    gpioPin_set(&cfg->sda);
    halfDelay(cfg);
    gpioPin_set(&cfg->sck);
    halfDelay(cfg);

    for (uint16_t i = 0; i < 255; i++) {
        if (gpioPin_read(&cfg->sda) == 0) {
            ret = 0;
            break;
        }
    }

    gpioPin_clear(&cfg->sck);
    halfDelay(cfg);
    gpioPin_setMode(&cfg->sda, OUTPUT);
    gpioPin_clear(&cfg->sda);

    return ret;
}

/**
 * \internal
 * Read a single byte from the bus, generating the master's ACK/NACK.
 *
 * @param cfg: driver configuration data.
 * @param final: true to NACK (last byte of the transfer), false to ACK.
 * @return byte read from the bus.
 */
static uint8_t i2cBus_readByte(const struct i2cBitbangCfg *cfg, bool final)
{
    uint8_t data = 0;

    gpioPin_setMode(&cfg->sda, INPUT_PULL_UP);

    for (uint8_t i = 0; i < 8; i++) {
        gpioPin_clear(&cfg->sck);
        halfDelay(cfg);
        gpioPin_set(&cfg->sck);
        halfDelay(cfg);
        data <<= 1;
        halfDelay(cfg);
        if (gpioPin_read(&cfg->sda))
            data |= 1U;
        gpioPin_clear(&cfg->sck);
        halfDelay(cfg);
    }

    gpioPin_setMode(&cfg->sda, OUTPUT);
    gpioPin_clear(&cfg->sck);
    halfDelay(cfg);

    if (final)
        gpioPin_set(&cfg->sda);
    else
        gpioPin_clear(&cfg->sda);

    halfDelay(cfg);
    gpioPin_set(&cfg->sck);
    halfDelay(cfg);
    gpioPin_clear(&cfg->sck);
    halfDelay(cfg);

    return data;
}

static int i2cBitbang_write(const struct i2cDevice *dev, const uint8_t addr,
                            const void *data, const size_t len, const bool stop)
{
    const struct i2cBitbangCfg *cfg = (const struct i2cBitbangCfg *)dev->periph;
    const uint8_t *bytes = (const uint8_t *)data;
    int ret = 0;

    if (!cfg->state->held) {
        i2cBus_start(cfg);
        ret = i2cBus_writeByte(cfg, (uint8_t)(addr << 1));
    }

    for (size_t i = 0; (i < len) && (ret == 0); i++)
        ret = i2cBus_writeByte(cfg, bytes[i]);

    if (stop || (ret != 0)) {
        i2cBus_stop(cfg);
        cfg->state->held = false;
    } else {
        cfg->state->held = true;
    }

    return ret;
}

static int i2cBitbang_read(const struct i2cDevice *dev, const uint8_t addr,
                           void *data, const size_t len, const bool stop)
{
    const struct i2cBitbangCfg *cfg = (const struct i2cBitbangCfg *)dev->periph;
    uint8_t *bytes = (uint8_t *)data;
    int ret = 0;

    if (!cfg->state->held) {
        i2cBus_start(cfg);
        ret = i2cBus_writeByte(cfg, (uint8_t)((addr << 1) | 1));
    }

    if (ret == 0) {
        for (size_t i = 0; i < len; i++)
            bytes[i] = i2cBus_readByte(cfg, (i == (len - 1)));
    }

    if (stop || (ret != 0)) {
        i2cBus_stop(cfg);
        cfg->state->held = false;
    } else {
        cfg->state->held = true;
    }

    return ret;
}

const struct i2cApi i2cBitbang_driver = {
    .read = i2cBitbang_read,
    .write = i2cBitbang_write,
};

void i2cBitbang_init(const struct i2cDevice *dev)
{
    const struct i2cBitbangCfg *cfg = (const struct i2cBitbangCfg *)dev->periph;

    gpioPin_setMode(&cfg->sck, OUTPUT);
    gpioPin_clear(&cfg->sck);
    gpioPin_setMode(&cfg->sda, OUTPUT);
    gpioPin_set(&cfg->sda);

    cfg->state->held = false;

    if (dev->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *)dev->mutex, NULL);
}

void i2cBitbang_terminate(const struct i2cDevice *dev)
{
    const struct i2cBitbangCfg *cfg = (const struct i2cBitbangCfg *)dev->periph;

    gpioPin_setMode(&cfg->sck, INPUT);
    gpioPin_setMode(&cfg->sda, INPUT);

    if (dev->mutex != NULL)
        pthread_mutex_destroy((pthread_mutex_t *)dev->mutex);
}
