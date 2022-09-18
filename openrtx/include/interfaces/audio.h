/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This file provides a common interface for the platform-dependent low-level
 * audio driver, in charge of managing microphone and audio amplifier.
 */

enum AudioSource
{
    SOURCE_MIC,        ///< Receive audio signal from the microphone
    SOURCE_RTX,        ///< Receive audio signal from the transceiver
    SOURCE_MCU         ///< Receive audio signal from a memory buffer
};

enum AudioSink
{
    SINK_SPK,          ///< Send audio signal to the speaker
    SINK_RTX,          ///< Send audio signal to the transceiver
    SINK_MCU           ///< Send audio signal to a memory buffer
};

enum AudioPriority
{
    PRIO_BEEP = 1,     ///< Priority level of system beeps
    PRIO_RX,           ///< Priority level of incoming audio from RX stage
    PRIO_PROMPT,       ///< Priority level of voice prompts
    PRIO_TX            ///< Priority level of outward audio directed to TX stage
};

/**
 * Initialise low-level audio management module.
 */
void audio_init();

/**
 * Shut down low-level audio management module.
 */
void audio_terminate();

/**
 * Enable microphone.
 */
void audio_enableMic();

/**
 * Disable microphone.
 */
void audio_disableMic();

/**
 * Enable audio PA.
 */
void audio_enableAmp();

/**
 * Disable audio PA.
 */
void audio_disableAmp();

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_H */
