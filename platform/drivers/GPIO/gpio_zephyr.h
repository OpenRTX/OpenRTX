/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GPIO_ZEPHYR_H
#define GPIO_ZEPHYR_H

#include "peripherals/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * OpenRTX gpioDev instances backed by Zephyr GPIO controllers.
 *
 * These allow generic OpenRTX drivers, based on the gpioPin API, to run on
 * Zephyr targets such as the Retevis C62 (CSK6011B), where GPIOs are owned
 * by the Zephyr device model.
 *
 * Pin numbers match the SoC GPIO numbering, e.g. ``{ &GpioA, 13 }`` is
 * GPIOA pin 13. They must be kept in sync with the board devicetree.
 */
extern const struct gpioDev GpioA;
extern const struct gpioDev GpioB;

#ifdef __cplusplus
}
#endif

#endif /* GPIO_ZEPHYR_H */
