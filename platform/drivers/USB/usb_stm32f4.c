/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#include <stdint.h>
#include <stdlib.h>
#include <interfaces/gpio.h>
#include <interfaces/platform.h>
#include "hwconfig.h"

void usb_init()
{
    gpio_setMode(GPIOA, 11, ALTERNATE);
    gpio_setAlternateFunction(GPIOA, 11, 10);
    gpio_setOutputSpeed(GPIOA, 11, HIGH);      // 100MHz output speed

    gpio_setMode(GPIOA, 12, ALTERNATE);
    gpio_setAlternateFunction(GPIOA, 12, 10);
    gpio_setOutputSpeed(GPIOA, 12, HIGH);      // 100MHz output speed
}

void usb_terminate()
{
}
