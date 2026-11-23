/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/baseband/BK4819_priv.h"
#include <assert.h>
#include <errno.h>

/**
 * Set the mode of a BK4819 GPIO pin.
 *
 * @param dev: GPIO device.
 * @param pin: pin number, 0 to 6.
 * @param mode: pin mode.
 * @return 0 on success, -EINVAL for an invalid pin, -ENOTSUP for an
 * unsupported mode.
 */
static int gpio_bk4819_mode(const struct gpioDev *dev, const uint8_t pin,
                            const uint16_t mode)
{
    if (pin > 6)
        return -EINVAL;

    struct BK4819Priv *cfg = (struct BK4819Priv *)dev->priv;
    const uint16_t direction = BK4819_REG33_DIRECTION_PIN0 >> pin;
    const uint16_t output = BK4819_REG33_OUTPUT_PIN0 >> pin;
    uint16_t mask = direction | output;
    uint16_t value = 0;

    switch (mode) {
        case INPUT:
            // Keep the output latch
            cfg->activeLowMask &= ~(1U << pin);
            mask = direction;
            value = direction;
            break;

        case OUTPUT:
            cfg->activeLowMask &= ~(1U << pin);
            break;

        case OUTPUT_ACTIVE_LOW:
            cfg->activeLowMask |= 1U << pin;
            value = output;
            break;

        default:
            return -ENOTSUP;
    }

    bk4819_maskSetRegister(cfg->bk4819, 0x33, mask, value);
    return 0;
}

/**
 * Drive a BK4819 GPIO pin to its active level.
 *
 * @param dev: GPIO device.
 * @param pin: pin number, 0 to 6.
 */
static void gpio_bk4819_set(const struct gpioDev *dev, const uint8_t pin)
{
    assert(pin <= 6);
    const struct BK4819Priv *cfg = dev->priv;
    const uint16_t output = BK4819_REG33_OUTPUT_PIN0 >> pin;
    uint16_t level = output;

    if ((cfg->activeLowMask & (1U << pin)) != 0)
        level = 0;

    bk4819_maskSetRegister(cfg->bk4819, 0x33, output, level);
}

/**
 * Drive a BK4819 GPIO pin to its inactive level.
 *
 * @param dev: GPIO device.
 * @param pin: pin number, 0 to 6.
 */
static void gpio_bk4819_clear(const struct gpioDev *dev, const uint8_t pin)
{
    assert(pin <= 6);
    const struct BK4819Priv *cfg = dev->priv;
    const uint16_t output = BK4819_REG33_OUTPUT_PIN0 >> pin;
    uint16_t level = 0;

    if ((cfg->activeLowMask & (1U << pin)) != 0)
        level = output;

    bk4819_maskSetRegister(cfg->bk4819, 0x33, output, level);
}

/**
 * Read a BK4819 GPIO pin.
 *
 * @param dev: GPIO device.
 * @param pin: pin number, 0 to 6.
 * @return true when the pin is at its active level.
 */
static bool gpio_bk4819_read(const struct gpioDev *dev, const uint8_t pin)
{
    assert(pin <= 6);
    const struct BK4819Priv *cfg = dev->priv;
    const uint16_t input = BK4819_REG0A_INPUT_PIN0 >> pin;

    const bool high = (bk4819_readReg(cfg->bk4819, 0x0A) & input) != 0;
    return high != ((cfg->activeLowMask & (1U << pin)) != 0);
}

const struct gpioApi gpioBK4819_ops = {
    .mode = &gpio_bk4819_mode,
    .set = &gpio_bk4819_set,
    .clear = &gpio_bk4819_clear,
    .read = &gpio_bk4819_read,
};
