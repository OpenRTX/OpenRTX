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
 * STM32H743 is used in the CS7000-plus. This radio is dual-boot and OpenRTX is
 * designed to live from adress 0x8100000 (bank 2).
 *
 */

 int main(void)
{
    platform_init();
    sleepFor(0, 500u);
    printf("Starting Flash tests\r\n");
    gfx_init();
    gfx_clearScreen();
    display_setBacklightLevel(255);


    point_t pos_line = {2, 12};
    const color_t color_white = {255, 255, 255, 255};
    gfx_print(pos_line, FONT_SIZE_6PT, TEXT_ALIGN_LEFT,
            color_white, "Testing NVM flash driver");
    pos_line.y += 12;

    const uint32_t mem_size = flash_size();
    gfx_print(pos_line, FONT_SIZE_6PT, TEXT_ALIGN_LEFT,
            color_white, "Flash size: %d kB", mem_size);
    pos_line.y += 12;

    gfx_render();

    STM32_FLASH_DEVICE_DEFINE(flash_driver_128k, SECTOR_128K)

    const size_t nb_tests = 15;
    uint8_t results[nb_tests];

    results[0] = (STM32Flash_init(&flash_driver_128k) == 0);

    // Erase the full allocated area
    const uint32_t sector11 = 0x8100000 + 2*0x20000; // 3rd sector of the OpenRTX area, 11th sector
    const size_t area_size = 0x40000; // 2 sectors of 128K

    results[1] = (nvm_devErase(&flash_driver_128k, sector11, area_size) == 0);

    // Fill the full area with consecutive numbers (32 bits numbers, 32 bytes access)
    results[2] = true;
    for(size_t i = 0; i < area_size/4; i+=8)
    {
        uint32_t data[8] = {i, i+1, i+2, i+3, i+4, i+5, i+6, i+7};
        int ret = nvm_devWrite(&flash_driver_128k, sector11 + (i*4), (void*)(data), sizeof(data));
        if(ret < 0)
        {
            printf("Test 3 failed: error %d while writing to flash.\r\n", ret);
            results[2] = false;
        }
    }

    uint32_t buffer[8] = {0};

    // These tests should fail
    // Write less than 32 bytes
    results[4] = (nvm_devWrite(&flash_driver_128k, sector11, &buffer, sizeof(buffer)/2) == -EINVAL);

    // Misaligned write
    results[5] = (nvm_devWrite(&flash_driver_128k, sector11 + 4, &buffer, sizeof(buffer)) == -EINVAL);

    // Erase less than a page
    results[6] = (nvm_devErase(&flash_driver_128k, sector11, 16384) == -EINVAL); // Erase 16K

    // Erase misaligned address
    results[7] = (nvm_devErase(&flash_driver_128k, sector11 + 1, 131072) == -EINVAL); // off-by-1

    // Go beyond device memory size
    uint32_t top_addr = FLASH_BASE + (mem_size << 10);
    results[8] = (nvm_devRead(&flash_driver_128k, top_addr - 2, &buffer, sizeof(buffer)) == -EINVAL);
    results[9] = (nvm_devWrite(&flash_driver_128k, top_addr - 2, &buffer, sizeof(buffer)) == -EINVAL);
    results[10] = (nvm_devErase(&flash_driver_128k, top_addr - 131072 , 2*131072) == -EINVAL);

    // Check that we read consecutive numbers.
    results[11] = true;
    for(size_t i = 0; i < area_size/4; i++)
    {
        uint32_t number;
        int ret = nvm_devRead(&flash_driver_128k, sector11 + i*4, &number, 4);
        if(ret < 0)
        {
            results[11] = false;
            printf("Test 12 failed at i=%d. Read returned %ld.\r\n", i, ret);
            break;
        }
        if(number != i)
        {
            results[11] = false;
            printf("Test 12 failed at i=%d. Read-back %ld.\r\n", i, number);
            break;
        }
    }

    // Erase second sector of the area
    results[12] = (nvm_devErase(&flash_driver_128k,  sector11 + 0x20000, 0x20000) == 0);

    // Check that we still read consecutive numbers.
    results[13] = true;
    for(size_t i = 0; i < 0x20000/4; i++)
    {
        uint32_t number;
        nvm_devRead(&flash_driver_128k, sector11 + (i*4), &number, 4);
        if(number != i)
        {
            results[13] = false;
            printf("Test 14 failed at i=%d. Read-back %ld.\r\n", i, number);
        }
    }

    // Check that we read 0xFFFFFFFF.
    results[14] = true;
    for(size_t i = 0x20000/4; i < 0x20000/2; i++)
    {
        uint32_t number;
        nvm_devRead(&flash_driver_128k, sector11 + (i*4), &number, 4);
        if(number != 0Xffffffff)
        {
            results[14] = false;
            printf("Test 15 failed at i=%d. Read-back %ld.\r\n", i, number);
        }
    }

    int nb_failed = 0;
    point_t list_pt = pos_line;
    pos_line.y += 12;

    point_t output = gfx_print(list_pt, FONT_SIZE_6PT, TEXT_ALIGN_LEFT,
                        color_white, "Failed: ", nb_tests-nb_failed, nb_tests);
    list_pt.x += output.x;

    for(size_t i = 0; i < nb_tests; i++)
    {
        if(!results[i])
        {
            nb_failed++;
            output = gfx_print(list_pt, FONT_SIZE_6PT, TEXT_ALIGN_LEFT,
                        color_white, "%d ", i+1);
            list_pt.x += output.x;
        }
    }

    if(nb_failed == 0)
    {
        gfx_print(list_pt, FONT_SIZE_6PT, TEXT_ALIGN_LEFT,
            color_white, "none.");
    }

    gfx_print(pos_line, FONT_SIZE_6PT, TEXT_ALIGN_LEFT,
             color_white, "Passed %d tests out of %d.", nb_tests-nb_failed, nb_tests);
    pos_line.y += 12;

    gfx_render();
    while(1);
    return 0;
}
