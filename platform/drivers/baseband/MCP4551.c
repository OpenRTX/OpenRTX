/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Federico Amedeo Izzo IU2NUO,             *
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

// Common WIPER values
#define MCP4551_WIPER_MID   0x080
#define MCP4551_WIPER_A     0x100
#define MCP4551_WIPER_B     0x000

// Command definitions (sent to WIPER register)
#define MCP4551_CMD_WRITE   0x00
#define MCP4551_CMD_INC     0x04
#define MCP4551_CMD_DEC     0x08
#define MCP4551_CMD_READ    0x0C


int mcp4551_init(const struct i2cDevice *i2c, const uint8_t devAddr)
{
    return mcp4551_setWiper(i2c, devAddr, MCP4551_WIPER_MID);
}

int mcp4551_setWiper(const struct i2cDevice *i2c, const uint8_t devAddr,
                     const uint16_t value)
{
    uint8_t data[2] =
    {
        (uint8_t)(value >> 8 & 0x01) | MCP4551_CMD_WRITE,
        (uint8_t) value
    };

    i2c_acquire(i2c);
    int ret = i2c_write(i2c, devAddr, data, 2, true);
    i2c_release(i2c);

    return ret;
}
