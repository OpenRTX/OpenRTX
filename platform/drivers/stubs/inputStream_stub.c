/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/audio_stream.h"

streamId inputStream_start(const enum AudioSource source,
                           const enum AudioPriority prio,
                           stream_sample_t * const buf,
                           const size_t bufLength,
                           const enum BufMode mode,
                           const uint32_t sampleRate)
{
    (void) source;
    (void) prio;
    (void) buf;
    (void) bufLength;
    (void) mode;
    (void) sampleRate;

    return -1;
}

dataBlock_t inputStream_getData(streamId id)
{
    (void) id;

    dataBlock_t block;
    block.data = NULL;
    block.len  = 0;

    return block;
}

void inputStream_stop(streamId id)
{
    (void) id;
}
