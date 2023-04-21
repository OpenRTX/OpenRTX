/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
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

#ifndef AUDIO_PATH_H
#define AUDIO_PATH_H

#include <interfaces/audio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum PathStatus
{
    PATH_CLOSED,
    PATH_OPEN,
    PATH_SUSPENDED
};

typedef struct
{
    uint8_t source;
    uint8_t sink;
    uint8_t prio;
    uint8_t status;
}
pathInfo_t;

typedef int32_t pathId;


/**
 * Request to set up an audio path, returns an error if the path is already used
 * with an higher priority.
 *
 * @param source: identifier of the input audio peripheral.
 * @param sink: identifier of the output audio peripheral.
 * @param prio: priority of the requester.
 * @return a unique identifier of the opened path or -1 if path is already in use.
 */
pathId audioPath_request(enum AudioSource source, enum AudioSink sink,
                         enum AudioPriority prio);

/**
 * Get all the informations of an audio path.
 *
 * @param id: ID of the audio path.
 * @return audio path informations.
 */
pathInfo_t audioPath_getInfo(const pathId id);

/**
 * Get the current status of an audio path.
 *
 * @param id: ID of the audio path.
 * @return status of the path queried.
 */
enum PathStatus audioPath_getStatus(const pathId id);

/**
 * Release an audio path.
 *
 * @param id: identifier of the path.
 */
void audioPath_release(const pathId id);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PATH_H */
