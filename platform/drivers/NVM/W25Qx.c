/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/NVM/W25Qx.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hwconfig.h"
#include "peripherals/gpio.h"
#include "interfaces/delays.h"

#define CMD_WRITE 0x02   /* Read data              */
#define CMD_READ  0x03   /* Read data              */
#define CMD_RDSTA 0x05   /* Read status register   */
#define CMD_WREN  0x06   /* Write enable           */
#define CMD_ESECT 0x20   /* Erase 4kB sector       */
#define CMD_RSECR 0x48   /* Read security register */
#define CMD_WKUP  0xAB   /* Release power down     */
#define CMD_PDWN  0xB9   /* Power down             */
#define CMD_EXADD 0xB7   /* Enter 4-byte addr mode */
#define CMD_ECHIP 0xC7   /* Full chip erase        */

static const size_t PAGE_SIZE = 256;
static const size_t SECT_SIZE = 4096;


/**
 * \internal
 * Wait until an erase or write operation finishes.
 *
 * @param timeout: wait timeout, in ms.
 * @return zero on success, -EIO if timeout expires.
 */
static int waitUntilReady(const struct W25QxCfg *cfg, uint32_t timeout)
{
    // Each wait tick is 500us
    timeout *= 2;

    while(timeout > 0)
    {
        delayUs(500);
        timeout--;

        uint8_t cmd[] = {CMD_RDSTA, 0x00};
        uint8_t ret[2];

        gpioPin_clear(&cfg->cs);
        spi_transfer(cfg->spi, cmd, ret, 2);
        gpioPin_set(&cfg->cs);

        /* If busy flag is low, we're done */
        if((ret[1] & 0x01) == 0)
            return 0;
    }

    return -EIO;
}

static inline void enableWrite(const struct W25QxCfg *cfg)
{
    const uint8_t cmd = CMD_WREN;

    gpioPin_clear(&cfg->cs);
    spi_send(cfg->spi, &cmd, 1);
    gpioPin_set(&cfg->cs);

    delayUs(5);
}

void W25Qx_init(const struct nvmDevice *dev)
{
    const struct W25QxCfg *cfg = (const struct W25QxCfg *) dev->priv;

    gpioPin_setMode(&cfg->cs, OUTPUT);
    gpioPin_set(&cfg->cs);

    // TODO: Implement sleep to increase power saving
    W25Qx_wakeup(dev);

#ifdef CONFIG_W25Qx_EXT_ADDR
    const uint8_t cmd = CMD_EXADD;

    gpioPin_clear(&cfg->cs);
    spi_send(cfg->spi, &cmd, 1);
    gpioPin_set(&cfg->cs);
#endif
}

void W25Qx_terminate(const struct nvmDevice *dev)
{
    const struct W25QxCfg *cfg = (const struct W25QxCfg *) dev->priv;

    W25Qx_sleep(dev);
    gpioPin_setMode(&cfg->cs, INPUT);
}

void W25Qx_wakeup(const struct nvmDevice *dev)
{
    const struct W25QxCfg *cfg = (const struct W25QxCfg *) dev->priv;
    const uint8_t cmd = CMD_WKUP;

    spi_acquire(cfg->spi);

    gpioPin_clear(&cfg->cs);
    spi_send(cfg->spi, &cmd, 1);
    gpioPin_set(&cfg->cs);

    spi_release(cfg->spi);
}

void W25Qx_sleep(const struct nvmDevice *dev)
{
    const struct W25QxCfg *cfg = (const struct W25QxCfg *) dev->priv;
    const uint8_t cmd = CMD_PDWN;

    spi_acquire(cfg->spi);

    gpioPin_clear(&cfg->cs);
    spi_send(cfg->spi, &cmd, 1);
    gpioPin_set(&cfg->cs);

    spi_release(cfg->spi);
}

static int nvm_api_readSecReg(const struct nvmDevice *dev, uint32_t address, void *data, size_t len)
{
    const struct W25QxCfg *cfg = (const struct W25QxCfg *) dev->priv;
    const size_t offset = (address + len) & 0x0FFF;

    // Avoid wrap-around when reading
    if(offset > 0xFF)
        return -EINVAL;

    const uint8_t command[] =
    {
        CMD_RSECR,              // Command
        (address >> 16) & 0xFF, // Address high
        (address >> 8)  & 0xFF, // Address middle
        address & 0xFF,         // Address low
        0x00                    // Dummy byte
    };

    spi_acquire(cfg->spi);
    gpioPin_clear(&cfg->cs);

    spi_send(cfg->spi, command, sizeof(command));
    spi_receive(cfg->spi, data, len);

    gpioPin_set(&cfg->cs);
    spi_release(cfg->spi);

    return 0;
}

static int nvm_api_read(const struct nvmDevice *dev, uint32_t offset, void *data, size_t len)
{
    const struct W25QxCfg *cfg = (const struct W25QxCfg *) dev->priv;

    const uint8_t command[] =
    {
        CMD_READ,               // Command
#ifdef CONFIG_W25Qx_EXT_ADDR
        (offset >> 24) & 0xFF,  // Address 31:24
#endif
        (offset >> 16) & 0xFF,  // Address high
        (offset >> 8)  & 0xFF,  // Address middle
        offset & 0xFF,          // Address low
    };

    spi_acquire(cfg->spi);
    gpioPin_clear(&cfg->cs);

    spi_send(cfg->spi, command, sizeof(command));
    spi_receive(cfg->spi, data, len);

    gpioPin_set(&cfg->cs);
    spi_release(cfg->spi);

    return 0;
}

static int nvm_api_erase(const struct nvmDevice *dev, uint32_t offset, size_t size)
{
    const struct W25QxCfg *cfg = (const struct W25QxCfg *) dev->priv;

    // Addr or size not aligned to sector size
    if(((offset % SECT_SIZE) != 0) || ((size % SECT_SIZE) != 0))
        return -EINVAL;

    spi_acquire(cfg->spi);

    int ret = 0;
    while(size > 0)
    {
        // Write enable, has to be issued for each erase operation
        enableWrite(cfg);

        // Sector erase
        const uint8_t command[] =
        {
            CMD_ESECT,              // Command
#ifdef CONFIG_W25Qx_EXT_ADDR
            (offset >> 24) & 0xFF,  // Address 31:24
#endif
            (offset >> 16) & 0xFF,  // Address high
            (offset >> 8)  & 0xFF,  // Address middle
            offset & 0xFF,          // Address low
        };

        gpioPin_clear(&cfg->cs);
        spi_send(cfg->spi, command, sizeof(command));
        gpioPin_set(&cfg->cs);

        ret = waitUntilReady(cfg, 500);
        if(ret < 0)
            break;

        size -= SECT_SIZE;
        offset += SECT_SIZE;
    }

    spi_release(cfg->spi);

    return ret;
}

static ssize_t W25Qx_writePage(const struct nvmDevice *dev, uint32_t addr,
                               const void* buf, size_t len)
{
    const struct W25QxCfg *cfg = (const struct W25QxCfg *) dev->priv;

    // Keep page boundary to avoid wrap-around when writing
    size_t addrRange = addr & (PAGE_SIZE - 1);
    size_t writeLen  = len;
    if((addrRange + len) > PAGE_SIZE)
    {
        writeLen = PAGE_SIZE - addrRange;
    }

    // Write enable bit has to be set before each page program
    enableWrite(cfg);

    // Page program
    const uint8_t command[] =
    {
        CMD_WRITE,            // Command
#ifdef CONFIG_W25Qx_EXT_ADDR
        (addr >> 24) & 0xFF,  // Address 31:24
#endif
        (addr >> 16) & 0xFF,  // Address high
        (addr >> 8)  & 0xFF,  // Address middle
         addr        & 0xFF,  // Address low
    };

    gpioPin_clear(&cfg->cs);
    spi_send(cfg->spi, command, sizeof(command));
    spi_send(cfg->spi, buf, writeLen);
    gpioPin_set(&cfg->cs);

    // Wait until write terminates, timeout after 500ms.
    int ret = waitUntilReady(cfg, 500);
    spi_release(cfg->spi);

    if(ret < 0)
        return (ssize_t) ret;
    else
        return writeLen;
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t offset, const void *data, size_t len)
{
    while(len > 0)
    {
        // Maximum single-shot write length is one page
        size_t toWrite = len;
        if(toWrite >= PAGE_SIZE)
            toWrite = PAGE_SIZE;

        ssize_t written = W25Qx_writePage(dev, offset, data, toWrite);
        if(written < 0)
            return (int) written;

        len    -= (size_t) written;
        data    = ((const uint8_t *) data) + (size_t) written;
        offset += (size_t) written;
    }

    return 0;
}

const struct nvmOps W25Qx_ops =
{
    .read   = nvm_api_read,
    .write  = nvm_api_write,
    .erase  = nvm_api_erase,
    .sync   = NULL,
};

const struct nvmOps W25Qx_secReg_ops =
{
    .read   = nvm_api_readSecReg,
    .write  = NULL,
    .erase  = NULL,
    .sync   = NULL,
};

const struct nvmInfo W25Qx_info =
{
    .write_size   = 1,
    .erase_size   = SECT_SIZE,
    .erase_cycles = 100000,
    .device_info  = NVM_FLASH | NVM_WRITE | NVM_ERASE
};

const struct nvmInfo W25Qx_secReg_info =
{
    .write_size   = 0,
    .erase_size   = 0,
    .erase_cycles = 0,
    .device_info  = NVM_FLASH
};
