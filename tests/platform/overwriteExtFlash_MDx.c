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

  // #######  DISCLAIMER #########
  // This test overwrites a portion of the SPI flash memory
  // Run this only if you know what you are doing!

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include "W25Qx.h"

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

int main()
{
    W25Qx_init();
    W25Qx_wakeup();

    uint8_t testData[16] = {0};
    uint8_t buffer[256] = {0};
    
    while(1)
    {
        getchar();
        
        // On UV380 flash at 0x6000 there are 36032 bytes of 0xFF
        uint32_t addr = 0x6000;
        printf("Read memory @ 0x%x\r\n", addr);
        W25Qx_readData(addr, buffer, 256);
        for (int offset = 0; offset < 256; offset += 16)
        {
            printf("\r\n%lx: ", addr + offset);
            printChunk(buffer + offset);
        }
        // Prepare test data
        for(int i = 0; i < 16; i++)
        {
            testData[i] = 'a' + (i % 16);
        }
        printf("\r\nOverwrite memory @ 0x6000... ");
        bool success = W25Qx_writeData(addr, testData, 16);
        printf("%s\r\n", success ? "success" : "failed");

        printf("Read memory @ 0x%x\r\n", addr);
        W25Qx_readData(addr, buffer, 256);
        for (int offset = 0; offset < 256; offset += 16)
        {
            printf("\r\n%lx: ", addr + offset);
            printChunk(buffer + offset);
        }
        
        uint32_t blockAddr = addr / 4096 * 4096;
        printf("\r\nErase memory @ 0x%x... ", blockAddr);
        success = W25Qx_eraseSector(blockAddr);
        printf("%s\r\n", success ? "success" : "failed");
        
        printf("Read memory @ 0x%x\r\n", addr);
        W25Qx_readData(addr, buffer, 256);
        for (int offset = 0; offset < 256; offset += 16)
        {
            printf("\r\n%lx: ", addr + offset);
            printChunk(buffer + offset);
        }
        
        printf("\r\nWrite memory @ 0x%x... ", addr);
        success = W25Qx_writePage(addr, testData, 16);
        printf("%s\r\n", success ? "success" : "failed");
        
        printf("Read memory @ 0x%x\r\n", addr);
        W25Qx_readData(addr, buffer, 256);
        for (int offset = 0; offset < 256; offset += 16)
        {
            printf("\r\n%lx: ", addr + offset);
            printChunk(buffer + offset);
        }
    }

    return 0;
}
