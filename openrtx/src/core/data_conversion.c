/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <data_conversion.h>
#include <hwconfig.h>

/*
 * Trivial implementation of Cortex M4 SIMD instructions for all the
 * targets not supporting them.
 */
#if (__CORTEX_M != 0x04)
static inline uint32_t __SADD16(uint32_t a, uint32_t b)
{
    uint16_t resHi = ((a >> 16) & 0x0000FFFF) + ((b >> 16) & 0x0000FFFF);
    uint16_t resLo = (a & 0x0000FFFF) + (b & 0x0000FFFF);
    return (resHi << 16) | resLo;
}
#endif


void S16toU12(int16_t *buffer, const size_t length)
{
    uint32_t *data = ((uint32_t *) buffer);
    for(size_t i = 0; i < length/2; i++)
    {
        uint32_t value = __SADD16(data[i], 0x80008000);
        data[i]        = (value >> 4) & 0x0FFF0FFF;
    }

    /* Handle last element in case of odd buffer length */
    if((length % 2) != 0)
    {
        int16_t value      = buffer[length - 1] + 32768;
        buffer[length - 1] = ((uint16_t) value) >> 4;
    }
}

void S16toU8(int16_t *buffer, const size_t length)
{
    uint32_t *data = ((uint32_t *) buffer);
    for(size_t i = 0; i < length/2; i++)
    {
        uint32_t value = __SADD16(data[i], 0x80008000);
        data[i]        = (value >> 8) & 0x00FF00FF;
    }

    /* Handle last element in case of odd buffer length */
    if((length % 2) != 0)
    {
        int16_t value      = buffer[length - 1] + 32768;
        buffer[length - 1] = ((uint16_t) value) >> 8;
    }
}
