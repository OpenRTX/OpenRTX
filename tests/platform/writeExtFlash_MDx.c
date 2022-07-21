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

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include "platform/drivers/NVM/W25Qx.h"

//static const uint32_t sector_address = 0x80000;
static const uint32_t block_address  = 0x3f000;


uint8_t block[256] = {0};

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

    while(1)
    {
        getchar();

        printf("Attempting write... ");

        for(size_t i = 0; i < 256; i++)
        {
            //block[i] = 'a' + (i % 16);
            block[i] = 0x00;
        }


        for(uint32_t i = 0; i < 16; i++)
            W25Qx_writePage(block_address+i * 256, block, 256);

        for(uint32_t pos = 0; pos < 0x1000; pos += 16)
        {
            uint8_t buf[16];
            (void) W25Qx_readData(block_address + pos, buf, 16);
            printf("\r\n%02lx: ", block_address + pos);
            printChunk(buf);
        }

        //printf("\r\n\r\nAttempting erase... ");
        //bool ok = W25Qx_eraseSector(sector_address);
        //printf("%d\r\n", ok);

        //for(uint32_t pos = 0; pos < 0xFF; pos += 16)
        //{
        //    uint8_t buf[16];
        //    (void) W25Qx_readData(block_address + pos, buf, 16);
        //    printf("\r\n%02lx: ", pos);
        //    printChunk(buf);
        //}
    }

    return 0;
}
