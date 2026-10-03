/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/GPIO/gpio_zephyr.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

static int gpioZephyr_mode(const struct gpioDev *dev, const uint8_t pin,
                           const uint16_t mode)
{
    const struct device *zdev = (const struct device *)dev->priv;
    gpio_flags_t flags = 0;

    switch (mode & 0x00FF) {
        case INPUT:
            flags = GPIO_INPUT;
            break;

        case INPUT_PULL_UP:
            flags = GPIO_INPUT | GPIO_PULL_UP;
            break;

        case INPUT_PULL_DOWN:
            flags = GPIO_INPUT | GPIO_PULL_DOWN;
            break;

        case OUTPUT:
            flags = GPIO_OUTPUT;
            break;

        case OPEN_DRAIN:
            flags = GPIO_OUTPUT | GPIO_OPEN_DRAIN;
            break;

        case OPEN_DRAIN_PU:
            flags = GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_PULL_UP;
            break;

        default:
            return -ENOTSUP;
    }

    if (!device_is_ready(zdev))
        return -ENODEV;

    return gpio_pin_configure(zdev, pin, flags);
}

static void gpioZephyr_set(const struct gpioDev *dev, const uint8_t pin)
{
    const struct device *zdev = (const struct device *)dev->priv;
    (void)gpio_pin_set(zdev, pin, 1);
}

static void gpioZephyr_clear(const struct gpioDev *dev, const uint8_t pin)
{
    const struct device *zdev = (const struct device *)dev->priv;
    (void)gpio_pin_set(zdev, pin, 0);
}

static bool gpioZephyr_read(const struct gpioDev *dev, const uint8_t pin)
{
    const struct device *zdev = (const struct device *)dev->priv;
    return gpio_pin_get(zdev, pin) > 0;
}

static const struct gpioApi gpioZephyr_api = { .mode = &gpioZephyr_mode,
                                               .set = &gpioZephyr_set,
                                               .clear = &gpioZephyr_clear,
                                               .read = &gpioZephyr_read };

const struct gpioDev GpioA = { .api = &gpioZephyr_api,
                               .priv = DEVICE_DT_GET(DT_NODELABEL(gpioa)) };

const struct gpioDev GpioB = { .api = &gpioZephyr_api,
                               .priv = DEVICE_DT_GET(DT_NODELABEL(gpiob)) };
