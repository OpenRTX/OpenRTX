/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#include <audio_stream.h>

streamId outputStream_start(const enum AudioSink destination,
                            const enum AudioPriority prio,
                            stream_sample_t * const buf,
                            const size_t length,
                            const enum BufMode mode,
                            const uint32_t sampleRate)
{
    (void) destination;
    (void) prio;
    (void) buf;
    (void) length;
    (void) mode;
    (void) sampleRate;

    return -1;
}

stream_sample_t *outputStream_getIdleBuffer(const streamId id)
{
    (void) id;

    return NULL;
}

bool outputStream_sync(const streamId id, const bool bufChanged)
{
    (void) id;
    (void) bufChanged;

    return false;
}

void outputStream_stop(const streamId id)
{
    (void) id;
}

void outputStream_terminate(const streamId id)
{
    (void) id;
}
