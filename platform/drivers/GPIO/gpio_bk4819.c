/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/baseband/BK4819.h"
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
    const struct BK4819 *bk4819 = cfg->bk4819;
    const uint16_t direction_mask = 0x4000U >> pin;
    const uint16_t output_mask = 0x0040U >> pin;
    uint16_t set_mask = 0;
    uint16_t clear_mask = 0;

    switch (mode) {
        case INPUT:
            cfg->activeLowMask &= ~(1U << pin);
            set_mask = direction_mask;
            break;

        case OUTPUT:
            cfg->activeLowMask &= ~(1U << pin);
            clear_mask = direction_mask | output_mask;
            break;

        case OUTPUT_ACTIVE_LOW:
            cfg->activeLowMask |= 1U << pin;
            set_mask = output_mask;
            clear_mask = direction_mask;
            break;

        default:
            return -ENOTSUP;
    }

    uint16_t value = bk4819_readReg(bk4819, 0x33);
    value |= set_mask;
    value &= ~clear_mask;

    bk4819_writeReg(bk4819, 0x33, value);
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
    const struct BK4819 *bk4819 = cfg->bk4819;
    uint16_t value = bk4819_readReg(bk4819, 0x33);

    if (cfg->activeLowMask & (1U << pin))
        value &= ~(0x0040U >> pin);
    else
        value |= 0x0040U >> pin;
    bk4819_writeReg(bk4819, 0x33, value);
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
    const struct BK4819 *bk4819 = cfg->bk4819;
    uint16_t value = bk4819_readReg(bk4819, 0x33);

    if (cfg->activeLowMask & (1U << pin))
        value |= 0x0040U >> pin;
    else
        value &= ~(0x0040U >> pin);
    bk4819_writeReg(bk4819, 0x33, value);
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
    const struct BK4819 *bk4819 = cfg->bk4819;

    const bool high = (bk4819_readReg(bk4819, 0x0A) & (0x0040U >> pin)) != 0;
    return high != ((cfg->activeLowMask & (1U << pin)) != 0);
}

const struct gpioApi gpioBK4819_ops = { .mode = &gpio_bk4819_mode,
                                        .set = &gpio_bk4819_set,
                                        .clear = &gpio_bk4819_clear,
                                        .read = &gpio_bk4819_read };
