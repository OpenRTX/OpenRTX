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

#include <hwconfig.h>
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <interfaces/platform.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "interfaces.h"

void _i2c_start();
void _i2c_stop();
void _i2c_write(uint8_t val);
uint8_t _i2c_read(bool ack);

/*
 * Implementation of AT1846S I2C interface.
 *
 * NOTE: on GDx devices the I2C bus is shared between the EEPROM and the AT1846S,
 * so we have to acquire exclusive ownership before exchanging data
 */

static const uint8_t devAddr = 0x5C;

void i2c_init()
{
    gpio_setMode(I2C_SDA, INPUT);
    gpio_setMode(I2C_SCL, OUTPUT);
    gpio_clearPin(I2C_SCL);
}

void i2c_writeReg16(uint8_t reg, uint16_t value)
{
    /*
     * Beware of endianness!
     * When writing an AT1846S register, bits 15:8 must be sent first, followed
     * by bits 7:0.
     */
    uint8_t valHi = (value >> 8) & 0xFF;
    uint8_t valLo = value & 0xFF;

    _i2c_start();
    _i2c_write(devAddr);
    _i2c_write(reg);
    _i2c_write(valHi);
    _i2c_write(valLo);
    _i2c_stop();
}

uint16_t i2c_readReg16(uint8_t reg)
{
    _i2c_start();
    _i2c_write(devAddr);
    _i2c_write(reg);
    _i2c_start();
    _i2c_write(devAddr | 0x01);
    uint8_t valHi = _i2c_read(true);
    uint8_t valLo = _i2c_read(false);
    _i2c_stop();

    return (valHi << 8) | valLo;
}

/*
 * Implementation of HR_C6000 "user" SPI interface.
 */

void uSpi_init()
{
    gpio_setMode(DMR_CS,    OUTPUT);
    gpio_setMode(DMR_CLK,   OUTPUT);
    gpio_setMode(DMR_MOSI,  OUTPUT);
    gpio_setMode(DMR_MISO,  OUTPUT);
}

uint8_t uSpi_sendRecv(uint8_t val)
{
    gpio_clearPin(DMR_CLK);

    uint8_t incoming = 0;
    uint8_t cnt = 0;

    for(; cnt < 8; cnt++)
    {
        gpio_setPin(DMR_CLK);

        if(val & (0x80 >> cnt))
        {
            gpio_setPin(DMR_MOSI);
        }
        else
        {
            gpio_clearPin(DMR_MOSI);
        }

        delayUs(1);
        gpio_clearPin(DMR_CLK);
        incoming = (incoming << 1) | gpio_readPin(DMR_MISO);
        delayUs(1);
    }

    return incoming;
}

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
    delayUs(2);

    gpio_setPin(I2C_SCL);
    delayUs(2);

    gpio_clearPin(I2C_SDA);
    delayUs(2);

    gpio_clearPin(I2C_SCL);
    delayUs(6);
}

void _i2c_stop()
{
    gpio_setMode(I2C_SDA, OUTPUT);

    gpio_clearPin(I2C_SCL);
    delayUs(2);

    gpio_clearPin(I2C_SDA);
    delayUs(2);

    gpio_setPin(I2C_SCL);
    delayUs(2);

    gpio_setPin(I2C_SDA);
    delayUs(2);

    /*
     * Lines commented to keep SCL high when idle
     *
    gpio_clearPin(I2C_SCL);
    delayUs(2);
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
        delayUs(2);
    }

    /* Ensure SCL is low before releasing SDA */
    gpio_clearPin(I2C_SCL);

    /* Clock cycle for slave ACK/NACK */
    gpio_setMode(I2C_SDA, INPUT_PULL_UP);
    delayUs(2);
    gpio_setPin(I2C_SCL);
    delayUs(2);
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
        delayUs(2);
        gpio_setPin(I2C_SCL);
        delayUs(2);

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
    delayUs(2);
    if(!ack) gpio_setPin(I2C_SDA);

    /* Clock cycle for ACK/NACK */
    delayUs(2);
    gpio_setPin(I2C_SCL);
    delayUs(2);
    gpio_clearPin(I2C_SCL);
    delayUs(2);

    return value;
}
