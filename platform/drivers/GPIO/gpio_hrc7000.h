/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GPIO_HRC7000_H
#define GPIO_HRC7000_H

#include "peripherals/gpio.h"
#include "registers.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Native GPIO driver for the HR_C7000 (Ailunce HD2) DesignWare DW_apb_gpio
 * banks GPIOA/GPIOB/GPIOC (see GPIO_TypeDef in registers.h): DR is the output
 * latch, DDR the direction register (1 = output), EXT_PORT the input read-back.
 *
 * Standard pin-function muxing is handled by the SOCSYS pad-mux, not the
 * per-GPIO registers, so gpio_setMode() covers only direction and level (see
 * it for how ALTERNATE modes are treated).  The one exception is the vendor
 * "alternate-function B" selector (per-bank register at +0x78, outside the
 * DW_apb_gpio map); use gpio_setAltFuncB() for it.
 */

/**
 * Configure gpio pin functional mode.
 *
 * @param port: gpio port (GPIOA/GPIOB/GPIOC).
 * @param pin: gpio pin number, between 0 and 31.
 * @param mode: functional mode (bit 7:0); the alternate-function field (bit
 * 15:8) is ignored -- this SoC muxes pin functions via the SOCSYS pad-mux.
 */
void gpio_setMode(const void *port, const uint8_t pin, const uint16_t mode);

/**
 * Set gpio pin to high logic level.
 *
 * @param port: gpio port.
 * @param pin: gpio pin number.
 */
static inline void gpio_setPin(const void *port, const uint8_t pin)
{
    ((GPIO_TypeDef *)port)->DR |= (1u << pin);
}

/**
 * Set gpio pin to low logic level.
 *
 * @param port: gpio port.
 * @param pin: gpio pin number.
 */
static inline void gpio_clearPin(const void *port, const uint8_t pin)
{
    ((GPIO_TypeDef *)port)->DR &= ~(1u << pin);
}

/**
 * Read gpio pin's logic level.
 *
 * @param port: gpio port.
 * @param pin: gpio pin number.
 * @return 1 if pin is at high logic level, 0 if pin is at low logic level.
 */
static inline uint8_t gpio_readPin(const void *port, const uint8_t pin)
{
    return (((GPIO_TypeDef *)port)->EXT_PORT >> pin) & 1u;
}

/**
 * Select the HR_C7000 vendor "alternate-function B" for a pin (per-bank
 * register at +0x78).  This is distinct from the SOCSYS pad-mux and is sticky
 * across gpio_setMode() direction changes; the keypad-matrix rows use it to
 * sense while the same pins otherwise carry LCD-bus signals.
 *
 * @param port: gpio port (GPIOA/GPIOB/GPIOC).
 * @param pin: gpio pin number, between 0 and 31.
 */
static inline void gpio_setAltFuncB(const void *port, const uint8_t pin)
{
    ((GPIO_TypeDef *)port)->ALT_FUNC_B |= (1u << pin);
}

#ifdef __cplusplus
}
#endif

#endif /* GPIO_HRC7000_H */
