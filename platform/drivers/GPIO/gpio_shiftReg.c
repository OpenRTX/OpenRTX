/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/delays.h"
#include "peripherals/gpio.h"
#include <string.h>
#include <errno.h>
#include "drivers/GPIO/gpio_shiftReg.h"

void gpioShiftReg_init(const struct gpioDev *dev)
{
    const struct gpioShiftRegPriv *cfg = (const struct gpioShiftRegPriv *) dev->priv;
    const size_t nBytes = (cfg->numOutputs + 8 - 1) / 8;

    // Setup strobe line
    gpioDev_setMode(cfg->strobe.port, cfg->strobe.pin, OUTPUT);
    gpioDev_clear(cfg->strobe.port,   cfg->strobe.pin);

    // Clear all outputs
    memset(cfg->outData, 0x00, nBytes);
    spi_send(cfg->spi, cfg->outData, nBytes);
    gpioDev_set(cfg->strobe.port, cfg->strobe.pin);
}

void gpioShiftReg_terminate(const struct gpioDev *dev)
{
    (void) dev;
}

static int gpioShiftReg_mode(const struct gpioDev *dev, const uint8_t pin,
                             const uint16_t mode)
{
    (void) dev;
    (void) pin;
    (void) mode;

    return -ENOTSUP;
}

static void gpioShiftReg_set(const struct gpioDev *dev, const uint8_t pin)
{
    const struct gpioShiftRegPriv *cfg = (const struct gpioShiftRegPriv *) dev->priv;
    const size_t nBytes = (cfg->numOutputs + 8 - 1) / 8;
    const size_t byte = (cfg->numOutputs - 1 - pin) / 8;
    const size_t bit  = pin % 8;

    if(pin > cfg->numOutputs)
        return;

    __disable_irq();

    cfg->outData[byte] |= (1 << bit);

    gpioDev_clear(cfg->strobe.port, cfg->strobe.pin);
    spi_send(cfg->spi, cfg->outData, nBytes);
    gpioDev_set(cfg->strobe.port, cfg->strobe.pin);

    __enable_irq();
}

static void gpioShiftReg_clear(const struct gpioDev *dev, const uint8_t pin)
{
    const struct gpioShiftRegPriv *cfg = (const struct gpioShiftRegPriv *) dev->priv;
    const size_t nBytes = (cfg->numOutputs + 8 - 1) / 8;
    const size_t byte = (cfg->numOutputs - 1 - pin) / 8;
    const size_t bit  = pin % 8;

    if(pin > cfg->numOutputs)
        return;

    __disable_irq();

    cfg->outData[byte] &= ~(1 << bit);

    gpioDev_clear(cfg->strobe.port, cfg->strobe.pin);
    spi_send(cfg->spi, cfg->outData, nBytes);
    gpioDev_set(cfg->strobe.port, cfg->strobe.pin);

    __enable_irq();
}

static bool gpioShiftReg_read(const struct gpioDev *dev, const uint8_t pin)
{
    const struct gpioShiftRegPriv *cfg = (const struct gpioShiftRegPriv *) dev->priv;
    const size_t byte = (cfg->numOutputs - 1 - pin) / 8;
    const size_t bit  = pin % 8;

    if(pin > cfg->numOutputs)
        return false;

    return ((cfg->outData[byte] & (1 << bit)) != 0) ? true : false;
}

const struct gpioApi gpioShiftReg_ops =
{
    .mode  = &gpioShiftReg_mode,
    .set   = &gpioShiftReg_set,
    .clear = &gpioShiftReg_clear,
    .read  = &gpioShiftReg_read
};
