/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#include <hwconfig.h>

#include "MCP4551.h"
#include "interfaces/delays.h"
#include "interfaces/gpio.h"

/*
 * Implementation of MCP4551 I2C interface.
 *
 * Hardware I2C is not yet implemented. Bit-bang, baby!
 */

void _i2c_start();
void _i2c_stop();
void _i2c_write(uint8_t val);
uint8_t _i2c_read(bool ack);

void i2c_init()
{
    gpio_setMode(I2C_SDA, INPUT);
    gpio_setMode(I2C_SCL, OUTPUT);
    gpio_clearPin(I2C_SCL);
}

void mcp4551_init(uint8_t addr)
{
    mcp4551_setWiper(addr, MCP4551_WIPER_MID);
}

void mcp4551_setWiper(uint8_t devAddr, uint16_t value)
{
    _i2c_start();
    _i2c_write(devAddr << 1);
    uint8_t temp = ((value >> 8 & 0x01) | MCP4551_CMD_WRITE);
    _i2c_write(temp);
    temp = (value & 0xFF);
    _i2c_write(temp);
    _i2c_stop();
}

/*uint16_t i2c_readReg16(uint8_t devAddr, uint8_t reg)
{
    _i2c_start();
    _i2c_write(devAddr << 1);
    _i2c_write(reg);
    _i2c_start();
    _i2c_write(devAddr | 0x01);
    uint8_t valHi = _i2c_read(true);
    uint8_t valLo = _i2c_read(false);
    _i2c_stop();

    return (valHi << 8) | valLo;
} */

/*
 * Software I2C routine
 */

void _i2c_start()
{
    gpio_setMode(I2C_SDA, OUTPUT);

    /*
     * Lines commented to keep SCL high when idle
     *
    gpio_clearPin(I2C_SCL);
    delayUs(2);
    */

    gpio_setPin(I2C_SDA);
    delayUs(5);

    gpio_setPin(I2C_SCL);
    delayUs(5);

    gpio_clearPin(I2C_SDA);
    delayUs(5);

    gpio_clearPin(I2C_SCL);
    delayUs(6);
}

void _i2c_stop()
{
    gpio_setMode(I2C_SDA, OUTPUT);

    gpio_clearPin(I2C_SCL);
    delayUs(5);

    gpio_clearPin(I2C_SDA);
    delayUs(5);

    gpio_setPin(I2C_SCL);
    delayUs(5);

    gpio_setPin(I2C_SDA);
    delayUs(5);

    /*
     * Lines commented to keep SCL high when idle
     *
    gpio_clearPin(I2C_SCL);
    delayUs(5);
    */
}

void _i2c_write(uint8_t val)
{
    gpio_setMode(I2C_SDA, OUTPUT);

    for(uint8_t i = 0; i < 8; i++)
    {
        gpio_clearPin(I2C_SCL);
        delayUs(1);

        if(val & 0x80)
        {
            gpio_setPin(I2C_SDA);
        }
        else
        {
            gpio_clearPin(I2C_SDA);
        }

        val <<= 1;
        delayUs(1);
        gpio_setPin(I2C_SCL);
        delayUs(5);
    }

    /* Ensure SCL is low before releasing SDA */
    gpio_clearPin(I2C_SCL);

    /* Clock cycle for slave ACK/NACK */
    gpio_setMode(I2C_SDA, INPUT_PULL_UP);
    delayUs(5);
    gpio_setPin(I2C_SCL);
    delayUs(5);
    gpio_clearPin(I2C_SCL);
    delayUs(1);

    /* Asserting SDA pin allows to fastly bring the line to idle state */
    gpio_setPin(I2C_SDA);
    gpio_setMode(I2C_SDA, OUTPUT);
    delayUs(6);
}

uint8_t _i2c_read(bool ack)
{
    gpio_setMode(I2C_SDA, INPUT_PULL_UP);
    gpio_clearPin(I2C_SCL);

    uint8_t value = 0;
    for(uint8_t i = 0; i < 8; i++)
    {
        delayUs(5);
        gpio_setPin(I2C_SCL);
        delayUs(5);

        value <<= 1;
        value |= gpio_readPin(I2C_SDA);

        gpio_clearPin(I2C_SCL);
    }

    /*
     * Set ACK/NACK state BEFORE putting SDA gpio to output mode.
     * This avoids spurious spikes which can be interpreted as NACKs
     */
    gpio_clearPin(I2C_SDA);
    gpio_setMode(I2C_SDA, OUTPUT);
    delayUs(5);
    if(!ack) gpio_setPin(I2C_SDA);

    /* Clock cycle for ACK/NACK */
    delayUs(5);
    gpio_setPin(I2C_SCL);
    delayUs(5);
    gpio_clearPin(I2C_SCL);
    delayUs(5);

    return value;
}
