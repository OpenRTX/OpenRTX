/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#include <hwconfig.h>
#include <stdint.h>

#include "interfaces/delays.h"
#include "interfaces/gpio.h"

/*
 * Implementation of external flash SPI interface for GDx devices.
 * In these radios the SPI communication has to be managed through software bit
 * banging, since there is no SPI peripheral available.
 */

uint8_t spiFlash_SendRecv(uint8_t val)
{
    uint8_t data = 0;
    for(uint8_t i = 0; i < 8; i++)
    {
        gpio_clearPin(FLASH_CLK);
        delayUs(1);

        if((val << i) & 0x80)
        {
            gpio_setPin(FLASH_SDO);
        }
        else
        {
            gpio_clearPin(FLASH_SDO);
        }

        gpio_setPin(FLASH_CLK);
        delayUs(1);

        data <<= 1;
        data |= (gpio_readPin(FLASH_SDI)) ? 0x01 : 0x00;
    }

    return data;
}

void spiFlash_init()
{
    gpio_setMode(FLASH_CLK, OUTPUT);
    gpio_setMode(FLASH_SDO, OUTPUT);
    gpio_setMode(FLASH_SDI, INPUT);

    gpio_clearPin(FLASH_CLK);
    gpio_clearPin(FLASH_SDO);
}

void spiFlash_terminate()
{
    gpio_setMode(FLASH_CLK, INPUT);
    gpio_setMode(FLASH_SDO, INPUT);
    gpio_setMode(FLASH_SDI, INPUT);
}
