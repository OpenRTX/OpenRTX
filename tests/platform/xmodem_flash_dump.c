/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <stdio.h>
#include <string.h>
#include <xmodem.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include "W25Qx.h"

static const size_t FLASH_SIZE = 16*1024*1024;  /* 16MB */
size_t addr = 0;

int callback(uint8_t *ptr, size_t size)
{
    if((addr + size) > FLASH_SIZE) return -1;
    W25Qx_readData(addr, ptr, size);
    addr += size;
    return 0;
}


int main()
{
    platform_init();
    W25Qx_init();
    W25Qx_wakeup();

    xmodem_sendData(FLASH_SIZE, callback);

    while(1)
    {
        platform_ledOn(GREEN);
        sleepFor(1,0);
        platform_ledOff(GREEN);
        sleepFor(1,0);
    }

    return 0;
}
