/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/graphics.h"
#include "hwconfig.h"
#include "drivers/NVM/flash_stm32.h"
#include "core/nvmem_access.h"
#include "interfaces/delays.h"
#include <errno.h>
#include "flash.h"

/**
 * This test is made to test the implementation of the STM32Flash nvm driver.
 * It will mess-up the data stored in flash, use at your own risk!!
 * 
 */

 int main(void)
{
    platform_init();
    sleepFor(0, 500u);
    printf("Starting Flash tests\r\n");
    gfx_init();
    gfx_clearScreen();
    
    point_t pos_line = {2, 12};
    const color_t color_white = {255, 255, 255, 255};
    gfx_print(pos_line, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
            color_white, "Testing NVM flash driver");
    pos_line.y += 12;

    const uint32_t mem_size = flash_size();
    gfx_print(pos_line, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
            color_white, "Flash size: %d kB", mem_size);
    pos_line.y += 12;

    gfx_render();  

    STM32_FLASH_DEVICE_DEFINE(flash_driver_16k, SECTOR_16K)
    STM32_FLASH_DEVICE_DEFINE(flash_driver_64k, SECTOR_64K)
    STM32_FLASH_DEVICE_DEFINE(flash_driver_128k, SECTOR_128K)

    const size_t nb_tests = 29;
    uint8_t results[nb_tests];

    results[0] = (STM32Flash_init(&flash_driver_16k) == 0);
    results[1] = (STM32Flash_init(&flash_driver_64k) == 0);
    results[2] = (STM32Flash_init(&flash_driver_128k) == 0);
    
    // Erase the full allocated area
    const uint32_t base_address = 0x8040000;
    const size_t area_size = 0x40000; // 2 sectors of 128K
    results[3] = (nvm_devErase(&flash_driver_128k, base_address, area_size) == 0); 

    // Fill the full area with consecutive numbers (32 bits access)
    results[4] = true;
    for(size_t i = 0; i < area_size/4; i+=4)
    {
        uint32_t data[4] = {i, i+1, i+2, i+3};
        int ret = nvm_devWrite(&flash_driver_128k, base_address + (i*4), (void*)(data), sizeof(data));
        if(ret < 0)
        {
            printf("Test 5 failed: error %d while writing to flash.\r\n", ret);
            results[4] = false;
        }
    }

    // These tests should fail
    uint32_t buffer = 0;
    // Write in wrong area
    results[5] = (nvm_devWrite(&flash_driver_16k, FLASH_BASE + 4*16384, &buffer, sizeof(buffer)) == -EINVAL); // Write in 64K sector
    results[6] = (nvm_devWrite(&flash_driver_64k, FLASH_BASE + 16*16384, &buffer, sizeof(buffer)) == -EINVAL); // Write in 128K sector
    results[7] = (nvm_devWrite(&flash_driver_128k, FLASH_BASE, &buffer, sizeof(buffer)) == -EINVAL); // Write in 16K sector

    // Read in wrong area
    results[8] = (nvm_devRead(&flash_driver_16k, FLASH_BASE + 4*16384, &buffer, sizeof(buffer)) == -EINVAL); // Read in 64K sector
    results[9] = (nvm_devRead(&flash_driver_64k, FLASH_BASE + 16*16384, &buffer, sizeof(buffer)) == -EINVAL); // Read in 128K sector
    results[10] = (nvm_devRead(&flash_driver_128k, FLASH_BASE, &buffer, sizeof(buffer)) == -EINVAL); // Read in 16K sector

    // Erase in wrong area
    results[11] = (nvm_devErase(&flash_driver_16k, FLASH_BASE + 4*16384, 65536) == -EINVAL); // Erase 64K sector
    results[12] = (nvm_devErase(&flash_driver_64k, FLASH_BASE + 8*16384, 131072) == -EINVAL); // Erase first 128K sector
    results[13] = (nvm_devErase(&flash_driver_128k, FLASH_BASE, 16384) == -EINVAL); // Erase first 16K sector

    // Erase less than a page
    results[14] = (nvm_devErase(&flash_driver_16k, FLASH_BASE, 8192) == -EINVAL); // Erase 8K
    results[15] = (nvm_devErase(&flash_driver_64k, FLASH_BASE + 4*16384, 16384) == -EINVAL); // Erase 16K
    results[16] = (nvm_devErase(&flash_driver_128k, FLASH_BASE + 8*16384, 16384) == -EINVAL); // Erase 16K

    // Erase misaligned address
    results[17] = (nvm_devErase(&flash_driver_16k, FLASH_BASE + 1, 16384) == -EINVAL); // Off-by-1
    results[18] = (nvm_devErase(&flash_driver_64k, FLASH_BASE + 4*16384 + 1, 65536) == -EINVAL); // off-by-1
    results[19] = (nvm_devErase(&flash_driver_128k, FLASH_BASE + 8*16384 + 1, 131072) == -EINVAL); // off-by-1

    // Go beyond device memory size
    uint32_t top_addr = FLASH_BASE + mem_size << 10;
    results[20] = (nvm_devRead(&flash_driver_128k, top_addr - 2, &buffer, sizeof(buffer)) == -EINVAL); // 
    results[21] = (nvm_devWrite(&flash_driver_128k, top_addr - 2, &buffer, sizeof(buffer)) == -EINVAL); // Write in 16K sector
    results[22] = (nvm_devErase(&flash_driver_128k, top_addr - 131072 , 2*131072) == -EINVAL); // off-by-1

    // Check that we still read consecutive numbers.
    results[23] = true;
    for(size_t i = 0; i < area_size/4; i++)
    {
        uint32_t number;
        nvm_devRead(&flash_driver_128k, base_address + i*4, &number, 4);
        if(number != i)
        {
            results[23] = false;
            printf("Test 24 failed at i=%d. Read-back %ld.\r\n", i, number);
        }
    }

    // Erase second sector of the area
    results[24] = (nvm_devErase(&flash_driver_128k,  base_address + 0x20000, 0x20000) == 0); 

    // Check that we still read consecutive numbers.
    results[25] = true;
    for(size_t i = 0; i < 0x20000/4; i++)
    {
        uint32_t number;
        nvm_devRead(&flash_driver_128k, base_address + (i*4), &number, 4);
        if(number != i)
        {
            results[25] = false;
            printf("Test 26 failed at i=%d. Read-back %ld.\r\n", i, number);
        }
    }

    // Check that we read 0xFFFFFFFF.
    results[26] = true;
    for(size_t i = 0x20000/4; i < 0x20000/2; i++)
    {
        uint32_t number;
        nvm_devRead(&flash_driver_128k, base_address + (i*4), &number, 4);
        if(number != 0Xffffffff)
        {
            results[26] = false;
            printf("Test 27 failed at i=%d. Read-back %ld.\r\n", i, number);
        }
    }    

    // Write consecutive numbers (8 bits access)
    results[27] = true;
    for(size_t i = 0; i < area_size/2; i++)
    {
        uint8_t data = (i%UINT8_MAX);
        int ret = nvm_devWrite(&flash_driver_128k, base_address + area_size/2 + i, (void*)(&data), sizeof(data));
        if(ret < 0)
        {
            printf("Test 28 failed: error %d while writing to flash.\r\n", ret);
            results[27] = false;
        }
        data++;
    }

    // Check that we still read consecutive numbers.
    results[28] = true;
    for(size_t i = 0; i < area_size/2; i++)
    {
        uint8_t number;
        nvm_devRead(&flash_driver_128k, base_address + area_size/2 + i, &number, 1);
        if(number != (i%UINT8_MAX) )
        {
            results[28] = false;
            printf("Test 29 failed at i=%d. Read-back %d.\r\n", i, number);
        }
    }

    int nb_failed = 0;
    point_t list_pt = pos_line;
    pos_line.y += 12;
    
    point_t output = gfx_print(list_pt, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
                        color_white, "Failed: ", nb_tests-nb_failed, nb_tests);
    list_pt.x += output.x;

    for(size_t i = 0; i < nb_tests; i++)
    {
        if(!results[i])
        {
            nb_failed++;
            output = gfx_print(list_pt, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
                        color_white, "%d ", i+1);
            list_pt.x += output.x;
        }
    }

    if(nb_failed == 0)
    {
        gfx_print(list_pt, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
            color_white, "none.");
    }

    gfx_print(pos_line, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
             color_white, "Passed %d tests out of %d.", nb_tests-nb_failed, nb_tests);
    pos_line.y += 12;

    gfx_render();
    while(1);
    return 0;
}
