/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Silvano Seva IU2KWO                      *
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

#include "stm32f4xx.h"
#include <peripherals/gpio.h>

void gpio_setMode(void *port, uint8_t pin, enum Mode mode)
{
    GPIO_TypeDef *p = (GPIO_TypeDef *)(port);
    p->MODER  &= ~(3 << (pin*2));
    p->OTYPER &= ~(1 << pin);
    p->PUPDR  &= ~(3 << (pin*2));

    switch(mode)
    {
        case INPUT:
            // (MODE=00 TYPE=0 PUP=00)
            p->MODER  |= 0x00 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;

        case INPUT_PULL_UP:
            // (MODE=00 TYPE=0 PUP=01)
            p->MODER  |= 0x00 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x01 << (pin*2);
            break;

        case INPUT_PULL_DOWN:
            // (MODE=00 TYPE=0 PUP=10)
            p->MODER  |= 0x00 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x02 << (pin*2);
            break;

        case INPUT_ANALOG:
            // (MODE=11 TYPE=0 PUP=00)
            p->MODER  |= 0x03 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;

        case OUTPUT:
            // (MODE=01 TYPE=0 PUP=00)
            p->MODER  |= 0x01 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;

        case OPEN_DRAIN:
            // (MODE=01 TYPE=1 PUP=00)
            p->MODER  |= 0x01 << (pin*2);
            p->OTYPER |= 0x01 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;

        case ALTERNATE:
            // (MODE=10 TYPE=0 PUP=00)
            p->MODER  |= 0x02 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;

        case ALTERNATE_OD:
            // (MODE=10 TYPE=1 PUP=00)
            p->MODER  |= 0x02 << (pin*2);
            p->OTYPER |= 0x01 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;

        default:
            // Default to INPUT mode
            p->MODER  |= 0x00 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;
    }
}

void gpio_setAlternateFunction(void *port, uint8_t pin, uint8_t afNum)
{
    GPIO_TypeDef *p = (GPIO_TypeDef *)(port);
    afNum &= 0x0F;
    if(pin < 8)
    {
        p->AFR[0] &= ~(0x0F << (pin*4));
        p->AFR[0] |= (afNum << (pin*4));
    }
    else
    {
        pin -= 8;
        p->AFR[1] &= ~(0x0F << (pin*4));
        p->AFR[1] |= (afNum << (pin*4));
    }
}

void gpio_setOutputSpeed(void *port, uint8_t pin, enum Speed spd)
{
    ((GPIO_TypeDef *)(port))->OSPEEDR &= ~(3 << (pin*2));   // Clear old value
    ((GPIO_TypeDef *)(port))->OSPEEDR |= spd << (pin*2);    // Set new value
}

void gpio_setPin(void *port, uint8_t pin)
{
    ((GPIO_TypeDef *)(port))->BSRRL = (1 << pin);
}

void gpio_clearPin(void *port, uint8_t pin)
{
    ((GPIO_TypeDef *)(port))->BSRRH = (1 << pin);
}

void gpio_togglePin(void *port, uint8_t pin)
{
    ((GPIO_TypeDef *)(port))->ODR ^= (1 << pin);
}

uint8_t gpio_readPin(const void *port, uint8_t pin)
{
    GPIO_TypeDef *p = (GPIO_TypeDef *)(port);
    return ((p->IDR & (1 << pin)) != 0) ? 1 : 0;
}
