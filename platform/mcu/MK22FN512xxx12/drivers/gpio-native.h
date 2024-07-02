/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef GPIO_NATIVE_H
#define GPIO_NATIVE_H

#include <peripherals/gpio.h>
#include <hwconfig.h>
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

#endif /* GPIO_NATIVE_H */
