/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/audio.h"
#include "drivers/baseband/SA8x8.h"

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
    /*
     * Squelch implementation:
     * when an audio path is created between SOURCE_RTX and SINK_SPK, unmute
     * speaker power amplifier to hear analog fm audio.
     */
    if (source == SOURCE_RTX && sink == SINK_SPK)
        sa8x8_setAudio(true);
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    /*
     * Squelch implementation:
     * when an audio path is released between SOURCE_RTX and SINK_SPK, mute
     * speaker power amplifier to squelch noise.
     */
    if (source == SOURCE_RTX && sink == SINK_SPK)
        sa8x8_setAudio(false);
}

bool audio_checkPathCompatibility(const enum AudioSource p1Source,
                                  const enum AudioSink   p1Sink,
                                  const enum AudioSource p2Source,
                                  const enum AudioSink   p2Sink)

{
    static const uint8_t RTX_SPK = (SOURCE_RTX * 3) + SINK_SPK;
    static const uint8_t MIC_RTX = (SOURCE_MIC * 3) + SINK_RTX;

    uint8_t p1 = (p1Source * 3) + p1Sink;
    uint8_t p2 = (p2Source * 3) + p2Sink;

    // RTX-SPK and MIC-RTX are compatible
    if((p1 == RTX_SPK) && (p2 == MIC_RTX))
        return true;

    // Same as above but with the paths swapped
    if((p1 == MIC_RTX) && (p2 == RTX_SPK))
        return true;

    // Disallow all the other path combinations
    return false;
}
