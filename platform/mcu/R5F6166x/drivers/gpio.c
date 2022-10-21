/***************************************************************************
 *   Copyright (C) 2022 by Silvano Seva IU2KWO                             *
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

#include "iodefine.h"
#include "typedefine.h"
#include <interfaces/gpio.h>

enum REGS
{
    PnDDR  = 0x000,    // Data direction register
    PnDR   = 0x3D0,    // Data register
    PnPORT = 0x3C0,    // Port register
    PnICR  = 0x010,    // Input buffer control register
    PnPCR  = 0x028,    // Pull-up MOS control register
    PnODR  = 0x03B     // Open-drain control register
};

#define REG(x,y) (((uint8_t *) x) + y)

void gpio_setMode(void *port, uint8_t pin, enum Mode mode)
{
    uint32_t p = (uint32_t) port;

    // GPIO port 5 is input-only
    if(p == (uint32_t)(&P5)) return;

    // Reset open-drain setting, only for P2 and PF
    if((p == ((uint32_t) &P2)) || (p == ((uint32_t) &PF)))
    {
        *REG(port, PnODR) &= ~(1 << pin);
    }

    // Reset pull-up setting, only for PD ... PK
    if((p >= ((uint32_t) &PD)) && (p <= ((uint32_t) &PK)))
    {
        *REG(port, PnPCR) &= ~(1 << pin);
    }

    switch(mode)
    {
        case INPUT:
            *REG(port, PnDDR) &= ~(1 << pin);
            break;

        case INPUT_PULL_UP:
            // Only PD ... PK have pull-up control register.
            if((p < ((uint32_t) &PD)) || (p > ((uint32_t) &PK))) return;

            *REG(port, PnDDR) &= ~(1 << pin);   // Input mode
            *REG(port, PnPCR) |=  (1 << pin);   // Enable pull-up
            break;

        case OUTPUT:
            *REG(port, PnDDR) |= (1 << pin);
            break;

        case OPEN_DRAIN:
            // Only P2 and PF have open drain capability.
            if((p != ((uint32_t) &P2)) && (p != ((uint32_t) &PF))) return;

            *REG(port, PnDDR) |= (1 << pin);    // Output mode
            *REG(port, PnODR) |= (1 << pin);    // Enable open-drain
            break;

        case INPUT_PULL_DOWN:
        case INPUT_ANALOG:
        case ALTERNATE:
        case ALTERNATE_OD:
        default:
            // Leave previous configuration
            break;
    }
}

void gpio_setAlternateFunction(void *port, uint8_t pin, uint8_t afNum)
{
    // This device does not have GPIO alternate function selection
    (void) port;
    (void) pin;
    (void) afNum;
}

void gpio_setOutputSpeed(void *port, uint8_t pin, enum Speed spd)
{
    // This device does not support setting of GPIO speed
    (void) port;
    (void) pin;
    (void) spd;
}

void gpio_setPin(void *port, uint8_t pin)
{
    // GPIO port 5 is input-only
    if((uint32_t)(port) == (uint32_t)(&P5)) return;

    *REG(port, PnDR) |= (1 << pin);
}

void gpio_clearPin(void *port, uint8_t pin)
{
    // GPIO port 5 is input-only
    if((uint32_t)(port) == (uint32_t)(&P5)) return;

    *REG(port, PnDR) &= ~(1 << pin);
}

void gpio_togglePin(void *port, uint8_t pin)
{
    // GPIO port 5 is input-only
    if((uint32_t)(port) == (uint32_t)(&P5)) return;

    *REG(port, PnDR) ^= (1 << pin);
}

uint8_t gpio_readPin(const void *port, uint8_t pin)
{
    return ((*REG(port, PnPORT)) >> pin) & 0x01;
}
