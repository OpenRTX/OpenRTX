/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/backup.h"
#include "core/xmodem.h"
#include <string.h>
#include "drivers/NVM/W25Qx.h"

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
