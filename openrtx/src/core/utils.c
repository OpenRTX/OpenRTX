/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
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

#include <utils.h>
#include <math.h>

uint8_t interpCalParameter(const freq_t freq, const freq_t *calPoints,
                           const uint8_t *param, const uint8_t elems)
{

    if(freq <= calPoints[0])         return param[0];
    if(freq >= calPoints[elems - 1]) return param[elems - 1];

    /* Find calibration point nearest to target frequency */
    uint8_t pos = 0;
    for(; pos < elems; pos++)
    {
        if(calPoints[pos] >= freq) break;
    }

    uint8_t interpValue = 0;
    freq_t  delta = calPoints[pos] - calPoints[pos - 1];

    if(param[pos - 1] < param[pos])
    {
        interpValue = param[pos - 1] + ((freq - calPoints[pos - 1]) *
                                        (param[pos] - param[pos - 1]))/delta;
    }
    else
    {
        interpValue = param[pos - 1] - ((freq - calPoints[pos - 1]) *
                                       (param[pos - 1] - param[pos]))/delta;
    }

    return interpValue;
}

float dBmToWatt(const uint8_t n)
{
    float dBm   = 10.0f + ((float) n) * 0.2f;
    float power = pow(10.0f, (dBm - 30.0f)/10.0f);

    return power;
}

uint32_t bcd2bin(uint32_t bcd)
{
    return ((bcd >> 28) & 0x0F) * 10000000 +
           ((bcd >> 24) & 0x0F) * 1000000 +
           ((bcd >> 20) & 0x0F) * 100000 +
           ((bcd >> 16) & 0x0F) * 10000 +
           ((bcd >> 12) & 0x0F) * 1000 +
           ((bcd >> 8) & 0x0F)  * 100 +
           ((bcd >> 4) & 0x0F)  * 10 +
           (bcd & 0x0F);
}
