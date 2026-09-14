/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <hwconfig.h>
#include "drivers/GPIO/gpio_zephyr.h"

/*
 * Pin mapping, must match platform/targets/c62/c62.dts:
 * BK4819: SCLK gpioa 13, SDATA gpioa 7, SCN gpioa 8 (active low)
 * BK1080: SCLK gpioa 5, SDATA gpioa 6, PWR gpiob 2 (active low)
 */
const struct BK4819 c62_bk4819 = { .sck = { &GpioA, 13 },
                                   .sda = { &GpioA, 7 },
                                   .scn = { &GpioA, 8 } };

const struct BK1080 c62_bk1080 = { .sck = { &GpioA, 5 },
                                   .sda = { &GpioA, 6 },
                                   .pwr = { &GpioB, 2 } };
