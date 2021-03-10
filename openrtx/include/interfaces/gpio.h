/***************************************************************************
 *   Copyright (C) 2020 by Silvano Seva IU2KWO                             *
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

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * GPIO functional modes.
 * For more details see microcontroller's reference manual.
 */
enum Mode
{
    INPUT              =  0, ///Floating Input
    INPUT_PULL_UP      =  1, ///Pullup   Input
    INPUT_PULL_DOWN    =  2, ///Pulldown Input
    INPUT_ANALOG       =  3, ///Analog   Input
    OUTPUT             =  4, ///Push Pull  Output
    OPEN_DRAIN         =  5, ///Open Drain Output
    ALTERNATE          =  6, ///Alternate function
    ALTERNATE_OD       =  7, ///Alternate Open Drain
};

/**
 * Maximum GPIO switching speed.
 * For more details see microcontroller's reference manual and datasheet.
 */
enum Speed
{
    LOW    = 0x0,   /// 2MHz for STM32
    MEDIUM = 0x1,   /// 25MHz for STM32
    FAST   = 0x2,   /// 50MHz for STM32
    HIGH   = 0x3    /// 100MHz for STM32
};

/**
 * Configure GPIO pin functional mode.
 * @param port: GPIO port, it has to be equal to GPIOA_BASE, GPIOB_BASE, ...
 * @param pin: GPIO pin number, between 0 and 15.
 * @param mode: GPIO functional mode to be set.
 */
void gpio_setMode(void *port, uint8_t pin, enum Mode mode);

/**
 * Map alternate function to GPIO pin. The pin has to be configured in alternate
 * mode by calling 'gpio_setMode'.
 * @param port: GPIO port, it has to be equal to GPIOA_BASE, GPIOB_BASE, ...
 * @param pin: GPIO pin number, between 0 and 15.
 * @param afNum: alternate function number, retrieved from mapping table in
 * microcontroller's datasheet.
 */
void gpio_setAlternateFunction(void *port, uint8_t pin, uint8_t afNum);

/**
 * Configure GPIO pin maximum output speed.
 * @param port: GPIO port, it has to be equal to GPIOA_BASE, GPIOB_BASE, ...
 * @param pin: GPIO pin number, between 0 and 15.
 * @param spd: GPIO output speed to be set.
 */
void gpio_setOutputSpeed(void *port, uint8_t pin, enum Speed spd);

/**
 * Set GPIO pin to high logic level.
 * NOTE: this operation is performed atomically.
 * @param port: GPIO port, it has to be equal to GPIOA_BASE, GPIOB_BASE, ...
 * @param pin: GPIO pin number, between 0 and 15.
 */
void gpio_setPin(void *port, uint8_t pin);

/**
 * Set GPIO pin to low logic level.
 * NOTE: this operation is performed atomically.
 * @param port: GPIO port, it has to be equal to GPIOA_BASE, GPIOB_BASE, ...
 * @param pin: GPIO pin number, between 0 and 15.
 */
void gpio_clearPin(void *port, uint8_t pin);

/**
 * Toggle logic level of a GPIO pin, with respect to its state before calling
 * this function.
 * @param port: GPIO port, it has to be equal to GPIOA_BASE, GPIOB_BASE, ...
 * @param pin: GPIO pin number, between 0 and 15.
 */
void gpio_togglePin(void *port, uint8_t pin);

/**
 * Read GPIO pin's logic level.
 * @param port: GPIO port, it has to be equal to GPIOA_BASE, GPIOB_BASE, ...
 * @param pin: GPIO pin number, between 0 and 15.
 * @return 1 if pin is at high logic level, 0 if pin is at low logic level.
 */
uint8_t gpio_readPin(const void *port, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */
