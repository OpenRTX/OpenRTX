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

#include <SPI2.h>
#include <hwconfig.h>
#include <stdint.h>

#include "interfaces/gpio.h"

/*
 * Implementation of external flash SPI interface for MD9600 devices.
 */

uint8_t spiFlash_SendRecv(uint8_t val)
{
    spi2_lockDeviceBlocking();
    uint8_t x = spi2_sendRecv(val);
    spi2_releaseDevice();

    return x;
}

void spiFlash_init()
{
}

void spiFlash_terminate()
{
}
