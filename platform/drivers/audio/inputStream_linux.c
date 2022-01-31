/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <interfaces/audio_stream.h>
#include <hwconfig.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

stream_sample_t *buffer = NULL;
size_t bufferLength = 0;
bool first_half = true;
FILE *baseband_file = NULL;

streamId inputStream_start(const enum AudioSource source,
                           const enum AudioPriority prio,
                           stream_sample_t * const buf,
                           const size_t bufLength,
                           const enum BufMode mode,
                           const uint32_t sampleRate)
{
    (void) source;
    (void) prio;
    (void) mode;
    (void) sampleRate;

    buffer = buf;
    bufferLength = bufLength;

    baseband_file = fopen("./tests/unit/assets/M17_test_baseband_dc.raw", "rb");
    if (!baseband_file)
    {
        perror("Error while reading file\n");
    }

    return -1;
}

dataBlock_t inputStream_getData(streamId id)
{
    (void) id;

    dataBlock_t block;
    block.len  = bufferLength;
    if (first_half)
    {
        first_half = false;
        block.data = buffer;
    } else
    {
        first_half = true;
        block.data = buffer + bufferLength;
    }
    size_t read_items = fread(block.data, 2, block.len, baseband_file);
    if (read_items != block.len)
    {
        exit(-1);
    }
    return block;
}

void inputStream_stop(streamId id)
{
    fclose(baseband_file);
    (void) id;
}
