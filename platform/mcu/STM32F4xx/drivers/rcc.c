/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "stm32f4xx.h"
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
