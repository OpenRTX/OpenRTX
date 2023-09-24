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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <nvmem_access.h>
#include <interfaces/nvmem.h>
#include <interfaces/delays.h>


#define FLASH_TEST_ADDR  0x10000
#define EEPROM_TEST_ADDR 0x00000


static void printChunk(void *chunk, size_t len)
{
    printf("\n\r");

    uint8_t *ptr = ((uint8_t *) chunk);
    for(size_t j = 0; j < len / 16; j++)
    {
        for(size_t i = 0; i < 16; i++)
            printf("%02x ", ptr[j * 16 + i]);

        for(size_t i = 0; i < 16; i++)
        {
            if((ptr[i] > 0x22) && (ptr[i] < 0x7f)) printf("%c", ptr[j * 16 + i]);
            else printf(".");
        }

        printf("\n\r");
    }
}

static int flashTest(const struct nvmArea *nvm, const size_t address)
{
    const struct nvmParams *params = nvmArea_params(nvm);

    char *buffer1 = (char *) malloc(params->erase_size);
    char *buffer2 = (char *) malloc(params->erase_size);

    // Read one block
    int ret = nvmArea_read(nvm, address, buffer1, params->erase_size);
    printf("Flash area read returned value %d\r\n", ret);
    if(ret < 0)
        return ret;

    printChunk(buffer1, params->erase_size);

    // Erase one block
    ret = nvmArea_erase(nvm, address, params->erase_size);
    printf("Flash erase returned value %d\r\n", ret);
    if(ret < 0)
        return ret;

    // Read back to confirm Erase
    ret = nvmArea_read(nvm, address, buffer2, params->erase_size);
    if(ret < 0)
        return ret;

    for(size_t i = 0; i < params->erase_size; i++)
    {
        if (buffer2[i] != 0xff)
        {
            printf("Flash test fail! Offset %d was not erased!\r\n", i);
            printChunk(buffer2, params->erase_size);
            return -1;
        }
    }

    // Write one block
    ret = nvmArea_write(nvm, address, buffer1, params->erase_size);
    printf("Flash area write returned value %d\r\n", ret);
    if(ret < 0)
        return ret;

    // Read back to confirm Write
    ret = nvmArea_read(nvm, address, buffer2, params->erase_size);
    printf("Flash area readback returned value %d\r\n", ret);
    if(ret < 0)
        return ret;

    if(memcmp(buffer1, buffer2, params->erase_size))
    {
        puts("Error in flash device write, content to write and written differ!\r\n");
        printChunk(buffer2, params->erase_size);
        return -1;
    }

    puts("Flash test passed\r\n");
    return 0;
}

static int eepromTest(const struct nvmArea *nvm, const size_t address)
{
    uint8_t chunk[128];

    int ret = nvmArea_read(nvm, address, chunk, sizeof(chunk));
    printf("EEPROM read operation returned %d\r\n", ret);
    if(ret < 0)
        return ret;

    printChunk(chunk, sizeof(chunk));

    for(size_t i = 0; i < sizeof(chunk); i += 2)
    {
        chunk[i]     = 0x55;
        chunk[i + 1] = 0xAA;
    }

    ret = nvmArea_write(nvm, address, chunk, sizeof(chunk));
    printf("EEPROM write operation returned %d\r\n", ret);
    if(ret < 0)
        return ret;

    memset(chunk, 0x00, sizeof(chunk));
    ret = nvmArea_read(nvm, address, chunk, sizeof(chunk));
    printf("EEPROM readback operation returned %d\r\n", ret);
    if(ret < 0)
        return ret;

    for(size_t i = 0; i < sizeof(chunk); i += 2)
    {
        if((chunk[i] != 0x55) || (chunk[i + 1] != 0xAA))
        {
            printf("Check error around index %d: memory[i] = %02x, memory[i + 1] = %02x\r\n",
                   i, chunk[i], chunk[i + 1]);
            return -1;
        }
    }

    puts("EEPROM test passed\r\n");
    return 0;
}

int main()
{
    platform_init();
    delayMs(2000);

    // This is generally called in platform_init()
    nvm_init();

    const struct nvmArea   *nvm;
    size_t num = nvm_getMemoryAreas(&nvm);

    if(num == 0)
    {
        puts("Error: no NVM areas available\r\n");
        while(1) ;
    }

    for(size_t i = 0; i < num; i++)
    {
        const struct nvmParams *params = nvmArea_params(&nvm[i]);
        if(params->type == NVM_FLASH)
        {
            printf("Found flash area at index %d, performing test...\r\n", i);
            flashTest(&nvm[i], FLASH_TEST_ADDR);
        }

        if(params->type == NVM_EEPROM)
        {
            printf("Found EEPROM area at index %d, performing test...\r\n", i);
            eepromTest(&nvm[i], EEPROM_TEST_ADDR);
        }
    }

    // Loop endlessly
    while(1) ;

    return 0;
}
