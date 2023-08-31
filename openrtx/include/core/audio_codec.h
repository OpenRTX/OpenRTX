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
 ***************************************************************************/

#ifndef AUDIO_CODEC_H
#define AUDIO_CODEC_H

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
void codec_init();

/**
 * Shutdown audio codec manager and deallocate data buffers.
 */
void codec_terminate();

/**
 * Start encoding of audio data from a given audio source.
 * Only an encoding or decoding operation at a time is possible: in case there
 * is already an operation in progress, this function returns false.
 *
 * @param path: audio path for encoding source.
 * @return true on success, false on failure.
 */
bool codec_startEncode(const pathId path);

/**
 * Start dencoding of audio data sending the uncompressed samples to a given
 * audio destination.
 * Only an encoding or decoding operation at a time is possible: in case there
 * is already an operation in progress, this function returns false.
 *
 * @param path: audio path for decoded audio.
 * @return true on success, false on failure.
 */
bool codec_startDecode(const pathId path);

/**
 * Stop an ongoing encoding or decoding operation.
 *
 * @param path: audio path on which the encoding or decoding operation was
 * started.
 */
void codec_stop(const pathId path);

/**
 * Get current oprational status of the codec thread.
 *
 * @return true if the codec thread is active.
 */
bool codec_running();

/**
 * Get a compressed audio frame from the internal queue. Each frame is composed
 * of 8 bytes.
 *
 * @param frame: pointer to a destination buffer where to put the encoded frame.
 * @param blocking: if true the execution flow will be blocked whenever the
 * internal buffer is empty and resumed as soon as an encoded frame is available.
 * @return zero on success, -EAGAIN if the queue is empty and the function is
 * nonblocking or -EPERM if there is no encoding operation ongoing.
 */
int codec_popFrame(uint8_t *frame, const bool blocking);

/**
 * Push a a compressed audio frame to the internal queue for decoding.
 * Each frame is composed of 8 bytes.
 *
 * @param frame: frame to be pushed to the queue.
 * @param blocking: if true the execution flow will be blocked whenever the
 * internal buffer is full and resumed as soon as space for an encoded frame is
 * available.
 * @return zero on success, -EAGAIN if the queue is full and the function is
 * nonblocking or -EPERM if there is no decoding operation ongoing.
 */
int codec_pushFrame(const uint8_t *frame, const bool blocking);

#ifdef __cplusplus
}
#endif

#endif
