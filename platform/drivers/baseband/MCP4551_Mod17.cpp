/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                Mathis Schmieder DB9MAT                  *
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

#include "MCP4551.h"
#include "I2C1.h"
#include <peripherals/gpio.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include <errno.h>


error_t mcp4551_init(uint8_t addr)
{
    return mcp4551_setWiper(addr, MCP4551_WIPER_MID);
}

error_t mcp4551_setWiper(uint8_t devAddr, uint16_t value)
{
    uint8_t data[2] = {(uint8_t)(value >> 8 & 0x01)|MCP4551_CMD_WRITE,
                        (uint8_t)value};

    i2c1_lockDeviceBlocking();
    error_t err = i2c1_write_bytes(devAddr, data, 2, true);
    i2c1_releaseDevice();
    return err;
}