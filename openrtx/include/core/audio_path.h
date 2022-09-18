/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
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

#ifndef AUDIO_PATH_H
#define AUDIO_PATH_H

#include <interfaces/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

enum PathStatus
{
    PATH_CLOSED,
    PATH_OPEN,
    PATH_SUSPENDED
};

typedef int32_t pathId;


/**
 * Request to set up an audio path, returns an error if the path is already used
 * with an higher priority.
 *
 * @param source: identifier of the input audio peripheral.
 * @param destination: identifier of the output audio peripheral.
 * @param prio: priority of the requester.
 * @return a unique identifier of the opened path or -1 if path is already in use.
 */
pathId audioPath_request(enum AudioSource source, enum AudioSink destination,
                         enum AudioPriority prio);

/**
 * Get the current status of an audio path.
 *
 * @param pathId: ID of the audio path.
 * @return status of the path queried.
 */
enum PathStatus audioPath_getStatus(const pathId pathId);

/**
 * Release an audio path.
 *
 * @param pathId: identifier of the path.
 */
void audioPath_release(const pathId pathId);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PATH_H */
