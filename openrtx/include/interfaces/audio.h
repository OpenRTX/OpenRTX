/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This file provides a common interface for the platform-dependent low-level
 * audio driver.
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
    BUF_LINEAR = 1,    ///< Linear buffer mode, conversion stops when full.
    BUF_CIRC_DOUBLE    ///< Circular double buffer mode, conversion never stops,
                       ///  thread woken up whenever half of the buffer is full.
};

typedef int16_t stream_sample_t;

/**
 * Data structure for audio stream context, to be used to configure audio
 * devices.
 */
struct streamCtx
{
    void            *priv;       ///< Pointer to audio device private data.
    stream_sample_t *buffer;     ///< Pointer to audio data buffer.
    size_t           bufSize;    ///< Size of the audio data buffer, in elements.
    uint8_t          bufMode;    ///< Buffer handling mode, linear or circular double.
    uint32_t         sampleRate; ///< Sample rate, in Hz.
    uint8_t          running;    ///< Audio device status, set to 1 when active.
}
__attribute__((packed));

/**
 * Driver interface for a generic audio device, either an input or output device,
 * bound to a specific input.
 */
struct audioDriver
{
    /**
     * Start an audio stream to or from the device.
     *
     * @param instance: driver instance number.
     * @param config: driver configuration.
     * @param ctx: pointer to audio stream context.
     * @return zero on success, a negative error code on failure.
     */
    int (*start)(const uint8_t instance, const void *config, struct streamCtx *ctx);

    /**
     * Get a pointer to the "free" data section, when running in circular double
     * buffered mode. For output streams the free data section is the one which
     * can be filled with new samples, for input streams is the section containing
     * the last acquired samples.
     *
     * @param ctx: pointer to audio stream context.
     * @param buf: pointer to the free section pointer.
     * @return size of the free data section.
     */
    int (*data)(struct streamCtx *ctx, stream_sample_t **buf);

    /**
     * Synchronize the execution flow with the driver. Execution is blocked
     * until the driver reaches a syncpoint, either the end or the middle of the
     * data stream.
     *
     * @param ctx: pointer to audio stream context.
     * @param dirty: flag to signal to the driver that the "free" data section
     * contains new data. Meaningful only for output streams.
     * @return zero on success, -1 if the calling thread was not blocked.
     */
    int (*sync)(struct streamCtx *ctx, uint8_t dirty);

    /**
     * Stop an ongoing data stream.
     * The stream is effectively stopped once it reaches the next syncpoint.
     *
     * @param ctx: pointer to audio stream context.
     */
    void (*stop)(struct streamCtx *ctx);

    /**
     * Immediately stop an ongoing data stream.
     *
     * @param ctx: pointer to audio stream context.
     */
    void (*terminate)(struct streamCtx *ctx);
};

/**
 * Audio device descriptor, grouping an audio driver, its configuration and
 * its input/output endpoint.
 */
struct audioDevice
{
    const struct audioDriver *driver;    ///< Audio driver functions
    const void               *config;    ///< Driver configuration
    const uint8_t             instance;  ///< Driver instance number
    const uint8_t             endpoint;  ///< Driver sink or source endpoint
}
__attribute__((packed));

extern const struct audioDevice inputDevices[];
extern const struct audioDevice outputDevices[];

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
