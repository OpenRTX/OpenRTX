/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GPIO_MK22_H
#define GPIO_MK22_H

#include "peripherals/gpio.h"
#include "hwconfig.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This file provides the interface for MK22 gpio.
 */

/**
 * Maximum GPIO switching speed.
 * For more details see microcontroller's reference manual and datasheet.
 */
enum Speed
{
    SLOW   = 0x0,   ///< 2MHz
    FAST   = 0x1    ///< 50MHz
};

/**
 * MK22 gpio devices
 */
extern const struct gpioDev GpioA;
extern const struct gpioDev GpioB;
extern const struct gpioDev GpioC;
extern const struct gpioDev GpioD;
extern const struct gpioDev GpioE;

/**
 * Configure gpio pin functional mode.
 *
 * @param port: gpio port.
 * @param pin: gpio pin number, between 0 and 15.
 * @param mode: bit 7:0 set gpio functional mode, bit 15:8 manage gpio alternate
 * function mapping.
 */
void gpio_setMode(const void *port, const uint8_t pin, const uint16_t mode);

/**
 * Configure gpio pin maximum output speed.
 *
 * @param port: gpio port.
 * @param pin: gpio pin number, between 0 and 15.
 * @param spd: gpio output speed to be set.
 */
void gpio_setOutputSpeed(const void *port, const uint8_t pin, const enum Speed spd);

/**
 * Set gpio pin to high logic level.
 * NOTE: this operation is performed atomically.
 *
 * @param port: gpio port.
 * @param pin: gpio pin number, between 0 and 15.
 */
static inline void gpio_setPin(const void *port, const uint8_t pin)
{
    ((GPIO_Type *)(port))->PSOR = (1 << pin);
}

/**
 * Set gpio pin to low logic level.
 * NOTE: this operation is performed atomically.
 *
 * @param port: gpio port.
 * @param pin: gpio pin number, between 0 and 15.
 */
static inline void gpio_clearPin(const void *port, const uint8_t pin)
{
    ((GPIO_Type *)(port))->PCOR = (1 << pin);
}

/**
 * Read gpio pin's logic level.
 *
 * @param port: gpio port.
 * @param pin: gpio pin number, between 0 and 15.
 * @return 1 if pin is at high logic level, 0 if pin is at low logic level.
 */
static inline uint8_t gpio_readPin(const void *port, const uint8_t pin)
{
    GPIO_Type *g = (GPIO_Type *)(port);
    return ((g->PDIR & (1 << pin)) != 0) ? 1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* GPIO_MK22_H */
