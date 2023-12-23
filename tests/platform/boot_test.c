/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <stdio.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <drivers/USART1.h>

int main()
{
    platform_init();
    usart1_init(115200);
    while(1)
    {
        GPIOB->cfgr_bit.iomc6 = 0x02; // PB6 Alternate function
        usart1_IRQwrite("Hello World!\r\n");
        platform_ledOn(GREEN);
        delayMs(1000);
        platform_ledOff(GREEN);
        delayMs(1000);
    }

    return 0;
}
