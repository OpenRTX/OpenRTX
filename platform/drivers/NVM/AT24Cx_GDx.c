/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "peripherals/gpio.h"
#include "interfaces/delays.h"
#include <stdint.h>
#include <string.h>
#include "I2C0.h"
#include "drivers/NVM/AT24Cx.h"

static const uint8_t DEV_ADDR  = 0xA0;    /* EEPROM I2C address */
static const size_t  PAGE_SIZE = 128;

void AT24Cx_init()
{
    /*
     * Nothing to do here, on GDx devices the I2C bus is initialised in
     * platform_init() before starting all the other modules.
     */
}

void AT24Cx_terminate()
{

}

int AT24Cx_readData(uint32_t addr, void* buf, size_t len)
{
    uint16_t a = __builtin_bswap16((uint16_t) addr);

    /*
     * On GDx devices the I2C bus is shared between the EEPROM and the AT1846S,
     * so we have to acquire exclusive ownership before exchanging data
     */
    i2c0_lockDeviceBlocking();

    i2c0_write(DEV_ADDR, &a, 2, false);
    delayUs(10);
    i2c0_read(DEV_ADDR, buf, len);

    i2c0_releaseDevice();

    return 0;
}

int AT24Cx_writeData(uint32_t addr, const void *buf, size_t len)
{
    size_t  toWrite;
    uint8_t writeBuf[PAGE_SIZE + 2];

    /*
     * On GDx devices the I2C bus is shared between the EEPROM and the AT1846S,
     * so we have to acquire exclusive ownership before exchanging data
     */
    i2c0_lockDeviceBlocking();

    while(len > 0)
    {
        toWrite = len;
        if(toWrite >= PAGE_SIZE)
            toWrite = PAGE_SIZE;

        writeBuf[0] = (addr >> 8) & 0xFF;   /* High address byte */
        writeBuf[1] = (addr & 0xFF);        /* Low address byte  */
        memcpy(&writeBuf[2], buf, toWrite); /* Data              */

        i2c0_write(DEV_ADDR, writeBuf, toWrite + 2, true);

        len  -= toWrite;
        buf   = ((const uint8_t *) buf) + toWrite;
        addr += toWrite;

        /* Wait for the write cycle to end (max 5ms as per datasheet) */
        sleepFor(0, 5);
    }

    i2c0_releaseDevice();

    return 0;
}

static int nvm_api_read(const struct nvmDevice *dev, uint32_t offset, void *data, size_t len)
{
    (void) dev;

    return AT24Cx_readData(offset, data, len);
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t offset, const void *data, size_t len)
{
    (void) dev;

    return AT24Cx_writeData(offset, data, len);
}


const struct nvmOps AT24Cx_ops =
{
    .read   = nvm_api_read,
    .write  = nvm_api_write,
    .erase  = NULL,
    .sync   = NULL
};

const struct nvmInfo AT24Cx_info =
{
    .write_size   = 1,
    .erase_size   = 1,
    .erase_cycles = 1000000,
    .device_info  = NVM_EEPROM,
};
