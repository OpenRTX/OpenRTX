/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Frederik Saraci IU2NRO                   *
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

#include <interfaces/gpio.h>
#include <stdio.h>

void gpio_setMode(void *port, uint8_t pin, enum Mode mode)
{
    printf("gpio_setMode(%s, %u, %u)\n", (char*) port, pin, mode);
}

void gpio_setAlternateFunction(void *port, uint8_t pin, uint8_t afNum)
{
    printf("gpio_setAlternateFunction(%s, %u, %u)\n", (char*) port, pin, afNum);
}

void gpio_setOutputSpeed(void *port, uint8_t pin, enum Speed spd)
{
    printf("gpio_setOutputSpeed(%s, %u, %u)\n", (char*) port, pin, spd);
}

void gpio_setPin(void *port, uint8_t pin)
{
    printf("gpio_setPin(%s, %u)\n", (char*) port, pin);
}

void gpio_clearPin(void *port, uint8_t pin)
{
    printf("gpio_clearPin(%s, %u)\n", (char*) port, pin);
}

void gpio_togglePin(void *port, uint8_t pin)
{
    printf("gpio_togglePin(%s, %u)\n", (char*) port, pin);
}

uint8_t gpio_readPin(const void *port, uint8_t pin)
{
    printf("gpio_readPin(%s, %u)\n", (char*) port, pin);
    return 1;
}
