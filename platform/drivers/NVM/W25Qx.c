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

#include "W25Qx.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <hwconfig.h>
#include <peripherals/gpio.h>
#include <interfaces/delays.h>

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

static const size_t PAGE_SIZE = 256;
static const size_t SECT_SIZE = 4096;


/**
 * \internal
 * Wait until an erase or write operation finishes.
 *
 * @param timeout: wait timeout, in ms.
 * @return zero on success, -EIO if timeout expires.
 */
static int waitUntilReady(uint32_t timeout)
{
    // Each wait tick is 500us
    timeout *= 2;

    while(timeout > 0)
    {
        delayUs(500);
        timeout--;

        gpio_clearPin(FLASH_CS);
        spiFlash_SendRecv(CMD_RDSTA);
        uint8_t status = spiFlash_SendRecv(0x00);
        gpio_setPin(FLASH_CS);

        /* If busy flag is low, we're done */
        if((status & 0x01) == 0)
            return 0;
    }

    return -EIO;
}


void W25Qx_init()
{
    gpio_setMode(FLASH_CS, OUTPUT);
    gpio_setPin(FLASH_CS);

    spiFlash_init();
    W25Qx_wakeup();
    // TODO: Implement sleep to increase power saving
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
    spiFlash_SendRecv(CMD_WKUP);
    gpio_setPin(FLASH_CS);
}

void W25Qx_sleep()
{
    gpio_clearPin(FLASH_CS);
    spiFlash_SendRecv(CMD_PDWN);
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
    spiFlash_SendRecv(CMD_RSECR);            /* Command        */
    spiFlash_SendRecv((addr >> 16) & 0xFF);  /* Address high   */
    spiFlash_SendRecv((addr >> 8) & 0xFF);   /* Address middle */
    spiFlash_SendRecv(addr & 0xFF);          /* Address low    */
    spiFlash_SendRecv(0x00);                 /* Dummy byte     */

    for(size_t i = 0; i < readLen; i++)
    {
        ((uint8_t *) buf)[i] = spiFlash_SendRecv(0x00);
    }

    gpio_setPin(FLASH_CS);

    return ((ssize_t) readLen);
}

int W25Qx_readData(uint32_t addr, void* buf, size_t len)
{
    gpio_clearPin(FLASH_CS);
    spiFlash_SendRecv(CMD_READ);             /* Command        */
    spiFlash_SendRecv((addr >> 16) & 0xFF);  /* Address high   */
    spiFlash_SendRecv((addr >> 8) & 0xFF);   /* Address middle */
    spiFlash_SendRecv(addr & 0xFF);          /* Address low    */

    for(size_t i = 0; i < len; i++)
    {
        ((uint8_t *) buf)[i] = spiFlash_SendRecv(0x00);
    }

    gpio_setPin(FLASH_CS);

    return 0;
}

int W25Qx_erase(uint32_t addr, size_t size)
{
    // Addr or size not aligned to sector size
    if(((addr % SECT_SIZE) != 0) || ((size % SECT_SIZE) != 0))
        return -EINVAL;

    gpio_clearPin(FLASH_CS);
    spiFlash_SendRecv(CMD_WREN);             /* Write enable   */
    gpio_setPin(FLASH_CS);

    delayUs(5);

    int ret = 0;
    while(size > 0)
    {
        gpio_clearPin(FLASH_CS);
        spiFlash_SendRecv(CMD_ESECT);            /* Command        */
        spiFlash_SendRecv((addr >> 16) & 0xFF);  /* Address high   */
        spiFlash_SendRecv((addr >> 8) & 0xFF);   /* Address middle */
        spiFlash_SendRecv(addr & 0xFF);          /* Address low    */
        gpio_setPin(FLASH_CS);

        ret = waitUntilReady(500);
        if(ret < 0)
            break;

        size -= SECT_SIZE;
        addr += SECT_SIZE;
    }

    return ret;
}

bool W25Qx_eraseChip()
{
    gpio_clearPin(FLASH_CS);
    spiFlash_SendRecv(CMD_WREN);
    gpio_setPin(FLASH_CS);

    delayUs(5);

    gpio_clearPin(FLASH_CS);
    spiFlash_SendRecv(CMD_ECHIP);
    gpio_setPin(FLASH_CS);

    /*
     * Wait until erase terminates, timeout after 200s.
     */
    int ret = waitUntilReady(200000);
    if(ret == 0)
        return true;

    return false;
}

ssize_t W25Qx_writePage(uint32_t addr, const void* buf, size_t len)
{
    /* Keep page boundary to avoid wrap-around when writing */
    size_t addrRange = addr & (PAGE_SIZE - 1);
    size_t writeLen  = len;
    if((addrRange + len) > PAGE_SIZE)
    {
        writeLen = PAGE_SIZE - addrRange;
    }

    gpio_clearPin(FLASH_CS);
    spiFlash_SendRecv(CMD_WREN);             /* Write enable   */
    gpio_setPin(FLASH_CS);

    delayUs(5);

    gpio_clearPin(FLASH_CS);
    spiFlash_SendRecv(CMD_WRITE);            /* Command        */
    spiFlash_SendRecv((addr >> 16) & 0xFF);  /* Address high   */
    spiFlash_SendRecv((addr >> 8) & 0xFF);   /* Address middle */
    spiFlash_SendRecv(addr & 0xFF);          /* Address low    */

    for(size_t i = 0; i < writeLen; i++)
    {
        uint8_t value = ((uint8_t *) buf)[i];
        (void) spiFlash_SendRecv(value);
    }

    gpio_setPin(FLASH_CS);

    /*
     * Wait until erase terminates, timeout after 500ms.
     */
    int ret = waitUntilReady(500);
    if(ret < 0)
        return (ssize_t) ret;

    return writeLen;
}

int W25Qx_writeData(uint32_t addr, const void *buf, size_t len)
{
    while(len > 0)
    {
        size_t toWrite = len;

        // Maximum single-shot write lenght is one page
        if(toWrite >= PAGE_SIZE)
            toWrite = PAGE_SIZE;

        ssize_t written = W25Qx_writePage(addr, buf, toWrite);
        if(written < 0)
            return (int) written;

        len  -= (size_t) written;
        buf   = ((const uint8_t *) buf) + (size_t) written;
        addr += (size_t) written;
    }

    return 0;
}

static const struct nvmParams W25Qx_params =
{
    .write_size   = 1,
    .erase_size   = SECT_SIZE,
    .erase_cycles = 100000,
    .type         = NVM_FLASH
};

static int nvm_api_readSecReg(const struct nvmDevice *dev, uint32_t offset, void *data, size_t len)
{
    (void) dev;

    return W25Qx_readSecurityRegister(offset, data, len);
}

static int nvm_api_read(const struct nvmDevice *dev, uint32_t offset, void *data, size_t len)
{
    (void) dev;

    return W25Qx_readData(offset, data, len);
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t offset, const void *data, size_t len)
{
    (void) dev;

    return W25Qx_writeData(offset, data, len);
}

static int nvm_api_erase(const struct nvmDevice *dev, uint32_t offset, size_t size)
{
    (void) dev;

    return W25Qx_erase(offset, size);
}

static const struct nvmParams *nvm_api_params(const struct nvmDevice *dev)
{
    (void) dev;

    return &W25Qx_params;
}

const struct nvmApi W25Qx_api =
{
    .read   = nvm_api_read,
    .write  = nvm_api_write,
    .erase  = nvm_api_erase,
    .sync   = NULL,
    .params = nvm_api_params
};

const struct nvmApi W25Qx_secReg_api =
{
    .read   = nvm_api_readSecReg,
    .write  = NULL,
    .erase  = NULL,
    .sync   = NULL,
    .params = nvm_api_params
};
