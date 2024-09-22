/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <stm32f4xx.h>
#include <stdint.h>
#include "rcc.h"

static uint32_t busClockFreq[PERIPH_BUS_NUM] = {0};

static void updateBusClockFreq(const uint8_t bus)
{
    uint32_t preBits;
    uint32_t clkDiv = 1;

    switch(bus)
    {
        case PERIPH_BUS_AHB:
            preBits = (RCC->CFGR >> 4) & 0x0F;
            if((preBits & 0x08) != 0)
                clkDiv = 1 << ((preBits & 0x07) + 1);
            break;

        case PERIPH_BUS_APB1:
            preBits = (RCC->CFGR >> 10) & 0x07;
            if((preBits & 0x04) != 0)
                clkDiv = 1 << ((preBits & 0x03) + 1);
            break;

        case PERIPH_BUS_APB2:
            preBits = (RCC->CFGR >> 13) & 0x07;
            if((preBits & 0x04) != 0)
                clkDiv = 1 << ((preBits & 0x03) + 1);
            break;

        default:
            break;
    }

    busClockFreq[bus] = SystemCoreClock / clkDiv;
}

uint32_t rcc_getBusClock(const uint8_t bus)
{
    if(bus >= PERIPH_BUS_NUM)
        return 0;

    if(busClockFreq[bus] == 0)
        updateBusClockFreq(bus);

    return busClockFreq[bus];
}
