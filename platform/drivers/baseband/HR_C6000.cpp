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

#include <utils.h>
#include "HR_C6000.h"

/*
 * Table of HR_C6000 CTCSS tones, used for reverse lookup of tone index to be
 * written in the configuration register. Taken from datasheet at page 90.
 */
static const uint16_t ctcssToneTable[] =
{
    670,  719,  744,  770,  797,  825,  854,
    885,  915,  948,  974,  1000, 1035, 1072,
    1109, 1148, 1188, 1230, 1273, 1318, 1365,
    1413, 1462, 1514, 1567, 1622, 1679, 1738,
    1799, 1862, 1928, 2035, 2107, 2181, 2257,
    2336, 2418, 2503, 693,  625,  1598, 1655,
    1713, 1773, 1835, 1899, 1966, 1995, 2065,
    2291, 2541
};

static uint8_t getToneIndex(const tone_t tone)
{
    uint8_t idx;

    for(idx = 0; idx < ARRAY_SIZE(ctcssToneTable); idx += 1)
    {
        if(ctcssToneTable[idx] == tone)
            break;
    }

    return idx + 1;
}

void HR_C6000::setTxCtcss(const tone_t tone, const uint8_t deviation)
{
    uint8_t index = getToneIndex(tone);
    writeCfgRegister(0xA8, index);        // Set CTCSS tone index
    writeCfgRegister(0xA0, deviation);    // Set CTCSS tone deviation
    writeCfgRegister(0xA1, 0x08);         // Enable CTCSS
}

void HR_C6000::setRxCtcss(const tone_t tone)
{
    uint8_t index = getToneIndex(tone);
    writeCfgRegister(0xA1, 0x08);         // Enable CTCSS
    writeCfgRegister(0xA7, 0x10);         // CTCSS detection threshold, value from datasheet
    writeCfgRegister(0xD3, 0x07);         // CTCSS sampling depth, value from datasheet
    writeCfgRegister(0xD2, 0xD0);
    writeCfgRegister(0xD4, index);        // Tone index
}
