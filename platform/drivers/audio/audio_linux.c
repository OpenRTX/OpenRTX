/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/audio.h"
#include "hwconfig.h"
#include "file_source.h"

static const uint8_t pathCompatibilityMatrix[9][9] =
{
    // MIC-SPK MIC-RTX MIC-MCU RTX-SPK RTX-RTX RTX-MCU MCU-SPK MCU-RTX MCU-MCU
    {    0   ,   0   ,   0   ,   1   ,   0   ,   1   ,   1   ,   0   ,   1   },  // MIC-RTX
    {    0   ,   0   ,   0   ,   0   ,   1   ,   1   ,   0   ,   1   ,   1   },  // MIC-SPK
    {    0   ,   0   ,   0   ,   1   ,   1   ,   0   ,   1   ,   1   ,   0   },  // MIC-MCU
    {    0   ,   1   ,   1   ,   0   ,   0   ,   0   ,   0   ,   1   ,   1   },  // RTX-SPK
    {    1   ,   0   ,   1   ,   0   ,   0   ,   0   ,   1   ,   0   ,   1   },  // RTX-RTX
    {    1   ,   1   ,   0   ,   0   ,   0   ,   0   ,   1   ,   1   ,   0   },  // RTX-MCU
    {    0   ,   1   ,   1   ,   0   ,   1   ,   1   ,   0   ,   0   ,   0   },  // MCU-SPK
    {    1   ,   0   ,   1   ,   1   ,   0   ,   1   ,   0   ,   0   ,   0   },  // MCU-RTX
    {    1   ,   1   ,   0   ,   1   ,   1   ,   0   ,   0   ,   0   ,   0   }   // MCU-MCU
};

const struct audioDevice outputDevices[] =
{
    {NULL, 0, 0, SINK_MCU},
    {NULL, 0, 0, SINK_RTX},
    {NULL, 0, 0, SINK_SPK},
};

const struct audioDevice inputDevices[] =
{
    {NULL,                      0,                   0, SOURCE_MCU},
    {&file_source_audio_driver, "/tmp/baseband.raw", 0, SOURCE_RTX},
    {NULL,                      0,                   0, SOURCE_MIC},
};

void audio_init()
{

}

void audio_terminate()
{

}

void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
    #ifdef VERBOSE
    printf("Connect ");
    switch(source)
    {
        case SOURCE_MIC: printf("MIC"); break;
        case SOURCE_RTX: printf("RTX"); break;
        case SOURCE_MCU: printf("MCU"); break;
        default:                        break;
    }

    printf(" with ");
    switch(sink)
    {
        case SINK_SPK: printf("SPK\n"); break;
        case SINK_RTX: printf("RTX\n"); break;
        case SINK_MCU: printf("MCU\n"); break;
        default:                        break;
    }
    #else
    (void) source;
    (void) sink;
    #endif
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    #ifdef VERBOSE
    printf("Disconnect ");
    switch(source)
    {
        case SOURCE_MIC: printf("MIC"); break;
        case SOURCE_RTX: printf("RTX"); break;
        case SOURCE_MCU: printf("MCU"); break;
        default:                        break;
    }

    printf(" from ");
    switch(sink)
    {
        case SINK_SPK: printf("SPK\n"); break;
        case SINK_RTX: printf("RTX\n"); break;
        case SINK_MCU: printf("MCU\n"); break;
        default:                        break;
    }
    #else
    (void) source;
    (void) sink;
    #endif
}

bool audio_checkPathCompatibility(const enum AudioSource p1Source,
                                  const enum AudioSink   p1Sink,
                                  const enum AudioSource p2Source,
                                  const enum AudioSink   p2Sink)

{
    uint8_t p1Index = (p1Source * 3) + p1Sink;
    uint8_t p2Index = (p2Source * 3) + p2Sink;

    return pathCompatibilityMatrix[p1Index][p2Index] == 1;
}
