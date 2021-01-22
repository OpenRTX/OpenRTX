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

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <W25Qx.h>

void printChunk(void *chunk)
{
    uint8_t *ptr = ((uint8_t *) chunk);
    for(size_t i = 0; i < 16; i++) printf("%02x ", ptr[i]);
    for(size_t i = 0; i < 16; i++)
    {
        if((ptr[i] > 0x22) && (ptr[i] < 0x7f)) printf("%c", ptr[i]);
        else printf(".");
    }
}

void printSecurityRegister(uint32_t reg)
{
    uint8_t secRegister[256] = {0};
    W25Qx_wakeup();
    W25Qx_readSecurityRegister(reg, secRegister, 256);
    W25Qx_sleep();

    for(uint32_t addr = 0; addr < 256; addr += 16)
    {
        printf("\r\n%02lx: ", addr);
        printChunk(&(secRegister[addr]));
    }
}

int main()
{
    W25Qx_init();

    while(1)
    {
        getchar();

        printf("0x1000:");
        printSecurityRegister(0x1000);

        printf("\r\n\r\n0x2000:");
        printSecurityRegister(0x2000);

        printf("\r\n\r\n0x3000:");
        printSecurityRegister(0x3000);
    }

    return 0;
}
