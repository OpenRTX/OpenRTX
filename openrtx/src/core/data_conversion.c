/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/data_conversion.h"
#include "hwconfig.h"

/*
 * Trivial implementation of Cortex M4 SIMD instructions for all the
 * targets not supporting them.
 */
#if (__CORTEX_M < 0x04)
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
