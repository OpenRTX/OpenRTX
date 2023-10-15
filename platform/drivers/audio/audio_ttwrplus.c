/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <interfaces/audio.h>

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
