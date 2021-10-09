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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
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
    SINK_MCU           ///< Send audio signal to a memory buffer
};

enum AudioPriority
{
    PRIO_BEEP = 1,     ///< Priority level of system beeps
    PRIO_RX,           ///< Priority level of incoming audio from RX stage
    PRIO_PROMPT,       ///< Priority level of voice prompts
    PRIO_TX            ///< Priority level of outward audio directed to TX stage
};

typedef int8_t pathId;

/**
 * Try to connect an audio path, returns an error if the path is already used
 * with an higher priority.
 *
 * @param source: identifier of the input audio peripheral.
 * @param sink: identifier of the output audio peripheral.
 * @param prio: priority of the requester.
 * @return a unique identifier of the opened path or -1 if path is already in use.
 */
pathId audioPath_open(enum AudioSource source,
                      enum AudioSink sink,
                      enum AudioPriority prio);

/**
 * Release an audio path.
 *
 * @param id: identifier of the previously opened path.
 * @return true on success, false on error.
 */
bool audioPath_close(pathId id);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PATH_H */
