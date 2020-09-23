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

#include "gpio.h"

void gpio_setMode(GPIO_TypeDef *port, uint8_t pin, enum Mode mode)
{
    port->MODER  &= ~(3 << (pin*2));
    port->OTYPER &= ~(1 << pin);
    port->PUPDR  &= ~(3 << (pin*2));

    switch(mode)
    {
        case INPUT:
            // (MODE=00 TYPE=0 PUP=00)
            port->MODER  |= 0x00 << (pin*2);
            port->OTYPER |= 0x00 << pin;
            port->PUPDR  |= 0x00 << (pin*2);
            break;

        case INPUT_PULL_UP:
            // (MODE=00 TYPE=0 PUP=01)
            port->MODER  |= 0x00 << (pin*2);
            port->OTYPER |= 0x00 << pin;
            port->PUPDR  |= 0x01 << (pin*2);
            break;

        case INPUT_PULL_DOWN:
            // (MODE=00 TYPE=0 PUP=10)
            port->MODER  |= 0x00 << (pin*2);
            port->OTYPER |= 0x00 << pin;
            port->PUPDR  |= 0x02 << (pin*2);
            break;

        case INPUT_ANALOG:
            // (MODE=11 TYPE=0 PUP=00)
            port->MODER  |= 0x03 << (pin*2);
            port->OTYPER |= 0x00 << pin;
            port->PUPDR  |= 0x00 << (pin*2);
            break;

        case OUTPUT:
            // (MODE=01 TYPE=0 PUP=00)
            port->MODER  |= 0x01 << (pin*2);
            port->OTYPER |= 0x00 << pin;
            port->PUPDR  |= 0x00 << (pin*2);
            break;

        case OPEN_DRAIN:
            // (MODE=01 TYPE=1 PUP=00)
            port->MODER  |= 0x01 << (pin*2);
            port->OTYPER |= 0x01 << pin;
            port->PUPDR  |= 0x00 << (pin*2);
            break;

        case ALTERNATE:
            // (MODE=10 TYPE=0 PUP=00)
            port->MODER  |= 0x02 << (pin*2);
            port->OTYPER |= 0x00 << pin;
            port->PUPDR  |= 0x00 << (pin*2);
            break;

        case ALTERNATE_OD:
            // (MODE=10 TYPE=1 PUP=00)
            port->MODER  |= 0x02 << (pin*2);
            port->OTYPER |= 0x01 << pin;
            port->PUPDR  |= 0x00 << (pin*2);
            break;

        default:
            // Default to INPUT mode
            port->MODER  |= 0x00 << (pin*2);
            port->OTYPER |= 0x00 << pin;
            port->PUPDR  |= 0x00 << (pin*2);
            break;
    }
}

void gpio_setAlternateFunction(GPIO_TypeDef *port, uint8_t pin, uint8_t afNum)
{
    afNum &= 0x0F;
    if(pin < 8)
    {
        port->AFR[0] &= ~(0x0F << (pin*4));
        port->AFR[0] |= (afNum << (pin*4));
    }
    else
    {
        pin -= 8;
        port->AFR[1] &= ~(0x0F << (pin*4));
        port->AFR[1] |= (afNum << (pin*4));
    }
}

void gpio_setOutputSpeed(GPIO_TypeDef *port, uint8_t pin, enum Speed spd)
{
    port->OSPEEDR &= ~(3 << (pin*2));   // Clear old value
    port->OSPEEDR |= spd << (pin*2);    // Set new value
}

void gpio_setPin(GPIO_TypeDef *port, uint8_t pin)
{
    port->BSRRL = (1 << pin);
}

void gpio_clearPin(GPIO_TypeDef *port, uint8_t pin)
{
    port->BSRRH = (1 << pin);
}

void gpio_togglePin(GPIO_TypeDef *port, uint8_t pin)
{
    port->ODR ^= (1 << pin);
}

uint8_t gpio_readPin(const GPIO_TypeDef *port, uint8_t pin)
{
    return ((port->IDR & (1 << pin)) != 0) ? 1 : 0;
}
