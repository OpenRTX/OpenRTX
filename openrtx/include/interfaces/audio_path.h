/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef AUDIO_PATH_H
#define AUDIO_PATH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
    SINK_MCU           ///< Send audio signal from a memory buffer
};

enum AudioPriority
{
    PRIO_BEEP = 1,     ///< Beeps have the lowest priority
    PRIO_RX,           ///< Rx can take over beeps
    PRIO_PROMPT,       ///< Voice prompt can take over Rx or beep
    PRIO_TX            ///< Tx can always take over and will never be interrupted
};

typedef int8_t pathId

/********************* Priority Management Functions ***********************/

/**
 * Try to connect an audio path
 * Returns an error if the path is already used with a higher priority
 * 
 * @param source: identifier of the input audio peripheral
 * @param sink: identifier of the output audio peripheral
 * @param prio: priority of the requester
 * @return a unique identifier of the opened path or -1 if path is already used.
 */
pathId audioPath_open(AudioSource source, AudioSink sink, AudioPriority prio);

/**
 * Release an audio path
 * 
 * @param id: identifier of the previously opened path
 * @return true on success, false on error
 */
bool audioPath_close(pathId id);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PATH_H */
