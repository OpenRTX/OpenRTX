/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/audio.h"

const struct audioDevice outputDevices[] =
{
    {NULL, 0, 0, SINK_MCU},
    {NULL, 0, 0, SINK_RTX},
    {NULL, 0, 0, SINK_SPK},
};

const struct audioDevice inputDevices[] =
{
    {NULL, 0, 0, SINK_MCU},
    {NULL, 0, 0, SINK_RTX},
    {NULL, 0, 0, SINK_SPK},
};

void audio_init()
{

}

void audio_terminate()
{

}

void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
    (void) source;
    (void) sink;
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    (void) source;
    (void) sink;
}

bool audio_checkPathCompatibility(const enum AudioSource p1Source,
                                  const enum AudioSink   p1Sink,
                                  const enum AudioSource p2Source,
                                  const enum AudioSink   p2Sink)

{
    (void) p1Source;
    (void) p1Sink;
    (void) p2Source;
    (void) p2Sink;

    return false;
}
