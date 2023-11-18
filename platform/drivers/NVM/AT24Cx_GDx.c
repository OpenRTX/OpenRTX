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

#include <peripherals/gpio.h>
#include <interfaces/delays.h>
#include <stdint.h>
#include <string.h>
#include <I2C0.h>
#include "AT24Cx.h"

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

static const struct nvmParams AT24Cx_params =
{
    .write_size   = 1,
    .erase_size   = 1,
    .erase_cycles = 1000000,
    .type         = NVM_EEPROM,
};


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

static const struct nvmParams *nvm_api_params(const struct nvmDevice *dev)
{
    (void) dev;

    return &AT24Cx_params;
}

const struct nvmApi AT24Cx_api =
{
    .read   = nvm_api_read,
    .write  = nvm_api_write,
    .erase  = NULL,
    .sync   = NULL,
    .params = nvm_api_params
};
