/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AUDIO_PATH_H
#define AUDIO_PATH_H

#include "interfaces/audio.h"
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
