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

#include "AT24Cx.h"
#include <I2C0.h>
#include <gpio.h>
#include <delays.h>
#include <stdint.h>
#include <hwconfig.h>

static const uint8_t devAddr = 0xA0;    /* EEPROM I2C address */

void AT24Cx_init()
{
    gpio_setMode(I2C_SDA, OPEN_DRAIN);
    gpio_setMode(I2C_SCL, OPEN_DRAIN);
    gpio_setAlternateFunction(I2C_SDA, 3);
    gpio_setAlternateFunction(I2C_SCL, 3);

    i2c0_init();
}

void AT24Cx_terminate()
{
    i2c0_terminate();
    gpio_setMode(I2C_SDA, INPUT);
    gpio_setMode(I2C_SCL, INPUT);
}

void AT24Cx_readData(uint32_t addr, void* buf, size_t len)
{
    uint16_t a = __builtin_bswap16((uint16_t) addr);

    /*
     * On GDx devices the I2C bus is shared between the EEPROM and the AT1846S,
     * so we have to acquire exclusive ownership before exchanging data
     */
    i2c0_lockDeviceBlocking();

    i2c0_write(devAddr, &a, 2, false);
    delayUs(10);
    i2c0_read(devAddr, buf, len);

    i2c0_releaseDevice();
}
