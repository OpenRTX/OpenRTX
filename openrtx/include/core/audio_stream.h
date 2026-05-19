/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AUDIO_STREAM_H
#define AUDIO_STREAM_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "core/audio_path.h"
#include "interfaces/audio.h"

#ifdef __cplusplus
extern "C" {
#endif

enum StreamMode
{
    STREAM_INPUT  = 0x10,    ///< Open an audio stream in input mode.
    STREAM_OUTPUT = 0x20     ///< Open an audio stream in output mode.
};

typedef int8_t streamId;


typedef struct
{
    stream_sample_t *data;
    size_t len;
}
dataBlock_t;

/**
 * Start an audio stream, either in input or output mode as specified by the
 * corresponding parameter.
 *
 * WARNING: for output streams the caller must ensure that buffer content is not
 * modified while the stream is being reproduced.
 *
 * @param path: audio path for the stream.
 * @param buf: buffer containing the audio samples.
 * @param length: length of the buffer, in elements.
 * @param sampleRate: sample rate in Hz.
 * @param mode: operation mode of the buffer
 * @return a unique identifier for the stream or a negative error code.
 */
streamId audioStream_start(const pathId path, stream_sample_t * const buf,
                           const size_t length, const uint32_t sampleRate,
                           const uint8_t mode);

/**
 * Request termination of a currently ongoing audio stream.
 * Stream is effectively stopped only when all the remaining data have been
 * processed, execution flow is blocked in the meantime.
 *
 * @param id: identifier of the stream to be stopped.
 */
void audioStream_stop(const streamId id);

/**
 * Interrupt a currently ongoing audio stream before its natural ending.
 *
 * @param id: identifier of the stream to be stopped.
 */
void audioStream_terminate(const streamId id);

/**
 * Get a chunk of data from an already opened input stream, blocking function.
 * If buffer management is configured to BUF_LINEAR this function also starts a
 * new data acquisition.
 *
 * @param id: identifier of the stream from which data is get.
 * @return dataBlock_t containing a pointer to the chunk head and its length. If
 * another thread is pending on this function, it returns immediately a
 * dataBlock_t cointaining < NULL, 0 >.
 */
dataBlock_t inputStream_getData(streamId id);

/**
 * Get a pointer to the section of the sample buffer not currently being read
 * by the DMA peripheral. The function is to be used primarily when the output
 * stream is running in double-buffered circular mode for filling a new block
 * of data to the stream.
 *
 * @param id: stream identifier.
 * @return a pointer to the idle section of the sample buffer or nullptr if the
 * stream is running in linear mode.
 */
stream_sample_t *outputStream_getIdleBuffer(const streamId id);

/**
 * Synchronise with the output stream DMA transfer, blocking function.
 * When the stream is running in circular mode, execution is blocked until
 * either the half or the end of the buffer is reached. In linear mode execution
 * is blocked until the end of the buffer is reached.
 * If the stream is not running or there is another thread waiting at the
 * synchronisation point, the function returns immediately.
 *
 * @param id: stream identifier.
 * @param bufChanged: if true, notifies the stream handler that new data has
 * been written to the idle section of the data buffer. This field is valid
 * only in circular double buffered mode.
 * @return true if execution was effectively blocked, false if stream is not
 * running or there is another thread waiting at the synchronisation point.
 */
bool outputStream_sync(const streamId id, const bool bufChanged);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_STREAM_H */
