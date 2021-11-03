/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
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

#include <hwconfig.h>
#include <interfaces/delays.h>
#include <interfaces/gpio.h>
#include <interfaces/platform.h>
#include <stdio.h>

int main()
{
    platform_init();

    gpio_setMode(CH_SELECTOR_0, INPUT_PULL_UP);
    gpio_setMode(CH_SELECTOR_1, INPUT_PULL_UP);

    while (1)
    {
        iprintf("%d %d\r\n", gpio_readPin(CH_SELECTOR_0),
                gpio_readPin(CH_SELECTOR_1));
        delayMs(1000);
    }

    return 0;
}
