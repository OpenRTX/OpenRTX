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

#ifndef AUDIO_STREAM_H
#define AUDIO_STREAM_H

#include <stdint.h>
#include <sys/types.h>
#include "audio_path.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int16_t stream_sample_t;
typedef int8_t  streamId;

enum BufMode
{
    BUF_LINEAR,        ///< Linear buffer mode, conversion stops when full.
    BUF_CIRC_DOUBLE    ///< Circular double buffer mode, conversion never stops, thread woken up whenever half of the buffer is full.
};

typedef struct
{
    stream_sample_t *data;
    size_t len;
}
dataBlock_t;

/**
 * Start the acquisition of an incoming audio stream, also opening the
 * corresponding audio path. If a stream is opened from the same source but
 * with an higher priority than the one currently open, the new stream takes
 * over the previous one.
 * The function returns an error when the audio path is not available or the
 * selected stream is already in use by another process with higher priority.
 *
 * @param source: input source specifier.
 * @param prio: priority of the requester.
 * @param buf: pointer to a buffer used for management of sampled data.
 * @param bufLength: length of the buffer, in elements.
 * @param mode: buffer management mode.
 * @param sampleRate: sample rate, in Hz.
 * @return a unique identifier for the stream or -1 if the stream could not be opened.
 */
streamId inputStream_start(const enum AudioSource source,
                           const enum AudioPriority prio,
                           stream_sample_t * const buf,
                           const size_t bufLength,
                           const enum BufMode mode,
                           const uint32_t sampleRate);

/**
 * Get a chunk of data from an already opened input stream, blocking function.
 * If buffer management is configured to BUF_LINEAR this function also starts a
 * new data acquisition.
 *
 * @param id: identifier of the stream from which data is get.
 * @return dataBlock_t containing a pointer to the chunk head and its length. If
 * another thread is pending on this function, it returns immediately a dataBlock_t
 * cointaining < NULL, 0 >.
 */
dataBlock_t inputStream_getData(streamId id);

/**
 * Release the current input stream, allowing for a new call of startInputStream.
 * If this function is called when sampler is running, acquisition is stopped and
 * any thread waiting on getData() is woken up and given a partial result.
 *
 * @param id: identifier of the stream to be stopped
 */
void inputStream_stop(streamId id);

/**
 * Send an audio stream to a given output. This function returns immediately if
 * there is not another stream already running with the same destination and
 * priority of the ones specified, otherwise it will block the caller until the
 * previous stream terminates.
 * If a stream is opened from the same source but with an higher priority than
 * the one currently open, the new stream takes over the previous one.
 *
 * WARNING: the caller must ensure that buffer content is not modified while the
 * stream is being reproduced.
 *
 * @param destination: destination of the output stream.
 * @param prio: priority of the requester.
 * @param buf: buffer containing the audio samples.
 * @param length: length of the buffer, in elements.
 * @param sampleRate: sample rate in Hz.
 * @return a unique identifier for the stream or -1 if the stream could not be opened.
 */
streamId outputStream_start(const enum AudioSink destination,
                            const enum AudioPriority prio,
                            stream_sample_t * const buf,
                            const size_t length,
                            const uint32_t sampleRate);

/**
 * Interrupt a currently ongoing output stream before its natural ending.
 *
 * @param id: identifier of the stream to be stopped.
 */
void outputStream_stop(streamId id);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_STREAM_H */
