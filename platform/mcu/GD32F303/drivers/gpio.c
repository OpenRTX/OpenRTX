/***************************************************************************
 *   Copyright (C) 2023 by Silvano Seva IU2KWO                             *
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

#include <gd32f30x.h>
#include <peripherals/gpio.h>
#include <gpio-native.h>

void gpio_setMode(const void *port, const uint8_t pin, const uint16_t mode)
{
    uint16_t newMode = 0;
    switch(mode)
    {
        case INPUT:
            // (CFGR=00 OMODE=0 PULL=00)
            newMode = GPIO_MODE_IN_FLOATING;
            break;

        case INPUT_PULL_UP:
            // (MODE=00 TYPE=0 PUP=01)
            newMode = GPIO_MODE_IPU;
            break;

        case INPUT_PULL_DOWN:
            // (MODE=00 TYPE=0 PUP=10)
            newMode = GPIO_MODE_IPD;
            break;

        case ANALOG:
            // (MODE=11 TYPE=0 PUP=00)
            newMode = GPIO_MODE_AIN;
            break;

        case OUTPUT:
            // (MODE=01 TYPE=0 PUP=00)
            newMode = GPIO_MODE_OUT_PP;
            break;

        case OPEN_DRAIN:
            // (MODE=01 TYPE=1 PUP=00)
            newMode = GPIO_MODE_OUT_OD;
            break;

        case ALTERNATE:
            // (MODE=10 TYPE=0 PUP=00)
            newMode = GPIO_MODE_AF_PP;
            break;

        case ALTERNATE_OD:
            // (MODE=10 TYPE=1 PUP=00)
            newMode = GPIO_MODE_AF_OD;
            break;

        default:
            // Default to INPUT mode
            newMode = GPIO_MODE_IN_FLOATING;
            break;
    }
    gpio_init((uint32_t)port, newMode, GPIO_OSPEED_50MHZ, 1 << pin);
}

void gpio_setAlternateFunction(void *port, uint8_t pin, uint8_t afNum)
{
   gpio_init((uint32_t)port, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, 1 << pin);
}

void gpio_setOutputSpeed(const void *port, const uint8_t pin, const enum Speed spd)
{
    uint32_t speed = 0;
    // low medium fast high
    switch(spd)
    {
        case LOW:
            speed = GPIO_OSPEED_2MHZ;
            break;

        case MEDIUM:
            speed = GPIO_OSPEED_10MHZ;
            break;

        case FAST:
            speed = GPIO_OSPEED_50MHZ;
            break;

        case HIGH:
            speed = GPIO_OSPEED_MAX;
            break;

        default:
            speed = GPIO_OSPEED_50MHZ;
            break;
    }
    gpio_init((uint32_t)port, GPIO_MODE_OUT_PP, speed, 1 << pin);
}