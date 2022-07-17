/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#include "W25Qx.h"

#include <hwconfig.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "interfaces/delays.h"
#include "interfaces/gpio.h"

#define CMD_WRITE 0x02   /* Read data              */
#define CMD_READ  0x03   /* Read data              */
#define CMD_RDSTA 0x05   /* Read status register   */
#define CMD_WREN  0x06   /* Write enable           */
#define CMD_ESECT 0x20   /* Erase 4kB sector       */
#define CMD_RSECR 0x48   /* Read security register */
#define CMD_WKUP  0xAB   /* Release power down     */
#define CMD_PDWN  0xB9   /* Power down             */
#define CMD_ECHIP 0xC7   /* Full chip erase        */

/*
 * Target-specific SPI interface functions, their implementation can be found
 * in source files "spiFlash_xxx.c"
 */
extern uint8_t spiFlash_SendRecv(uint8_t val);
extern void spiFlash_init();
extern void spiFlash_terminate();

void W25Qx_init()
{
    gpio_setMode(FLASH_CS, OUTPUT);
    gpio_setPin(FLASH_CS);

    spiFlash_init();
}

void W25Qx_terminate()
{
    W25Qx_sleep();

    gpio_setMode(FLASH_CS,  INPUT);

    spiFlash_terminate();
}

void W25Qx_wakeup()
{
    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_WKUP);
    gpio_setPin(FLASH_CS);
}

void W25Qx_sleep()
{
    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_PDWN);
    gpio_setPin(FLASH_CS);
}

ssize_t W25Qx_readSecurityRegister(uint32_t addr, void* buf, size_t len)
{
    uint32_t addrBase  = addr & 0x3000;
    uint32_t addrRange = addr & 0xCFFF;
    if((addrBase < 0x1000) || (addrBase > 0x3000)) return -1; /* Out of base  */
    if(addrRange > 0xFF) return -1;                           /* Out of range */

    /* Keep 256-byte boundary to avoid wrap-around when reading */
    size_t readLen = len;
    if((addrRange + len) > 0xFF)
    {
        readLen = 0xFF - (addrRange & 0xFF);
    }

    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_RSECR);             /* Command        */
    (void) spiFlash_SendRecv((addr >> 16) & 0xFF);  /* Address high   */
    (void) spiFlash_SendRecv((addr >> 8) & 0xFF);   /* Address middle */
    (void) spiFlash_SendRecv(addr & 0xFF);          /* Address low    */
    (void) spiFlash_SendRecv(0x00);                 /* Dummy byte     */

    for(size_t i = 0; i < readLen; i++)
    {
        ((uint8_t *) buf)[i] = spiFlash_SendRecv(0x00);
    }

    gpio_setPin(FLASH_CS);

    return ((ssize_t) readLen);
}

void W25Qx_readData(uint32_t addr, void* buf, size_t len)
{
    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_READ);             /* Command        */
    (void) spiFlash_SendRecv((addr >> 16) & 0xFF);  /* Address high   */
    (void) spiFlash_SendRecv((addr >> 8) & 0xFF);   /* Address middle */
    (void) spiFlash_SendRecv(addr & 0xFF);          /* Address low    */

    for(size_t i = 0; i < len; i++)
    {
        ((uint8_t *) buf)[i] = spiFlash_SendRecv(0x00);
    }

    gpio_setPin(FLASH_CS);
}

bool W25Qx_eraseSector(uint32_t addr)
{
    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_WREN);             /* Write enable   */
    gpio_setPin(FLASH_CS);

    delayUs(5);

    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_ESECT);            /* Command        */
    (void) spiFlash_SendRecv((addr >> 16) & 0xFF);  /* Address high   */
    (void) spiFlash_SendRecv((addr >> 8) & 0xFF);   /* Address middle */
    (void) spiFlash_SendRecv(addr & 0xFF);          /* Address low    */
    gpio_setPin(FLASH_CS);

    /*
     * Wait till erase terminates.
     * Timeout after 500ms, at 250us per tick
     */
    uint16_t timeout = 2000;
    while(timeout > 0)
    {
        delayUs(250);
        timeout--;

        gpio_clearPin(FLASH_CS);
        (void) spiFlash_SendRecv(CMD_RDSTA);        /* Read status    */
        uint8_t status = spiFlash_SendRecv(0x00);
        gpio_setPin(FLASH_CS);

        /* If busy flag is low, we're done */
        if((status & 0x01) == 0) return true;
    }

    /* If we get here, we had a timeout */
    return false;
}

bool W25Qx_eraseChip()
{
    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_WREN);     /* Write enable */
    gpio_setPin(FLASH_CS);

    delayUs(5);

    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_ECHIP);    /* Command */
    gpio_setPin(FLASH_CS);

    /*
     * Wait till erase terminates.
     * Timeout after 200s, at 20ms per tick
     */
    uint16_t timeout = 10000;
    while(timeout > 0)
    {
        delayMs(20);
        timeout--;

        gpio_clearPin(FLASH_CS);
        (void) spiFlash_SendRecv(CMD_RDSTA);        /* Read status    */
        uint8_t status = spiFlash_SendRecv(0x00);
        gpio_setPin(FLASH_CS);

        /* If busy flag is low, we're done */
        if((status & 0x01) == 0) return true;
    }

    /* If we get here, we had a timeout */
    return false;
}

ssize_t W25Qx_writePage(uint32_t addr, void* buf, size_t len)
{
    /* Keep 256-byte boundary to avoid wrap-around when writing */
    size_t addrRange = addr & 0x0000FF;
    size_t writeLen  = len;
    if((addrRange + len) > 0x100)
    {
        writeLen = 0x100 - addrRange;
    }

    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_WREN);             /* Write enable   */
    gpio_setPin(FLASH_CS);

    delayUs(5);

    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_WRITE);            /* Command        */
    (void) spiFlash_SendRecv((addr >> 16) & 0xFF);  /* Address high   */
    (void) spiFlash_SendRecv((addr >> 8) & 0xFF);   /* Address middle */
    (void) spiFlash_SendRecv(addr & 0xFF);          /* Address low    */

    for(size_t i = 0; i < writeLen; i++)
    {
        uint8_t value = ((uint8_t *) buf)[i];
        (void) spiFlash_SendRecv(value);
    }

    gpio_setPin(FLASH_CS);

    /*
     * Wait till write terminates.
     * Timeout after 500ms, at 250us per tick
     */
    uint16_t timeout = 2000;
    while(timeout > 0)
    {
        delayUs(250);
        timeout--;

        gpio_clearPin(FLASH_CS);
        (void) spiFlash_SendRecv(CMD_RDSTA);        /* Read status    */
        uint8_t status = spiFlash_SendRecv(0x00);
        gpio_setPin(FLASH_CS);

        /* If busy flag is low, we're done */
        if((status & 0x01) == 0) return ((ssize_t) writeLen);
    }

    /* If we get here, we had a timeout */
    return -1;
}

bool W25Qx_writeData(uint32_t addr, void* buf, size_t len)
{
    /* Fail if we are trying to write more than 4K bytes */
    if(len > 4096) return false;

    /* Fail if we are trying to write across 4K blocks: this would erase two 4K
     * blocks for one write, which is not good for flash life.
     * We calculate block address using integer division of start and end address
     */
    uint32_t startBlockAddr = addr / 4096 * 4096;
    uint32_t endBlockAddr = (addr + len - 1) / 4096 * 4096;
    if(endBlockAddr != startBlockAddr)
        return false;

    /* Before writing, check if we're not trying to write the same content */
    uint8_t *flashData = ((uint8_t *) malloc(len));
    W25Qx_readData(addr, flashData, len);
    if(memcmp(buf, flashData, len) == 0)
    {
        free(flashData);
        return true;
    }

    free(flashData);

    /* Perform the actual read-erase-write of flash data. */
    uint8_t *flashBlock = ((uint8_t *) malloc(4096));
    W25Qx_readData(startBlockAddr, flashBlock, 4096);

    /* Overwrite changed portion */
    uint32_t blockOffset = addr % 4096;
    memcpy(&flashBlock[blockOffset], buf, len);

    /* Erase the 4K block */
    if(!W25Qx_eraseSector(startBlockAddr))
    {
        /* Erase operation failed, return failure */
        free(flashBlock);
        return false;
    }

    /* Write back the modified 4K block in chunks of 256 bytes */
    for(uint32_t offset = 0; offset < 4096; offset += 256)
    {
        W25Qx_writePage(startBlockAddr + offset, &flashBlock[offset], 256);
    }

    free(flashBlock);
    return true;
}
