/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO,                            *
 *                         Federico Terraneo                               *
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
#include <stdio.h>

uint16_t crc16(const void *message, size_t length)
{
    const uint8_t *m = ((const uint8_t* ) message);
    uint16_t crc = 0xffff;
    for(size_t i = 0; i < length; i++)
    {
        uint16_t x = ((crc >> 8) ^ m[i]) & 0xff;
        x ^= x >> 4;
        crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
    }

    return crc;
}

/**
 * \internal
 * used by memDump
 */
static void memPrint(const char *data, char len)
{
    iprintf("0x%08x | ", ((unsigned int) data));
    for(int i = 0; i < len; i++) iprintf("%02x ", data[i]);
    for(int i = 0; i < (16-len); i++) iprintf("   ");
    iprintf("| ");
    for(int i = 0; i < len; i++)
    {
        if((data[i] >= 32) && (data[i] < 127)) iprintf("%c", data[i]);
        else iprintf(".");
    }
    iprintf("\r\n");
}

void memDump(const void *start, int len)
{
    const char *data = ((const char *) start);
    while(len > 16)
    {
        memPrint(data, 16);
        len -= 16;
        data += 16;
    }
    if(len > 0) memPrint(data, len);
}
