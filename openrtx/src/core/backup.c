/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#include <backup.h>
#include <xmodem.h>
#include <string.h>
#include "W25Qx.h"

#if defined(PLATFORM_GD77) || defined(PLATFORM_DM1801)
static const size_t EFLASH_SIZE = 1024*1024;    // 1 MB
#else
static const size_t EFLASH_SIZE = 16*1024*1024; // 16 MB
#endif

size_t  memAddr = 0;

static int getDataCallback(uint8_t *ptr, size_t size)
{
    if((memAddr + size) > EFLASH_SIZE) return -1;
    W25Qx_readData(memAddr, ptr, size);
    memAddr += size;
    return 0;
}

static void writeDataCallback(uint8_t *ptr, size_t size)
{
    // Trigger sector erase on each 4kB address boundary
    if((memAddr % 0x1000) == 0)
    {
        W25Qx_erase(memAddr, 0x1000);
    }

    for(size_t written = 0; written < size; )
    {
        size_t toWrite = size - written;
        if(toWrite > 256) toWrite = 256;
        W25Qx_writePage(memAddr, ptr, toWrite);
        written += toWrite;
        memAddr += toWrite;
        ptr     += toWrite;
    }
}

void eflash_dump()
{
    memAddr = 0;
    W25Qx_wakeup();
    xmodem_sendData(EFLASH_SIZE, getDataCallback);
}

void eflash_restore()
{
    memAddr = 0;
    W25Qx_wakeup();
    xmodem_receiveData(EFLASH_SIZE, writeDataCallback);
}
