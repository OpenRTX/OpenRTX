/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/audio_stream.h"

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
