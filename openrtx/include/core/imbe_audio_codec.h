/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
 *                                                                         *
 *   (2025) Modified by KD0OSS for use in Module17/OpenRTX                 *
 ***************************************************************************/

#ifndef AMBE_AUDIO_CODEC_H
#define AMBE_AUDIO_CODEC_H

#include <audio_path.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialise audio codec manager, allocating data buffers.
 *
 * This function allows recursive calls. However the number of times this
 * function is called must be balanced by an equal number of calls to the
 * terminate() function.
 */
void imbe_init();

/**
 * Shutdown audio codec manager and deallocate data buffers.
 */
void imbe_terminate();

/**
 * Start encoding of audio data from a given audio source.
 * Only an encoding or decoding operation at a time is possible: in case there
 * is already an operation in progress, this function returns false.
 *
 * @param path: audio path for encoding source.
 * @return true on success, false on failure.
 */
bool imbe_startEncode(const pathId path);

/**
 * Start dencoding of audio data sending the uncompressed samples to a given
 * audio destination.
 * Only an encoding or decoding operation at a time is possible: in case there
 * is already an operation in progress, this function returns false.
 *
 * @param path: audio path for decoded audio.
 * @return true on success, false on failure.
 */
bool imbe_startDecode(const pathId path);

/**
 * Stop an ongoing encoding or decoding operation.
 *
 * @param path: audio path on which the encoding or decoding operation was
 * started.
 */
void imbe_stop(const pathId path);

/**
 * Get current oprational status of the codec thread.
 *
 * @return true if the codec thread is active.
 */
bool imbe_running();


#ifdef __cplusplus
}
#endif

#endif
