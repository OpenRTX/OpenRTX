/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Bring-up GPIO driver for the HR_C7000 CSKY V2 SoC.  Minimal vtable
 * impl backing OpenRTX's gpioPin_* indirect API.  Same functions serve
 * all three GPIO banks (A/B/C) — the bank base lives in dev->priv.
 * Not preemption-safe: read-modify-write on DR/DDR is unguarded.
 */

#include "drivers/gpio_csky.h"
#include <stdint.h>

#define GPIOA_BASE   0x14020000u
#define GPIOB_BASE   0x14100000u
#define GPIOC_BASE   0x14110000u

#define DR_OFFSET    0x00u
#define DDR_OFFSET   0x04u
#define DIN_OFFSET   0x50u

#define BANK_REG(base, off)  (*(volatile uint32_t *)((uintptr_t)(base) + (off)))

static int gpio_mode(const struct gpioDev *dev, const uint8_t pin,
                     const uint16_t mode)
{
    uintptr_t base = (uintptr_t)dev->priv;
    const uint32_t bit = 1u << pin;

    switch (mode & 0xff) {
        case OUTPUT:
        case OPEN_DRAIN:
        case OPEN_DRAIN_PU:
        case ALTERNATE:
        case ALTERNATE_OD:
        case ALTERNATE_OD_PU:
            BANK_REG(base, DDR_OFFSET) |= bit;
            break;
        default:
            BANK_REG(base, DDR_OFFSET) &= ~bit;
            break;
    }

    return 0;
}

static void gpio_set(const struct gpioDev *dev, const uint8_t pin)
{
    BANK_REG((uintptr_t)dev->priv, DR_OFFSET) |= (1u << pin);
}

static void gpio_clear(const struct gpioDev *dev, const uint8_t pin)
{
    BANK_REG((uintptr_t)dev->priv, DR_OFFSET) &= ~(1u << pin);
}

static bool gpio_read(const struct gpioDev *dev, const uint8_t pin)
{
    return (BANK_REG((uintptr_t)dev->priv, DIN_OFFSET) & (1u << pin)) != 0;
}

static const struct gpioApi gpio_api =
{
    .mode  = gpio_mode,
    .set   = gpio_set,
    .clear = gpio_clear,
    .read  = gpio_read,
};

const struct gpioDev hd2_gpioa_dev =
{
    .api  = &gpio_api,
    .priv = (void *)(uintptr_t)GPIOA_BASE,
};

const struct gpioDev hd2_gpioc_dev =
{
    .api  = &gpio_api,
    .priv = (void *)(uintptr_t)GPIOC_BASE,
};
