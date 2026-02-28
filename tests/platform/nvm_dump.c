/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "core/nvmem_access.h"
#include "interfaces/nvmem.h"
#include "interfaces/delays.h"

#define LINE_LENGTH    16
#define CHUNK_SIZE     (LINE_LENGTH * 2)

static void printChunk(size_t offset, void *chunk, size_t len)
{
    uint8_t *ptr = ((uint8_t *) chunk);

    for(size_t j = 0; j < len / LINE_LENGTH; j++) {
        printf("%08x: ", offset + (j * LINE_LENGTH));

        for(size_t i = 0; i < LINE_LENGTH; i++)
            printf("%02x ", ptr[(j * LINE_LENGTH) + i]);

        for(size_t i = 0; i < LINE_LENGTH; i++) {
            char elem = (char) ptr[(j * LINE_LENGTH) + i];

            if((elem > 0x20) && (elem < 0x7f))
                putchar(elem);
            else
                putchar('.');
        }

        puts("\r");
    }
}

static void waitForPtt()
{
    // Wait until PTT is pressed
    while(platform_getPttStatus() == false) {
        platform_ledOn(GREEN);
        sleepFor(0, 500);
        platform_ledOff(GREEN);
        sleepFor(0, 500);
    }

    // Resume execution on PTT release
    platform_ledOn(RED);
    while(platform_getPttStatus() == true) ;
    platform_ledOff(RED);
}

static bool pttLongPress()
{
    static uint32_t timeout;
    bool ptt = platform_getPttStatus();

    if(ptt == false)
        timeout = 250;
    else
        timeout -= 1;

    return (timeout == 0);
}

int main()
{
    platform_init();
    nvm_init();

    // Wait for user to be ready
    waitForPtt();

    size_t idx = 0;
    const struct nvmDescriptor *nvm = nvm_getDesc(0);

    if(nvm == NULL) {
        puts("No NVM areas available, end!\r\n");

        while(1) {
            platform_ledOn(RED);
            sleepFor(0, 500);
            platform_ledOff(RED);
            sleepFor(0, 500);
        }
    }

    while(nvm != NULL) {
        printf("NVM device %d:\r\n", idx);
        printf("\t - Name:  %s\r\n", nvm->name);
        printf("\t - Size:  %d\r\n", nvm->size);
        printf("\t - Flags: %04lx\r\n", nvm->dev->info->device_info);
        puts("\r");

        idx += 1;
        nvm = nvm_getDesc(idx);
    }

    for(size_t i = 0; i < idx; i++) {
        nvm = nvm_getDesc(i);

        printf("\r\nNVM device %d: press PTT to dump\r\n", i);
        waitForPtt();

        for(size_t addr = 0; addr < nvm->size; addr += CHUNK_SIZE) {
            uint8_t chunk[CHUNK_SIZE];

            nvm_read(i, 0, addr, chunk, sizeof(chunk));
            printChunk(addr, chunk, sizeof(chunk));

            // Exit in case of PTT press
            if(pttLongPress() == true)
                break;
        }
    }

    // Loop endlessly
    while(1) {
        platform_ledOn(RED);
        sleepFor(0, 500);
        platform_ledOff(RED);
        sleepFor(0, 500);
    }

    return 0;
}
