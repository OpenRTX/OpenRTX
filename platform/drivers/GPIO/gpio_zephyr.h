/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GPIO_ZEPHYR_H
#define GPIO_ZEPHYR_H

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include "peripherals/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Wrapper of the Zephyr GPIO controllers, for the generic OpenRTX drivers
 * based on the gpioPin API: on Zephyr targets the GPIOs are owned by the
 * Zephyr device model.
 */
extern const struct gpioApi gpioZephyr_ops;

/**
 * Instantiate a GPIO device backed by a Zephyr GPIO controller. Pin numbers
 * are those of the controller.
 *
 * @param name: device name.
 * @param label: devicetree node label of the GPIO controller.
 */
#define GPIO_ZEPHYR_DEVICE_DEFINE(name, label)      \
    const struct gpioDev name = {                   \
        .api = &gpioZephyr_ops,                     \
        .priv = DEVICE_DT_GET(DT_NODELABEL(label)), \
    };

#ifdef __cplusplus
}
#endif

#endif /* GPIO_ZEPHYR_H */
