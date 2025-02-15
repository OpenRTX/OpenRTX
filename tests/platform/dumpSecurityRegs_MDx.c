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
#include <stdint.h>
#include <sys/types.h>
#include <W25Qx.h>
#include <nvmem_access.h>
#include <interfaces/delays.h>

#define SECREG_READ(dev, data, len) \
    nvm_devRead((const struct nvmDevice *) dev, 0x0000, data, len)

extern const struct W25QxSecRegDevice cal1;
extern const struct W25QxSecRegDevice cal2;
extern const struct W25QxSecRegDevice hwInfo;


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

void printSecurityRegister(const struct W25QxSecRegDevice dev)
{
    uint8_t secRegister[256] = {0};
    SECREG_READ(&dev, secRegister, 256);

    for(uint32_t addr = 0; addr < 256; addr += 16)
    {
        printf("\r\n%02lx: ", addr);
        printChunk(&(secRegister[addr]));
    }
}

int main()
{

    while(1)
    {
        delayMs(5000);

        printf("\r\n\r\n0x1000:");
        printSecurityRegister(cal1);

        printf("\r\n\r\n0x2000:");
        printSecurityRegister(cal2);

        printf("\r\n\r\n0x3000:");
        printSecurityRegister(hwInfo);
    }

    return 0;
}
