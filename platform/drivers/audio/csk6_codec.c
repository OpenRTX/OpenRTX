/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <csk6_cm33/include/cache.h>
#include <csk6_cm33/include/venus_ap.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <hwconfig.h>
#include "csk6_codec.h"
#include "AudioSystem.h"
#include "AudioTrack.h"
#include "AudioRecord.h"

/* One worker services the DSP's 48 kHz stereo transport:
 *   capture: microphone left, radio right -> application buffers or FM queues
 *   playback: application buffers or FM queues -> speaker left, radio right
 * Application rates use sample selection/repetition without filtering.
 * Circular streams run continuously; callers process the idle half between
 * boundary notifications. FM routes have no application buffer and use queues.
 * The worker sleeps while nothing is captured or played.
 */
LOG_MODULE_REGISTER(csk6_codec, LOG_LEVEL_INF);

#define DSP_SAMPLE_RATE 48000
/* Stereo transport channels, indexed by the AudioSource and AudioSink values */
#define CODEC_CHANNELS 2
BUILD_ASSERT(((int)SOURCE_MIC == (int)SINK_SPK)
                 && ((int)SOURCE_RTX == (int)SINK_RTX)
                 && (SOURCE_MCU == CODEC_CHANNELS)
                 && (SINK_MCU == CODEC_CHANNELS),
             "Codec channels must match the audio source and sink values");
#define CAPTURE_FRAME_MS 10
#define CAPTURE_FRAME_SAMPLES \
    (DSP_SAMPLE_RATE * CAPTURE_FRAME_MS / MSEC_PER_SEC)
#define RECORD_CHANNELS (CHANNEL_IN_LEFT | CHANNEL_IN_RIGHT)
/* Longest key=value string sent to the DSP */
#define DSP_PARAM_LEN 128
/*
 * Default arguments of the LSF audio API, as ListenAI's audio_recorder sample
 * passes them: parameters for every stream, the default input source, the
 * default playback buffer size and no wait for a playback buffer.
 * https://github.com/OpenRTX/lsf-zephyr-sdk/blob/2ac605527dcde3e90c814bb4175546cc7fbd36da/samples/audio_recorder/src/main.c
 */
#define DSP_IO_ALL 0
#define DSP_INPUT_SOURCE_DEFAULT 0
#define DSP_TRACK_FRAMES_DEFAULT 0
#define DSP_BUFFER_NO_WAIT 0
/* Capacity per FM output; not a target fill level. */
#define QUEUE_MS 160
#define QUEUE_SAMPLES (DSP_SAMPLE_RATE * QUEUE_MS / MSEC_PER_SEC)
/* Allowance for a stalled device, on top of one buffer period */
#define IO_TIMEOUT_MS 1000
/* Added to the drain estimate, which cannot see the DAC */
#define DRAIN_MARGIN_MS 20
#define NO_SOURCE (-1)
/* Above the rtx and UI threads (Zephyr priority 14 as POSIX default), below
 * main, so the DSP streams are paced on time */
#define WORKER_PRIORITY 5
/* Static worst case about 0.9 KiB, with immediate-mode logging and an FPU
 * exception frame, from -fcallgraph-info=su */
#define WORKER_STACK_SIZE 2048

/*
 * CSK6 audio codec ADC01, the AON codec, not the CP's ADC23. Not in
 * hal_listenai: from ListenAI's Venus HAL codec driver, as used by OE3ANC in
 * https://github.com/OE3ANC/OpenRTX-c62/commit/8a160732f36755d2d42e3673cb10c7243c8f32a2
 */
#define CODEC_ADC01_BASE (AON_VAD_BASE + 0x20000)

/** Codec ADC register offsets. */
enum codecAdcReg {
    CODEC_ADC_R5 = 0x14, ///< Enable and reset
    CODEC_ADC_R6 = 0x18, ///< Filters
};

/**
 * Codec ADC register fields. The source names R5 bits 7 and 8 together, as
 * enable and reset release, without saying which is which.
 */
enum codecAdcField {
    CODEC_ADC_R5_ACTIVE = BIT(7) | BIT(8), ///< Enabled and out of reset
    CODEC_ADC_R6_HPF2 = BIT(14),           ///< HPF2 on both channels
    CODEC_ADC_R6_HPF1 = BIT(15),           ///< HPF1, left as it is
};

/* FM passthrough only: bridge separately available capture/playback frames. */
struct sampleQueue {
    int16_t samples[QUEUE_SAMPLES]; ///< Ring of 48 kHz samples
    size_t head;                    ///< Oldest sample
    size_t count;                   ///< Samples queued
};

/* The worker continuously traverses the attached application buffer, like
 * DMA. sync() observes half/full boundaries; it does not submit transfers. */
struct endpoint {
    struct streamCtx *ctx; ///< Attached stream, or NULL
    size_t position;       ///< Next application sample to read or write
    size_t channel;        ///< Codec channel: SINK_* or SOURCE_* value
    uint32_t boundaries;   ///< Completed halves, or linear completions
    uint32_t observed;     ///< Last boundary acknowledged by sync()
    unsigned int divisor;  ///< 48 kHz samples per application sample
    int error;             ///< Error that stopped the stream, or 0
    bool input;            ///< Capture endpoint
    bool busy;             ///< A sync/stop callback is waiting on it
    bool stop;             ///< Finish the current output block, then drain
};

struct inputState {
    struct endpoint endpoint; ///< Stream state
    unsigned int phase;       ///< 48 kHz samples since the last one kept
};

struct outputState {
    int64_t deadline;           ///< Drain end, or 0 until it is estimated
    struct endpoint endpoint;   ///< Stream state
    struct sampleQueue fmQueue; ///< FM passthrough samples
    unsigned int phase;         ///< Repeats of the current sample played
    bool draining;              ///< Last block sent, waiting for the DAC
};

static struct inputState inputs[CODEC_CHANNELS]
    __attribute__((section(".psram_section")));
static struct outputState outputs[CODEC_CHANNELS]
    __attribute__((section(".psram_section")));
/* Protect routing, endpoint state and worker accesses to application buffers.
 * Callers access only the idle half; they must finish before it becomes active.
 * Condition waits release this mutex so the worker and cancellation can run. */
static K_MUTEX_DEFINE(audioMutex);
static K_CONDVAR_DEFINE(changed);
/* Wakes an idle worker: a route or stream changed, or a stop was requested */
static K_SEM_DEFINE(workerWake, 0, 1);
static atomic_t stopRequested;
static bool running;
/* Desired routing, written by board callbacks and applied by the worker. */
static bool inputEnabled[CODEC_CHANNELS];
static int outputSource[CODEC_CHANNELS] = { NO_SOURCE, NO_SOURCE };
static bool routesDirty;
/* Only openDsp(), from csk6codec_init(), and then the worker access vendor
 * objects and actual device state. */
static AudioRecord record;
static AudioTrack playbackTrack;
static bool captureRunning;
static bool playbackRunning;
static int64_t playbackDeadline;
static uint32_t playbackFraction;
static struct k_thread worker;
static K_THREAD_STACK_DEFINE(workerStack, WORKER_STACK_SIZE);

/**
 * Empty an FM queue.
 */
static void queueReset(struct sampleQueue *q)
{
    q->head = 0;
    q->count = 0;
}

/**
 * Discard the oldest samples of an FM queue.
 *
 * @param q: queue.
 * @param count: samples to discard, at most the queue count.
 */
static void queueDrop(struct sampleQueue *q, size_t count)
{
    q->head = (q->head + count) % QUEUE_SAMPLES;
    q->count -= count;
}

/**
 * Append samples to an FM queue, discarding the oldest when it is full.
 *
 * @param q: queue.
 * @param src: first sample.
 * @param stride: distance between samples, in samples.
 * @param count: samples to append, at most QUEUE_SAMPLES.
 */
static void queueWrite(struct sampleQueue *q, const int16_t *src, size_t stride,
                       size_t count)
{
    size_t space = QUEUE_SAMPLES - q->count;
    if (count > space)
        queueDrop(q, count - space);

    size_t tail = (q->head + q->count) % QUEUE_SAMPLES;
    while (count > 0) {
        size_t run = MIN(count, QUEUE_SAMPLES - tail);
        for (size_t i = 0; i < run; i++)
            q->samples[tail + i] = src[i * stride];

        src += run * stride;
        count -= run;
        q->count += run;
        tail = 0;
    }
}

/**
 * Take the oldest samples from an FM queue.
 *
 * @param q: queue.
 * @param dst: first sample to write.
 * @param stride: distance between samples, in samples.
 * @param count: samples wanted.
 * @return the samples written, fewer than count if the queue ran empty.
 */
static size_t queueRead(struct sampleQueue *q, int16_t *dst, size_t stride,
                        size_t count)
{
    const size_t total = MIN(count, q->count);
    size_t done = 0;

    while (done < total) {
        size_t run = MIN(total - done, QUEUE_SAMPLES - q->head);
        for (size_t i = 0; i < run; i++)
            dst[(done + i) * stride] = q->samples[q->head + i];

        queueDrop(q, run);
        done += run;
    }
    return total;
}

/**
 * Samples between two boundary notifications: half the buffer for circular
 * streams, the whole buffer for linear ones.
 */
static size_t blockSamples(const struct streamCtx *ctx)
{
    if (ctx->bufMode == BUF_CIRC_DOUBLE)
        return ctx->bufSize / 2;
    return ctx->bufSize;
}

/**
 * Application samples left before an endpoint's next block boundary.
 */
static size_t blockRemaining(const struct endpoint *ep)
{
    const size_t block = blockSamples(ep->ctx);
    return block - ep->position % block;
}

/**
 * Check whether an endpoint has an attached stream that is still running.
 */
static bool endpointActive(const struct endpoint *ep)
{
    return (ep->ctx != NULL) && ep->ctx->running;
}

/**
 * Fail a stream: wake a waiting caller and stop further buffer transfers.
 */
static void failStream(struct endpoint *ep, int error)
{
    ep->error = error;
    if (ep->ctx != NULL)
        ep->ctx->running = 0;
    k_condvar_broadcast(&changed);
}

/**
 * Cancel and detach a stream. Call with audioMutex held, so no worker
 * access can outlive the detachment.
 */
static void detach(struct endpoint *ep)
{
    failStream(ep, -ECANCELED);
    ep->ctx = NULL;
}

/**
 * Count a completed buffer half (or linear buffer) and wake waiting callers.
 */
static void notifyBoundary(struct endpoint *ep)
{
    ep->boundaries++;
    k_condvar_broadcast(&changed);
}

/**
 * Deadline for a caller waiting on a stream: one buffer period, plus an
 * allowance for a stalled device.
 */
static int64_t ioDeadline(const struct streamCtx *ctx)
{
    return k_uptime_get() + IO_TIMEOUT_MS
         + (uint64_t)ctx->bufSize * MSEC_PER_SEC / ctx->sampleRate;
}

/**
 * Wait, with audioMutex held, until the worker or a canceller signals a
 * change.
 *
 * @param deadline: from ioDeadline().
 * @return false if the deadline passed.
 */
static bool waitForChange(int64_t deadline)
{
    int64_t remaining = deadline - k_uptime_get();
    if (remaining <= 0)
        return false;

    k_condvar_wait(&changed, &audioMutex, K_MSEC(remaining));
    return true;
}

/**
 * Check whether an output channel has something to play.
 */
static bool outputEnabled(size_t channel)
{
    if (outputSource[channel] == SOURCE_MCU)
        return (outputs[channel].endpoint.ctx != NULL)
            && (outputs[channel].endpoint.error == 0);
    return outputSource[channel] != NO_SOURCE;
}

/**
 * Reload a stream's shared layout, which the DSP can change while it
 * constructs the stream: drop the cached copy, then rebuild the MCU side.
 */
static struct ICStreamShare *refreshStreamLayout(ICStream *stream)
{
    struct ICStreamShare *share = stream->share;
    dcache_invalidate_range((unsigned long)share,
                            (unsigned long)share
                                + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(*share)));
    __DSB();
    ICStream_reconfig(stream);
    return share;
}

/**
 * Bypass the codec ADC's HPF2 after each capture start, to keep the received
 * baseband. Apply after the synchronous DSP start, which may configure the
 * codec.
 */
static void disableAdcHpf2(void)
{
    uint32_t r5 = sys_read32(CODEC_ADC01_BASE + CODEC_ADC_R5);
    uint32_t r6 = sys_read32(CODEC_ADC01_BASE + CODEC_ADC_R6);
    uint32_t bypass = r6 & ~CODEC_ADC_R6_HPF2;

    /* Only touch an ADC that is both enabled and out of reset */
    if ((r5 & CODEC_ADC_R5_ACTIVE) != CODEC_ADC_R5_ACTIVE) {
        LOG_ERR("ADC01 HPF2 bypass skipped: inactive R5=%08x R6=%08x", r5, r6);
        return;
    }
    sys_write32(bypass, CODEC_ADC01_BASE + CODEC_ADC_R6);
    __DSB();
    if (sys_read32(CODEC_ADC01_BASE + CODEC_ADC_R6) != bypass)
        LOG_ERR("ADC01 HPF2 bypass readback mismatch");
}

/**
 * Send a key=value parameter string to the DSP.
 *
 * @param kv: parameters, separated by ';'.
 * @return 0 on success.
 */
static int setDspParam(const char *kv)
{
    String8 param;
    String8_ctor_char(&param, kv);
    int ret = AudioSystem_setParameters(DSP_IO_ALL, &param);
    String8_dtor(&param);
    return ret;
}

/**
 * Set the mic (left) and BK4819 RX audio (right) ADC gains from hwconfig.h.
 *
 * @return 0 on success.
 */
static int configureAdcGain(void)
{
    char kv[DSP_PARAM_LEN];
    snprintf(kv, sizeof(kv),
             "ADC_PDM_GAIN_A_LEFT=%d;ADC_PDM_GAIN_D_LEFT=%d;"
             "ADC_PDM_GAIN_A_RIGHT=%d;ADC_PDM_GAIN_D_RIGHT=%d",
             C62_ADC_MIC_GAIN_A, C62_ADC_MIC_GAIN_D, C62_ADC_RX_GAIN_A,
             C62_ADC_RX_GAIN_D);
    return setDspParam(kv);
}

/**
 * Set the speaker (left) and BK4819 TX (right) DAC gains from hwconfig.h.
 *
 * @return 0 on success.
 */
static int configureDacGain(void)
{
    char kv[DSP_PARAM_LEN];
    snprintf(kv, sizeof(kv),
             "DAC_GAIN_A_LEFT=%d;DAC_GAIN_D_LEFT=%d;"
             "DAC_GAIN_A_RIGHT=%d;DAC_GAIN_D_RIGHT=%d",
             C62_DAC_SPK_GAIN_A, C62_DAC_SPK_GAIN_D, C62_DAC_TX_GAIN_A,
             C62_DAC_TX_GAIN_D);
    return setDspParam(kv);
}

/**
 * Set the fixed 48 kHz hardware rate and the capture channels, before the
 * streams are opened.
 *
 * @return 0 on success.
 */
static int configureSampleFormat(void)
{
    char kv[DSP_PARAM_LEN];
    snprintf(kv, sizeof(kv), "samplingRate=%u;channels=%u",
             (unsigned int)DSP_SAMPLE_RATE, (unsigned int)RECORD_CHANNELS);
    return setDspParam(kv);
}

/**
 * Check the capture stream layout, which the SDK's openRecord can change,
 * against what captureFrame() expects.
 *
 * @param rec: capture stream.
 * @return 0 if it matches.
 */
static int checkRecordLayout(const AudioRecord *rec)
{
    struct ICStreamShare *share = refreshStreamLayout(rec->mICStream);
    if ((share->sampleCountPerFrame != rec->mCblk->frameCount)
        || (share->channelCount != rec->mCblk->channels)
        || (share->sampleCountPerFrame <= 0)
        || (share->sampleCountPerFrame > QUEUE_SAMPLES)
        || (share->channelCount < CODEC_CHANNELS)) {
        LOG_ERR("DSP capture layout mismatch: frames=%d/%d channels=%d/%d",
                (int)share->sampleCountPerFrame, (int)rec->mCblk->frameCount,
                (int)share->channelCount, (int)rec->mCblk->channels);
        return -EINVAL;
    }
    return 0;
}

/**
 * Check that the playback stream is interleaved 16-bit stereo, as
 * playFrame() writes it.
 *
 * @param track: playback stream.
 * @return 0 if it is.
 */
static int checkPlaybackLayout(const AudioTrack *track)
{
    struct ICStreamShare *share = refreshStreamLayout(track->mICStream);
    if ((track->mCblk->channels != CODEC_CHANNELS)
        || (track->mCblk->frameSize != CODEC_CHANNELS * sizeof(int16_t))
        || (share->channelCount != CODEC_CHANNELS)
        || (share->cellSize != sizeof(int16_t))
        || (share->sampleCountPerFrame <= 0)) {
        LOG_ERR("DSP playback layout mismatch: channels=%d frameSize=%d "
                "shared channels=%d cellSize=%d",
                track->mCblk->channels, track->mCblk->frameSize,
                share->channelCount, share->cellSize);
        return -ENOTSUP;
    }
    return 0;
}

/**
 * Set the codec gains and open the 48 kHz capture and playback streams.
 * Called by csk6codec_init(); on failure, closes whatever it opened.
 *
 * @param rec: capture stream to open.
 * @param track: playback stream to open.
 * @return 0 on success.
 */
static int openDsp(AudioRecord *rec, AudioTrack *track)
{
    int ret = configureAdcGain();
    if (ret != 0) {
        LOG_ERR("Setting the ADC gains failed: %d", ret);
        return ret;
    }

    ret = configureSampleFormat();
    if (ret != 0) {
        LOG_ERR("Setting the 48 kHz sample format failed: %d", ret);
        return ret;
    }

    ret = AudioRecord_ctor(rec, DSP_INPUT_SOURCE_DEFAULT, DSP_SAMPLE_RATE,
                           PCM_16_BIT, RECORD_CHANNELS, CAPTURE_FRAME_SAMPLES,
                           NULL);
    if (ret != 0) {
        LOG_ERR("Opening the 48 kHz capture stream failed: %d", ret);
        return ret;
    }

    ret = checkRecordLayout(rec);
    if (ret != 0)
        goto closeRecord;

    ret = AudioTrack_ctor(track, DSP_SAMPLE_RATE, PCM_16_BIT,
                          CHANNEL_OUT_STEREO, DSP_TRACK_FRAMES_DEFAULT, NULL);
    if (ret != 0) {
        LOG_ERR("Opening the 48 kHz playback stream failed: %d", ret);
        goto closeRecord;
    }

    ret = checkPlaybackLayout(track);
    if (ret != 0)
        goto closePlayback;

    ret = configureDacGain();
    if (ret != 0) {
        LOG_ERR("Setting the DAC gains failed: %d", ret);
        goto closePlayback;
    }

    return 0;

closePlayback:
    AudioTrack_dtor(track);
closeRecord:
    AudioRecord_dtor(rec);
    return ret;
}

/**
 * Start or stop the DSP capture stream. Worker only.
 *
 * @param rec: capture stream.
 * @param enable: true to start it.
 */
static void setCapture(AudioRecord *rec, bool enable)
{
    if (enable == captureRunning)
        return;

    if (enable) {
        AudioRecord_start(rec);
        disableAdcHpf2();
    } else {
        AudioRecord_stop(rec);
    }
    captureRunning = enable;
}

/**
 * Start or stop the DSP playback stream. Worker only.
 *
 * @param track: playback stream.
 * @param enable: true to start it.
 */
static void setPlayback(AudioTrack *track, bool enable)
{
    if (enable == playbackRunning)
        return;

    if (enable) {
        AudioTrack_start(track);
        playbackDeadline = k_uptime_get();
        playbackFraction = 0;
    } else {
        AudioTrack_stop(track);
        AudioTrack_flush(track);
    }
    playbackRunning = enable;
}

/**
 * Start or stop capture and playback to match the routes and attached
 * streams. Worker only.
 *
 * @param rec: capture stream.
 * @param track: playback stream.
 */
static void applyRoutes(AudioRecord *rec, AudioTrack *track)
{
    bool capture = false;
    bool playback = false;
    for (size_t i = 0; i < CODEC_CHANNELS; i++) {
        capture |= inputEnabled[i] || (outputSource[i] == SOURCE_MIC)
                || (outputSource[i] == SOURCE_RTX);
        playback |= outputEnabled(i);
    }

    setCapture(rec, capture);
    setPlayback(track, playback);

    if (routesDirty) {
        routesDirty = false;
        k_condvar_broadcast(&changed);
    }
}

/**
 * Move an input endpoint past the samples just written. At a block boundary
 * notify the caller; at the end of the buffer wrap around, or stop a linear
 * stream.
 *
 * @param ep: input endpoint.
 * @param count: samples written, at most blockRemaining().
 */
static void inputAdvance(struct endpoint *ep, size_t count)
{
    struct streamCtx *ctx = ep->ctx;

    ep->position += count;
    if (ep->position % blockSamples(ctx) != 0)
        return;

    if (ep->position == ctx->bufSize) {
        ep->position = 0;
        if (ctx->bufMode == BUF_LINEAR)
            ctx->running = 0;
    }
    notifyBoundary(ep);
}

/**
 * Hand 48 kHz samples to an input stream, keeping every divisor-th sample.
 * Copies a run up to each block boundary. The phase persists across DSP
 * frames.
 *
 * @param in: input stream.
 * @param src: first sample.
 * @param stride: distance between samples, in samples.
 * @param count: samples.
 */
static void captureToStream(struct inputState *in, const int16_t *src,
                            size_t stride, size_t count)
{
    struct endpoint *ep = &in->endpoint;
    if (!endpointActive(ep))
        return;

    const unsigned int divisor = ep->divisor;
    size_t next = (divisor - in->phase) % divisor; /* First sample kept */
    in->phase = (in->phase + count) % divisor;

    while ((next < count) && endpointActive(ep)) {
        size_t run = MIN(DIV_ROUND_UP(count - next, divisor),
                         blockRemaining(ep));
        stream_sample_t *dst = &ep->ctx->buffer[ep->position];
        for (size_t i = 0; i < run; i++)
            dst[i] = src[(next + i * divisor) * stride];

        next += run * divisor;
        inputAdvance(ep, run);
    }
}

/**
 * Hand 48 kHz samples to the FM queue of each output routed from this
 * source.
 *
 * @param source: SOURCE_MIC or SOURCE_RTX.
 * @param src: first sample.
 * @param stride: distance between samples, in samples.
 * @param count: samples.
 */
static void captureToFmQueues(size_t source, const int16_t *src, size_t stride,
                              size_t count)
{
    for (size_t sink = 0; sink < CODEC_CHANNELS; sink++) {
        if (outputSource[sink] == (int)source)
            queueWrite(&outputs[sink].fmQueue, src, stride, count);
    }
}

/**
 * Fail every attached input stream.
 */
static void failInputs(int error)
{
    for (size_t i = 0; i < CODEC_CHANNELS; i++) {
        if (inputs[i].endpoint.ctx != NULL)
            failStream(&inputs[i].endpoint, error);
    }
}

/**
 * Take one capture frame, if any, and hand each channel to its input stream
 * and FM queues. AudioRecord_read() can block indefinitely and copies whole
 * frames, so this uses the SDK's stream, channel mapping and cache API
 * directly.
 *
 * @param rec: capture stream.
 */
static void captureFrame(AudioRecord *rec)
{
    ICStream *ic = rec->mICStream;
    ICStream_Consumer_fetchRemote(ic);
    if (ICStream_Consumer_isEmpty(ic))
        return;

    void *frame;
    if (ICStream_Consumer_acquireFrame(ic, &frame) != IC_OK) {
        failInputs(-EIO);
        return;
    }

    const int16_t *samples = frame;
    const size_t stride = ic->share->channelCount;
    const size_t count = rec->mCblk->frameCount;
    for (size_t source = 0; source < CODEC_CHANNELS; source++) {
        const int16_t *src = &samples[rec->mChannelOutIdx[source]];
        captureToStream(&inputs[source], src, stride, count);
        captureToFmQueues(source, src, stride, count);
    }

    int ret = ICStream_Consumer_releaseFrame(ic, frame);
    if (ret == IC_OK)
        ret = ICStream_Consumer_commitRemote(ic);
    if (ret != IC_OK)
        failInputs(-EIO);
}

/**
 * Move an output endpoint past the samples just played, each application
 * sample being repeated divisor times. At a block boundary either notify the
 * caller and wrap around, or, for the last block of a stopping or linear
 * stream, start draining.
 *
 * @param out: output stream.
 * @param count: 48 kHz samples played, up to the next block boundary.
 */
static void outputAdvance(struct outputState *out, size_t count)
{
    struct endpoint *ep = &out->endpoint;
    const size_t repeats = out->phase + count;

    ep->position += repeats / ep->divisor;
    out->phase = repeats % ep->divisor;
    if ((out->phase != 0) || (ep->position % blockSamples(ep->ctx) != 0))
        return;

    if (ep->stop || (ep->ctx->bufMode == BUF_LINEAR)) {
        out->draining = true;
        out->deadline = 0;
    } else {
        ep->position %= ep->ctx->bufSize;
        notifyBoundary(ep);
    }
}

/**
 * Play an application stream, repeating each sample to reach 48 kHz. Copies
 * a run up to each block boundary.
 *
 * @param out: output stream.
 * @param dst: first sample to write.
 * @param stride: distance between samples, in samples.
 * @param count: samples wanted.
 * @return the samples written, fewer than count once the stream is inactive
 * or draining.
 */
static size_t playStream(struct outputState *out, int16_t *dst, size_t stride,
                         size_t count)
{
    struct endpoint *ep = &out->endpoint;
    size_t done = 0;

    while ((done < count) && endpointActive(ep) && !out->draining) {
        const unsigned int divisor = ep->divisor;
        size_t left = blockRemaining(ep) * divisor - out->phase;
        size_t run = MIN(count - done, left);
        const stream_sample_t *src = &ep->ctx->buffer[ep->position];
        unsigned int phase = out->phase;
        for (size_t i = 0; i < run; i++) {
            dst[(done + i) * stride] = *src;
            if (++phase == divisor) {
                phase = 0;
                src++;
            }
        }

        outputAdvance(out, run);
        done += run;
    }
    return done;
}

/**
 * Fill an output channel of a playback frame. FM samples pass through
 * unchanged; TX levels are set by the codec gains in hwconfig.h. Unrouted,
 * empty, inactive or draining channels play silence.
 *
 * @param channel: SINK_SPK or SINK_RTX.
 * @param dst: first sample of the channel.
 * @param stride: distance between samples, in samples.
 * @param count: samples.
 */
static void playChannel(size_t channel, int16_t *dst, size_t stride,
                        size_t count)
{
    struct outputState *out = &outputs[channel];
    size_t done = 0;

    if (outputSource[channel] == SOURCE_MCU)
        done = playStream(out, dst, stride, count);
    else if (outputSource[channel] != NO_SOURCE)
        done = queueRead(&out->fmQueue, dst, stride, count);

    for (; done < count; done++)
        dst[done * stride] = 0;
}

/**
 * Check whether any output has samples left to send to the DSP.
 */
static bool outputsPending(void)
{
    for (size_t i = 0; i < CODEC_CHANNELS; i++) {
        const struct outputState *out = &outputs[i];
        if ((out->fmQueue.count != 0)
            || (endpointActive(&out->endpoint) && !out->draining))
            return true;
    }
    return false;
}

/**
 * Schedule the next playback frame. The DSP FIFO can accept several frames
 * at once: pacing writes keeps it from consuming application halves faster
 * than callers can refill them. Carry fractional milliseconds forward, and
 * don't catch up in a burst after a stall.
 *
 * @param frames: frames just written.
 */
static void pacePlayback(size_t frames)
{
    uint64_t duration = frames * (uint64_t)MSEC_PER_SEC + playbackFraction;
    int64_t period = duration / DSP_SAMPLE_RATE;
    playbackFraction = duration % DSP_SAMPLE_RATE;
    playbackDeadline += period;
    if (playbackDeadline + period <= k_uptime_get())
        playbackDeadline = k_uptime_get() + MAX(1, period);
}

/**
 * Write one stereo frame (speaker left, radio right) to the DSP, paced to
 * the 48 kHz rate.
 *
 * @param track: playback stream.
 */
static void playFrame(AudioTrack *track)
{
    if (!outputsPending() || !playbackRunning
        || (k_uptime_get() < playbackDeadline))
        return;

    AudioTrack_Buffer buffer = {
        .frameCount = track->mICStream->share->sampleCountPerFrame,
    };
    if (AudioTrack_obtainBuffer(track, &buffer, DSP_BUFFER_NO_WAIT) != 0)
        return;

    playChannel(SINK_SPK, &buffer.i16[SINK_SPK], CODEC_CHANNELS,
                buffer.frameCount);
    playChannel(SINK_RTX, &buffer.i16[SINK_RTX], CODEC_CHANNELS,
                buffer.frameCount);
    /* Always publish a complete frame, padding the final tail with silence. */
    AudioTrack_releaseBuffer(track, &buffer);
    pacePlayback(buffer.frameCount);
}

/**
 * Time at which everything written to the DSP so far will have played: the
 * samples still in its FIFO, its latency and a margin. Publishing the final
 * frame does not mean it reached the DAC.
 *
 * @param track: playback stream.
 */
static int64_t drainDeadline(AudioTrack *track)
{
    ICStream_Producer_fetchRemote(track->mICStream);
    const ICStreamFifo *fifo = &track->mICStream->share->fifo;
    size_t frames = (fifo->size - fifo_avail(fifo))
                  / (CODEC_CHANNELS * sizeof(int16_t));
    return k_uptime_get() + DIV_ROUND_UP(frames * MSEC_PER_SEC, DSP_SAMPLE_RATE)
         + track->mLatency + DRAIN_MARGIN_MS;
}

/**
 * Complete draining outputs once their last samples have played. This is an
 * estimate; draining does not read application memory.
 *
 * @param track: playback stream.
 */
static void finishOutputs(AudioTrack *track)
{
    for (size_t i = 0; i < CODEC_CHANNELS; i++) {
        struct outputState *out = &outputs[i];
        struct endpoint *ep = &out->endpoint;
        if (!endpointActive(ep) || !out->draining)
            continue;

        if (out->deadline == 0) {
            out->deadline = drainDeadline(track);
        } else if (k_uptime_get() >= out->deadline) {
            ep->ctx->running = 0;
            out->draining = false;
            notifyBoundary(ep);
        }
    }
}

/**
 * Codec worker: moves samples every millisecond while anything is captured
 * or played, until csk6codec_terminate().
 */
static void audioWorker(void *a, void *b, void *c)
{
    (void)a;
    (void)b;
    (void)c;

    while (!atomic_get(&stopRequested)) {
        k_mutex_lock(&audioMutex, K_FOREVER);
        applyRoutes(&record, &playbackTrack);
        if (captureRunning)
            captureFrame(&record);
        playFrame(&playbackTrack);
        finishOutputs(&playbackTrack);
        const bool idle = !captureRunning && !playbackRunning;
        k_mutex_unlock(&audioMutex);

        if (idle)
            k_sem_take(&workerWake, K_FOREVER);
        else
            k_sleep(K_MSEC(1));
    }

    k_mutex_lock(&audioMutex, K_FOREVER);
    for (size_t i = 0; i < CODEC_CHANNELS; i++) {
        detach(&inputs[i].endpoint);
        detach(&outputs[i].endpoint);
    }
    setCapture(&record, false);
    setPlayback(&playbackTrack, false);
    AudioRecord_dtor(&record);
    AudioTrack_dtor(&playbackTrack);
    running = false;
    k_condvar_broadcast(&changed);
    k_mutex_unlock(&audioMutex);
}

/**
 * Set up the endpoints, which live in PSRAM, before the worker starts.
 */
static void initEndpoints(void)
{
    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));
    for (size_t i = 0; i < CODEC_CHANNELS; i++) {
        inputs[i].endpoint.input = true;
        inputs[i].endpoint.channel = i;
        outputs[i].endpoint.channel = i;
    }
}

int csk6codec_init(void)
{
    initEndpoints();
    int ret = openDsp(&record, &playbackTrack);
    if (ret != 0)
        return ret;

    running = true;
    k_thread_create(&worker, workerStack, K_THREAD_STACK_SIZEOF(workerStack),
                    audioWorker, NULL, NULL, NULL, WORKER_PRIORITY, 0,
                    K_NO_WAIT);
    k_thread_name_set(&worker, "csk6_codec");
    return 0;
}

void csk6codec_terminate(void)
{
    /* The worker cancels the streams as it exits */
    atomic_set(&stopRequested, 1);
    k_sem_give(&workerWake);
    k_thread_join(&worker, K_FOREVER);
}

void csk6codec_setRoute(enum AudioSource source, enum AudioSink sink,
                        bool enabled)
{
    k_mutex_lock(&audioMutex, K_FOREVER);
    if (!running) {
        k_mutex_unlock(&audioMutex);
        return;
    }

    if ((sink == SINK_MCU) && (source != SOURCE_MCU)) {
        /* Capture into an application buffer */
        inputEnabled[source] = enabled;
        if (!enabled)
            detach(&inputs[source].endpoint);
    } else if (sink != SINK_MCU) {
        /* Playback from an application buffer, or an FM passthrough */
        outputSource[sink] = enabled ? (int)source : NO_SOURCE;
        queueReset(&outputs[sink].fmQueue);
        if (!enabled)
            detach(&outputs[sink].endpoint);
    }

    /* Return only after the worker has applied the device start/stop. */
    routesDirty = true;
    k_sem_give(&workerWake);
    while (routesDirty && running)
        k_condvar_wait(&changed, &audioMutex, K_FOREVER);
    k_mutex_unlock(&audioMutex);
}

/**
 * Check a stream's channel, buffer and sample rate. Supports 8, 16, 24 and
 * 48 kHz.
 */
static bool streamValid(uint8_t channel, const struct streamCtx *ctx)
{
    if ((channel >= CODEC_CHANNELS) || (ctx->buffer == NULL)
        || (ctx->bufSize == 0))
        return false;

    switch (ctx->bufMode) {
        case BUF_LINEAR:
            break;
        case BUF_CIRC_DOUBLE:
            if (ctx->bufSize % 2)
                return false;
            break;
        default:
            return false;
    }

    switch (ctx->sampleRate) {
        case 8000:
        case 16000:
        case 24000:
        case DSP_SAMPLE_RATE:
            return true;
        default:
            return false;
    }
}

/**
 * Attach a stream to an endpoint and start it at the beginning of its
 * buffer. Rearming a linear input keeps its sample-selection phase, so
 * consecutive reads stay evenly spaced.
 */
static void attach(struct endpoint *ep, struct streamCtx *ctx, bool rearm)
{
    ep->ctx = ctx;
    ep->error = 0;
    ep->stop = false;
    ep->position = 0;
    ep->boundaries = 0;
    ep->observed = 0;
    ep->divisor = DSP_SAMPLE_RATE / ctx->sampleRate;

    if (ep->input) {
        if (!rearm)
            inputs[ep->channel].phase = 0;
    } else {
        struct outputState *out = &outputs[ep->channel];
        out->phase = 0;
        out->draining = false;
        out->deadline = 0;
    }

    ctx->priv = ep;
    ctx->running = 1;
}

/**
 * audioDriver start: attach a stream to a channel.
 */
static int csk6codec_start(uint8_t channel, const void *config,
                           struct streamCtx *ctx)
{
    if (!streamValid(channel, ctx))
        return -EINVAL;

    const bool input = (enum Csk6CodecMode)(uintptr_t)config
                    == CSK6_CODEC_MODE_INPUT;
    struct endpoint *ep = input ? &inputs[channel].endpoint :
                                  &outputs[channel].endpoint;
    int ret = 0;

    k_mutex_lock(&audioMutex, K_FOREVER);
    const bool routed = input ? inputEnabled[channel] :
                                (outputSource[channel] == SOURCE_MCU);
    if (!running || !routed) {
        ret = -ENODEV;
    } else if (ep->busy || ((ep->ctx != NULL) && (ep->ctx != ctx))) {
        ret = -EBUSY;
    } else {
        bool rearm = (ep->ctx == ctx) && (ctx->bufMode == BUF_LINEAR);
        if (rearm && (ep->error != 0))
            ret = ep->error;
        else
            attach(ep, ctx, rearm);
    }
    k_mutex_unlock(&audioMutex);

    /* An attached output can start playback */
    if (ret == 0)
        k_sem_give(&workerWake);
    return ret;
}

/**
 * audioDriver data: the half (or buffer) the caller may use now.
 */
static int csk6codec_data(struct streamCtx *ctx, stream_sample_t **buffer)
{
    k_mutex_lock(&audioMutex, K_FOREVER);
    struct endpoint *ep = ctx->priv;
    int count = -EIO;
    *buffer = NULL;
    if ((ep != NULL) && (ep->ctx == ctx) && (ep->error == 0)) {
        count = blockSamples(ctx);
        *buffer = ctx->buffer;
        /* Initially the second half is idle. After each boundary the
         * completed half is available until the next boundary. */
        if ((ctx->bufMode == BUF_CIRC_DOUBLE) && (ep->position < (size_t)count))
            *buffer += count;
    }
    k_mutex_unlock(&audioMutex);
    return count;
}

/**
 * audioDriver sync: wait for the next buffer boundary. A timeout fails only
 * this call: the stream keeps running.
 */
static int csk6codec_sync(struct streamCtx *ctx, uint8_t dirty)
{
    (void)dirty; /* Samples are converted by the worker as it traverses them. */
    k_mutex_lock(&audioMutex, K_FOREVER);
    struct endpoint *ep = ctx->priv;
    if ((ep == NULL) || (ep->ctx != ctx) || (ep->error != 0) || ep->busy) {
        k_mutex_unlock(&audioMutex);
        return -EIO;
    }

    ep->busy = true;
    int ret = 0;
    int64_t deadline = ioDeadline(ctx);
    while ((ep->ctx == ctx) && ctx->running && (ep->error == 0)
           && (ep->boundaries == ep->observed)) {
        if (!waitForChange(deadline)) {
            ret = -ETIMEDOUT;
            break;
        }
    }

    if (ret == 0)
        ret = ep->ctx == ctx ? ep->error : -ECANCELED;
    if (ret == 0) {
        /*
         * More than one boundary means the caller was late: capture data
         * was overwritten or playback repeated a half. Carry on with the
         * current half, as the DMA drivers do: failing the stream would
         * leave the caller without a buffer.
         */
        if (ep->boundaries == ep->observed)
            ret = -EIO;
        ep->observed = ep->boundaries;
    }
    ep->busy = false;
    k_condvar_broadcast(&changed);
    k_mutex_unlock(&audioMutex);
    return ret;
}

/**
 * audioDriver terminate: detach the stream at once.
 */
static void csk6codec_halt(struct streamCtx *ctx)
{
    k_mutex_lock(&audioMutex, K_FOREVER);
    struct endpoint *ep = ctx->priv;
    if ((ep != NULL) && (ep->ctx == ctx))
        detach(ep);
    k_mutex_unlock(&audioMutex);
}

/**
 * Let an output stream finish before it is detached: RF output finishes its
 * current half, to keep the last TX frame, and linear output its buffer;
 * both then drain the DSP FIFO. Circular speaker streams stop at once, for
 * the M17 decoder's shutdown. Call with audioMutex held.
 */
static void finishOutputStream(struct endpoint *ep, struct streamCtx *ctx)
{
    /* If another callback is already waiting, the caller detaches at once
     * to cancel it. */
    if (ep->input || (ep->error != 0) || ep->busy)
        return;
    if ((ep->channel != SINK_RTX) && (ctx->bufMode != BUF_LINEAR))
        return;

    ep->busy = true;
    ep->stop = true;
    int64_t deadline = ioDeadline(ctx);
    while ((ep->ctx == ctx) && ctx->running && (ep->error == 0)) {
        if (!waitForChange(deadline))
            break;
    }
    ep->busy = false;
}

/**
 * audioDriver stop: let radio and linear output finish, then detach.
 */
static void csk6codec_stop(struct streamCtx *ctx)
{
    k_mutex_lock(&audioMutex, K_FOREVER);
    struct endpoint *ep = ctx->priv;
    if ((ep != NULL) && (ep->ctx == ctx)) {
        finishOutputStream(ep, ctx);
        if (ep->ctx == ctx)
            detach(ep);
    }
    k_condvar_broadcast(&changed);
    k_mutex_unlock(&audioMutex);
}

const struct audioDriver csk6_codec_audio_driver = {
    .start = csk6codec_start,
    .data = csk6codec_data,
    .sync = csk6codec_sync,
    .stop = csk6codec_stop,
    .terminate = csk6codec_halt,
};
