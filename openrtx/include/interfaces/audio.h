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

#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This file provides a common interface for the platform-dependent low-level
 * audio driver, in charge of managing microphone and audio amplifier.
 */

enum AudioSource
{
    SOURCE_MIC = 0,    ///< Receive audio signal from the microphone
    SOURCE_RTX = 1,    ///< Receive audio signal from the transceiver
    SOURCE_MCU = 2     ///< Receive audio signal from a memory buffer
};

enum AudioSink
{
    SINK_SPK = 0,      ///< Send audio signal to the speaker
    SINK_RTX = 1,      ///< Send audio signal to the transceiver
    SINK_MCU = 2       ///< Send audio signal to a memory buffer
};

enum AudioPriority
{
    PRIO_BEEP = 1,     ///< Priority level of system beeps
    PRIO_RX,           ///< Priority level of incoming audio from RX stage
    PRIO_PROMPT,       ///< Priority level of voice prompts
    PRIO_TX            ///< Priority level of outward audio directed to TX stage
};

enum BufMode
{
    BUF_LINEAR,        ///< Linear buffer mode, conversion stops when full.
    BUF_CIRC_DOUBLE    ///< Circular double buffer mode, conversion never stops,
                       ///  thread woken up whenever half of the buffer is full.
};

typedef int16_t stream_sample_t;

/**
 * Initialise low-level audio management module.
 */
void audio_init();

/**
 * Shut down low-level audio management module.
 */
void audio_terminate();

/**
 * Connect an audio source to an audio sink.
 *
 * @param source: identifier of the input audio peripheral.
 * @param sink: identifier of the output audio peripheral.
 */
void audio_connect(const enum AudioSource source, const enum AudioSink sink);

/**
 * Disconnect an audio source from an audio sink.
 *
 * @param source: identifier of the input audio peripheral.
 * @param sink: identifier of the output audio peripheral.
 */
void audio_disconnect(const enum AudioSource source, const enum AudioSink sink);

/**
 * Check if two audio paths are compatible that is, if they can be open at the
 * same time.
 *
 * @param p1Source: identifier of the input audio peripheral of the first path.
 * @param p1Sink: identifier of the output audio peripheral of the first path.
 * @param p2Source: identifier of the input audio peripheral of the second path.
 * @param p2Sink: identifier of the output audio peripheral of the second path.
 */
bool audio_checkPathCompatibility(const enum AudioSource p1Source,
                                  const enum AudioSink   p1Sink,
                                  const enum AudioSource p2Source,
                                  const enum AudioSink   p2Sink);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_H */
