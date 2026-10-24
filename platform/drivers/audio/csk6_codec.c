/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
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
 */
LOG_MODULE_REGISTER(csk6_codec, LOG_LEVEL_INF);

#define DSP_SAMPLE_RATE 48000
#define CAPTURE_FRAME_SAMPLES (DSP_SAMPLE_RATE / 100)
#define RECORD_CHANNELS (CHANNEL_IN_LEFT | CHANNEL_IN_RIGHT)
/* Capacity per FM output, in 48 kHz samples; not a target fill level. */
#define QUEUE_SAMPLES (DSP_SAMPLE_RATE * 160 / 1000)
#define IO_TIMEOUT_MS 1000
#define NO_SOURCE (-1)

/* FM passthrough only: bridge separately available capture/playback frames. */
struct sample_queue {
    int16_t samples[QUEUE_SAMPLES];
    size_t head;
    size_t count;
};

/* The worker continuously traverses the attached application buffer, like
 * DMA. sync() observes half/full boundaries; it does not submit transfers. */
struct endpoint {
    struct streamCtx *ctx;
    size_t position;     /* Next application sample to read or write. */
    uint32_t boundaries; /* Completed halves, or linear completions. */
    uint32_t observed;   /* Last boundary acknowledged by sync(). */
    unsigned int divisor;
    unsigned int channel;
    int error;
    bool input;
    bool busy; /* A sync/stop callback is waiting on this endpoint. */
    bool stop; /* Finish the current output block, then drain. */
};

struct input_state {
    struct endpoint endpoint;
    unsigned int phase;
};

struct output_state {
    struct endpoint endpoint;
    struct sample_queue fm_queue;
    unsigned int phase;
    int64_t deadline;
    bool draining;
};

static struct input_state inputs[2] __attribute__((section(".psram_section")));
static struct output_state outputs[2]
    __attribute__((section(".psram_section")));
/* Protect routing, endpoint state and worker accesses to application buffers.
 * Callers access only the idle half; they must finish before it becomes active.
 * Condition waits release this mutex so the worker and cancellation can run. */
static K_MUTEX_DEFINE(audio_mutex);
static K_CONDVAR_DEFINE(changed);
static enum { OFF, STARTING, RUNNING, STOPPING } state;
static int init_result;
/* Desired routing, written by board callbacks and applied by the worker. */
static bool input_enabled[2];
static int output_source[2] = { NO_SOURCE, NO_SOURCE };
static bool routes_dirty;
/* Only the worker accesses vendor objects and actual device state. */
static AudioRecord record;
static AudioTrack playback_track;
static bool capture_running;
static bool playback_running;
static int64_t playback_deadline;
static uint32_t playback_fraction;
static struct k_thread worker;
K_THREAD_STACK_DEFINE(worker_stack, 8 * 1024);

/**
 * Empty an FM queue.
 */
static void queue_reset(struct sample_queue *q)
{
    q->head = q->count = 0;
}

/**
 * Take the oldest sample from a non-empty FM queue.
 */
static int16_t queue_pop(struct sample_queue *q)
{
    int16_t sample = q->samples[q->head];
    q->head = (q->head + 1) % QUEUE_SAMPLES;
    q->count--;
    return sample;
}

/**
 * Append a sample to an FM queue that has room for it.
 */
static void queue_push(struct sample_queue *q, int16_t sample)
{
    q->samples[(q->head + q->count) % QUEUE_SAMPLES] = sample;
    q->count++;
}

/**
 * Fail a stream: wake a waiting caller and stop further buffer transfers.
 */
static void fail_stream(struct endpoint *ep, int error)
{
    ep->error = error;
    if (ep->ctx != NULL)
        ep->ctx->running = 0;
    k_condvar_broadcast(&changed);
}

/**
 * Cancel and detach a stream. Call with audio_mutex held, so no worker
 * access can outlive the detachment.
 */
static void detach(struct endpoint *ep)
{
    fail_stream(ep, -ECANCELED);
    ep->ctx = NULL;
}

/**
 * Count a completed buffer half (or linear buffer) and wake waiting callers.
 */
static void notify_boundary(struct endpoint *ep)
{
    ep->boundaries++;
    k_condvar_broadcast(&changed);
}

/**
 * Check whether an output channel has something to play.
 */
static bool output_enabled(unsigned int channel)
{
    if (output_source[channel] == SOURCE_MCU)
        return outputs[channel].endpoint.ctx != NULL
            && outputs[channel].endpoint.error == 0;
    return output_source[channel] != NO_SOURCE;
}

/**
 * Reload a stream's shared layout, which the DSP can change while it
 * constructs the stream: drop the cached copy, then rebuild the MCU side.
 */
static struct ICStreamShare *refresh_stream_layout(ICStream *stream)
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
 * Bypass ADC HPF2 after each capture start, to keep the received baseband.
 * Venus HAL: ADC01 is the AON codec at 0x46e20000, not CP ADC23. R6 bit 14
 * controls HPF2 for both channels; bit 15 is the separate HPF1. Apply after
 * the synchronous DSP start, which may configure the codec.
 */
static void disable_adc_hpf2(void)
{
    const uintptr_t adc = AON_VAD_BASE + 0x20000;
    uint32_t r5 = sys_read32(adc + 0x14);
    uint32_t before = sys_read32(adc + 0x18);
    /* Only touch an enabled ADC that has been released from reset. */
    if ((r5 & (BIT(7) | BIT(8))) != (BIT(7) | BIT(8))) {
        LOG_ERR("ADC01 HPF2 bypass skipped: inactive R5=%08x R6=%08x", r5,
                before);
        return;
    }
    sys_write32(before & ~BIT(14), adc + 0x18);
    __DSB();
    uint32_t after = sys_read32(adc + 0x18);
    if (after != (before & ~BIT(14)))
        LOG_ERR("ADC01 HPF2 bypass readback mismatch");
}

/**
 * Send a key=value parameter string to the DSP.
 *
 * @param kv: parameters, separated by ';'.
 * @return 0 on success.
 */
static int set_dsp_param(const char *kv)
{
    String8 param;
    String8_ctor_char(&param, kv);
    int ret = AudioSystem_setParameters(0, &param);
    String8_dtor(&param);
    return ret;
}

/**
 * Set the mic (left) and BK4819 RX audio (right) ADC gains from hwconfig.h.
 *
 * @return 0 on success.
 */
static int configure_adc_gain(void)
{
    char kv[128];
    snprintf(kv, sizeof(kv),
             "ADC_PDM_GAIN_A_LEFT=%d;ADC_PDM_GAIN_D_LEFT=%d;"
             "ADC_PDM_GAIN_A_RIGHT=%d;ADC_PDM_GAIN_D_RIGHT=%d",
             C62_ADC_MIC_GAIN_A, C62_ADC_MIC_GAIN_D, C62_ADC_RX_GAIN_A,
             C62_ADC_RX_GAIN_D);
    return set_dsp_param(kv);
}

/**
 * Set the speaker (left) and BK4819 TX (right) DAC gains from hwconfig.h.
 *
 * @return 0 on success.
 */
static int configure_dac_gain(void)
{
    char kv[128];
    snprintf(kv, sizeof(kv),
             "DAC_GAIN_A_LEFT=%d;DAC_GAIN_D_LEFT=%d;"
             "DAC_GAIN_A_RIGHT=%d;DAC_GAIN_D_RIGHT=%d",
             C62_DAC_SPK_GAIN_A, C62_DAC_SPK_GAIN_D, C62_DAC_TX_GAIN_A,
             C62_DAC_TX_GAIN_D);
    return set_dsp_param(kv);
}

/**
 * Set the codec gains and open the 48 kHz capture and playback streams.
 * Worker only; on failure, closes whatever it opened.
 *
 * @return 0 on success.
 */
static int open_dsp(void)
{
    const char *stage = "ADC gain";
    int ret = configure_adc_gain();
    if (ret != 0)
        goto fail;
    String8 param;
    /* Set the fixed hardware rate and capture channels before opening streams. */
    stage = "48k ADC/DAC configuration";
    char config[64];
    snprintf(config, sizeof(config), "samplingRate=%u;channels=%u",
             (unsigned int)DSP_SAMPLE_RATE, (unsigned int)RECORD_CHANNELS);
    String8_ctor_char(&param, config);
    ret = AudioSystem_setParameters(0, &param);
    String8_dtor(&param);
    if (ret != 0)
        goto fail;
    stage = "48k recording constructor";
    ret = AudioRecord_ctor(&record, 0, DSP_SAMPLE_RATE, PCM_16_BIT,
                           RECORD_CHANNELS, CAPTURE_FRAME_SAMPLES, NULL);
    if (ret != 0)
        goto fail;

    /* Refresh the input layout changed by SDK openRecord. */
    stage = "recording buffer layout";
    struct ICStreamShare *share = refresh_stream_layout(record.mICStream);
    if (share->sampleCountPerFrame != record.mCblk->frameCount
        || share->channelCount != record.mCblk->channels
        || share->sampleCountPerFrame <= 0 || share->channelCount < 2) {
        LOG_ERR("DSP input stream configuration mismatch");
        ret = -EINVAL;
        goto fail_record;
    }
    stage = "48k stereo playback constructor";
    ret = AudioTrack_ctor(&playback_track, DSP_SAMPLE_RATE, PCM_16_BIT,
                          CHANNEL_OUT_STEREO, 0, NULL);
    if (ret != 0)
        goto fail_record;
    stage = "stereo playback buffer layout";
    struct ICStreamShare *out = refresh_stream_layout(playback_track.mICStream);
    if (playback_track.mCblk->channels != 2
        || playback_track.mCblk->frameSize != 2 * sizeof(int16_t)
        || out->channelCount != 2 || out->cellSize != sizeof(int16_t)
        || out->sampleCountPerFrame <= 0) {
        printk("C62 DSP output: channels=%d frameSize=%d shared channels=%d "
               "cellSize=%d\n",
               playback_track.mCblk->channels, playback_track.mCblk->frameSize,
               out->channelCount, out->cellSize);
        ret = -ENOTSUP;
        goto fail_playback;
    }
    stage = "DAC gain";
    ret = configure_dac_gain();
    if (ret != 0)
        goto fail_playback;
    return 0;
fail_playback:
    AudioTrack_dtor(&playback_track);
fail_record:
    AudioRecord_dtor(&record);
fail:
    LOG_ERR("Audio initialization failed at %s: %d", stage, ret);
    return ret;
}

/**
 * Start or stop capture and playback to match the routes and attached
 * streams. Worker only.
 */
static void apply_routes(void)
{
    bool capture = input_enabled[0] || input_enabled[1];
    bool playback = false;
    for (unsigned int i = 0; i < 2; i++) {
        capture |= output_source[i] == SOURCE_MIC
                || output_source[i] == SOURCE_RTX;
        playback |= output_enabled(i);
    }
    if (capture != capture_running) {
        if (capture) {
            AudioRecord_start(&record);
            disable_adc_hpf2();
        } else
            AudioRecord_stop(&record);
        capture_running = capture;
    }
    if (playback != playback_running) {
        if (playback) {
            AudioTrack_start(&playback_track);
            playback_deadline = k_uptime_get();
            playback_fraction = 0;
        } else {
            AudioTrack_stop(&playback_track);
            AudioTrack_flush(&playback_track);
        }
        playback_running = playback;
    }
    if (routes_dirty) {
        routes_dirty = false;
        k_condvar_broadcast(&changed);
    }
}

/**
 * Take one capture frame, if any, and hand its samples to the attached
 * input streams and FM queues. AudioRecord_read() can block indefinitely and
 * copies whole frames, so this uses the SDK's stream, channel mapping and
 * cache API directly.
 */
static void capture_frame(void)
{
    ICStream *ic = record.mICStream;
    ICStream_Consumer_fetchRemote(ic);
    if (ICStream_Consumer_isEmpty(ic))
        return;
    void *frame;
    int ret = ICStream_Consumer_acquireFrame(ic, &frame);
    if (ret != IC_OK)
        goto fail;
    const int16_t *samples = frame;
    for (size_t n = 0; n < record.mCblk->frameCount; n++) {
        for (unsigned int source = 0; source < 2; source++) {
            int16_t sample = samples[n * ic->share->channelCount
                                     + record.mChannelOutIdx[source]];
            struct input_state *in = &inputs[source];
            struct endpoint *ep = &in->endpoint;
            /* Select every divisor-th sample directly into the caller's
             * buffer; phase persists across DSP frame boundaries. */
            if (ep->ctx != NULL && ep->ctx->running) {
                if (in->phase == 0) {
                    ep->ctx->buffer[ep->position++] = sample;
                    size_t size = ep->ctx->bufSize;
                    if (ep->position == size) {
                        ep->position = 0;
                        if (ep->ctx->bufMode == BUF_LINEAR)
                            ep->ctx->running = 0;
                        notify_boundary(ep);
                    } else if (ep->ctx->bufMode == BUF_CIRC_DOUBLE
                               && ep->position == size / 2)
                        notify_boundary(ep);
                }
                in->phase = (in->phase + 1) % ep->divisor;
            }
            for (unsigned int sink = 0; sink < 2; sink++) {
                if (output_source[sink] != (int)source)
                    continue;
                struct sample_queue *q = &outputs[sink].fm_queue;
                /* On overflow discard the oldest FM sample. */
                if (q->count == QUEUE_SAMPLES)
                    queue_pop(q);
                queue_push(q, sample);
            }
        }
    }
    ret = ICStream_Consumer_releaseFrame(ic, frame);
    if (ret == IC_OK)
        ret = ICStream_Consumer_commitRemote(ic);
    if (ret == IC_OK)
        return;
fail:
    for (unsigned int i = 0; i < 2; i++) {
        if (inputs[i].endpoint.ctx != NULL)
            fail_stream(&inputs[i].endpoint, -EIO);
    }
}

/**
 * Next 48 kHz sample for an output channel. FM samples pass through
 * unchanged; application samples are repeated to reach 48 kHz. TX levels are
 * set by the codec gains in hwconfig.h. Inactive, empty or draining channels
 * play silence.
 *
 * @param channel: SINK_SPK or SINK_RTX.
 * @return the sample.
 */
static int16_t output_sample(unsigned int channel)
{
    struct output_state *out = &outputs[channel];
    struct endpoint *ep = &out->endpoint;
    if (output_source[channel] == NO_SOURCE)
        return 0;
    if (output_source[channel] != SOURCE_MCU)
        return out->fm_queue.count ? queue_pop(&out->fm_queue) : 0;
    if (ep->ctx == NULL || !ep->ctx->running || out->draining)
        return 0;
    int16_t sample = ep->ctx->buffer[ep->position];
    if (++out->phase == ep->divisor) {
        out->phase = 0;
        ep->position++;
        size_t block = ep->ctx->bufSize
                     / (ep->ctx->bufMode == BUF_CIRC_DOUBLE ? 2 : 1);
        if (ep->position % block == 0) {
            if (ep->stop || ep->ctx->bufMode == BUF_LINEAR) {
                out->draining = true;
                out->deadline = 0;
            } else {
                ep->position %= ep->ctx->bufSize;
                notify_boundary(ep);
            }
        }
    }
    return sample;
}

/**
 * Write one stereo frame (speaker left, radio right) to the DSP, paced to
 * the 48 kHz rate.
 */
static void play_frame(void)
{
    bool pending = false;
    for (unsigned int i = 0; i < 2; i++) {
        struct endpoint *ep = &outputs[i].endpoint;
        pending |= outputs[i].fm_queue.count
                || (ep->ctx != NULL && ep->ctx->running
                    && !outputs[i].draining);
    }
    if (!pending || !playback_running || k_uptime_get() < playback_deadline)
        return;
    AudioTrack_Buffer buffer = {
        .frameCount = playback_track.mICStream->share->sampleCountPerFrame
    };
    if (AudioTrack_obtainBuffer(&playback_track, &buffer, 0) != 0)
        return;
    for (size_t n = 0; n < buffer.frameCount; n++) {
        buffer.i16[2 * n] = output_sample(SINK_SPK);
        buffer.i16[2 * n + 1] = output_sample(SINK_RTX);
    }
    /* Always publish a complete frame, padding the final tail with silence. */
    AudioTrack_releaseBuffer(&playback_track, &buffer);
    /* The DSP FIFO can accept several frames at once. Pace writes so it cannot
     * consume application halves faster than callers can refill them. Carry
     * fractional milliseconds forward and avoid catch-up bursts after stalls. */
    uint64_t duration = buffer.frameCount * 1000ULL + playback_fraction;
    int64_t period = duration / DSP_SAMPLE_RATE;
    playback_fraction = duration % DSP_SAMPLE_RATE;
    playback_deadline += period;
    if (playback_deadline + period <= k_uptime_get())
        playback_deadline = k_uptime_get() + MAX(1, period);
}

/**
 * Complete draining outputs once their last samples have played. Publishing
 * the final frame does not mean it reached the DAC, so wait for the queued
 * samples, the DSP latency and a 20 ms margin. This is an estimate; draining
 * does not read application memory.
 */
static void finish_outputs(void)
{
    for (unsigned int i = 0; i < 2; i++) {
        struct output_state *out = &outputs[i];
        struct endpoint *ep = &out->endpoint;
        if (ep->ctx == NULL || !ep->ctx->running || !out->draining)
            continue;
        if (out->deadline == 0) {
            ICStream_Producer_fetchRemote(playback_track.mICStream);
            const ICStreamFifo *fifo = &playback_track.mICStream->share->fifo;
            size_t frames = (fifo->size - fifo_avail(fifo))
                          / (2 * sizeof(int16_t));
            out->deadline = k_uptime_get()
                          + DIV_ROUND_UP(frames * 1000, DSP_SAMPLE_RATE)
                          + playback_track.mLatency + 20;
        } else if (k_uptime_get() >= out->deadline) {
            ep->ctx->running = 0;
            out->draining = false;
            notify_boundary(ep);
        }
    }
}

/**
 * Codec worker: opens the DSP streams, then moves samples every millisecond
 * until csk6codec_terminate().
 */
static void audio_worker(void *a, void *b, void *c)
{
    (void)a;
    (void)b;
    (void)c;
    k_mutex_lock(&audio_mutex, K_FOREVER);
    init_result = open_dsp();
    state = init_result == 0 ? RUNNING : OFF;
    k_condvar_broadcast(&changed);
    k_mutex_unlock(&audio_mutex);
    if (init_result != 0)
        return;
    for (;;) {
        k_mutex_lock(&audio_mutex, K_FOREVER);
        if (state == STOPPING)
            break;
        apply_routes();
        if (capture_running)
            capture_frame();
        play_frame();
        finish_outputs();
        k_mutex_unlock(&audio_mutex);
        k_sleep(K_MSEC(1));
    }
    if (capture_running)
        AudioRecord_stop(&record);
    if (playback_running) {
        AudioTrack_stop(&playback_track);
        AudioTrack_flush(&playback_track);
    }
    AudioRecord_dtor(&record);
    AudioTrack_dtor(&playback_track);
    state = OFF;
    k_condvar_broadcast(&changed);
    k_mutex_unlock(&audio_mutex);
}

/* The board serializes init/terminate. Wait for the worker to construct the
 * DSP streams so initialization failures are reported to the board caller. */
int csk6codec_init(void)
{
    k_mutex_lock(&audio_mutex, K_FOREVER);
    if (state != OFF) {
        int ret = state == RUNNING ? 0 : -EBUSY;
        k_mutex_unlock(&audio_mutex);
        return ret;
    }
    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));
    memset(input_enabled, 0, sizeof(input_enabled));
    for (unsigned int i = 0; i < 2; i++) {
        inputs[i].endpoint.input = true;
        inputs[i].endpoint.channel = outputs[i].endpoint.channel = i;
        output_source[i] = NO_SOURCE;
    }
    capture_running = playback_running = routes_dirty = false;
    state = STARTING;
    k_thread_create(&worker, worker_stack, K_THREAD_STACK_SIZEOF(worker_stack),
                    audio_worker, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
    k_thread_name_set(&worker, "csk6_codec");
    while (state == STARTING)
        k_condvar_wait(&changed, &audio_mutex, K_FOREVER);
    int ret = init_result;
    k_mutex_unlock(&audio_mutex);
    if (ret != 0)
        k_thread_join(&worker, K_FOREVER);
    return ret;
}

void csk6codec_terminate(void)
{
    k_mutex_lock(&audio_mutex, K_FOREVER);
    if (state != RUNNING) {
        k_mutex_unlock(&audio_mutex);
        return;
    }
    state = STOPPING;
    for (unsigned int i = 0; i < 2; i++) {
        detach(&inputs[i].endpoint);
        detach(&outputs[i].endpoint);
    }
    k_mutex_unlock(&audio_mutex);
    k_thread_join(&worker, K_FOREVER);
    k_mutex_lock(&audio_mutex, K_FOREVER);
    /* Let cancelled callbacks return before a later init resets their state. */
    while (inputs[0].endpoint.busy || inputs[1].endpoint.busy
           || outputs[0].endpoint.busy || outputs[1].endpoint.busy)
        k_condvar_wait(&changed, &audio_mutex, K_FOREVER);
    k_mutex_unlock(&audio_mutex);
}

void csk6codec_setRoute(enum AudioSource source, enum AudioSink sink,
                        bool enabled)
{
    if (source > SOURCE_MCU || sink > SINK_MCU)
        return;
    k_mutex_lock(&audio_mutex, K_FOREVER);
    if (state == RUNNING) {
        if (sink == SINK_MCU && source != SOURCE_MCU) {
            input_enabled[source] = enabled;
            if (!enabled) {
                detach(&inputs[source].endpoint);
            }
        } else if (sink != SINK_MCU) {
            output_source[sink] = enabled ? (int)source : NO_SOURCE;
            queue_reset(&outputs[sink].fm_queue);
            if (!enabled)
                detach(&outputs[sink].endpoint);
        }
        /* Return only after the worker has applied the device start/stop. */
        routes_dirty = true;
        while (routes_dirty && state == RUNNING)
            k_condvar_wait(&changed, &audio_mutex, K_FOREVER);
    }
    k_mutex_unlock(&audio_mutex);
}

/**
 * audioDriver start: attach a stream to a channel. Supports 8, 16, 24 and
 * 48 kHz.
 */
static int stream_start(uint8_t channel, const void *config,
                        struct streamCtx *ctx)
{
    if (channel >= 2 || ctx->buffer == NULL || ctx->bufSize == 0
        || (ctx->bufMode != BUF_LINEAR && ctx->bufMode != BUF_CIRC_DOUBLE)
        || (ctx->bufMode == BUF_CIRC_DOUBLE && ctx->bufSize % 2)
        || (ctx->sampleRate != 8000 && ctx->sampleRate != 16000
            && ctx->sampleRate != 24000 && ctx->sampleRate != DSP_SAMPLE_RATE))
        return -EINVAL;
    bool input = *(const bool *)config;
    struct endpoint *ep = input ? &inputs[channel].endpoint :
                                  &outputs[channel].endpoint;
    k_mutex_lock(&audio_mutex, K_FOREVER);
    int ret = 0;
    if (state != RUNNING
        || !(input ? input_enabled[channel] :
                     output_source[channel] == SOURCE_MCU))
        ret = -ENODEV;
    else if (ep->busy || (ep->ctx != NULL && ep->ctx != ctx))
        ret = -EBUSY;
    else {
        /* Preserve input sample-selection phase when rearming a linear read. */
        bool rearm = ep->ctx == ctx && ctx->bufMode == BUF_LINEAR;
        if (rearm && ep->error) {
            ret = ep->error;
        } else {
            ep->ctx = ctx;
            ep->error = 0;
            ep->stop = false;
            ep->position = 0;
            ep->boundaries = ep->observed = 0;
            ep->divisor = DSP_SAMPLE_RATE / ctx->sampleRate;
            if (input) {
                if (!rearm)
                    inputs[channel].phase = 0;
            } else {
                struct output_state *out = &outputs[channel];
                out->phase = 0;
                out->draining = false;
                out->deadline = 0;
            }
            ctx->priv = ep;
            ctx->running = 1;
        }
    }
    k_mutex_unlock(&audio_mutex);
    return ret;
}

/**
 * audioDriver data: the half (or buffer) the caller may use now.
 */
static int stream_data(struct streamCtx *ctx, stream_sample_t **buffer)
{
    k_mutex_lock(&audio_mutex, K_FOREVER);
    struct endpoint *ep = ctx->priv;
    int count = -EIO;
    *buffer = NULL;
    if (ep != NULL && ep->ctx == ctx && ep->error == 0) {
        count = ctx->bufSize;
        *buffer = ctx->buffer;
        if (ctx->bufMode == BUF_CIRC_DOUBLE) {
            count /= 2;
            /* Initially the second half is idle. After each boundary the
             * completed half is available until the next boundary. */
            if (ep->position < (size_t)count)
                *buffer += count;
        }
    }
    k_mutex_unlock(&audio_mutex);
    return count;
}

/**
 * audioDriver sync: wait for the next buffer boundary.
 */
static int stream_sync(struct streamCtx *ctx, uint8_t dirty)
{
    (void)dirty; /* Samples are converted by the worker as it traverses them. */
    k_mutex_lock(&audio_mutex, K_FOREVER);
    struct endpoint *ep = ctx->priv;
    if (ep == NULL || ep->ctx != ctx || ep->error || ep->busy) {
        k_mutex_unlock(&audio_mutex);
        return -EIO;
    }
    ep->busy = true;
    /* Allow one buffer period plus a bounded margin for a stalled device. */
    int64_t deadline = k_uptime_get() + IO_TIMEOUT_MS
                     + (uint64_t)ctx->bufSize * 1000 / ctx->sampleRate;
    while (ep->ctx == ctx && ctx->running && ep->error == 0
           && ep->boundaries == ep->observed) {
        int64_t remaining = deadline - k_uptime_get();
        if (remaining <= 0) {
            fail_stream(ep, -ETIMEDOUT);
            break;
        }
        k_condvar_wait(&changed, &audio_mutex, K_MSEC(remaining));
    }
    int ret = ep->ctx == ctx ? ep->error : -ECANCELED;
    if (ret == 0) {
        uint32_t elapsed = ep->boundaries - ep->observed;
        /* More than one boundary means capture data was overwritten or
         * playback reused a half before the caller could refill it. */
        if (elapsed > 1) {
            fail_stream(ep, -EOVERFLOW);
            ret = -EOVERFLOW;
        } else if (elapsed == 0)
            ret = -EIO;
        ep->observed = ep->boundaries;
    }
    ep->busy = false;
    k_condvar_broadcast(&changed);
    k_mutex_unlock(&audio_mutex);
    return ret;
}

/**
 * audioDriver terminate: detach the stream at once.
 */
static void stream_terminate(struct streamCtx *ctx)
{
    k_mutex_lock(&audio_mutex, K_FOREVER);
    struct endpoint *ep = ctx->priv;
    if (ep != NULL && ep->ctx == ctx)
        detach(ep);
    k_mutex_unlock(&audio_mutex);
}

/**
 * audioDriver stop: let radio and linear output finish, then detach.
 */
static void stream_stop(struct streamCtx *ctx)
{
    k_mutex_lock(&audio_mutex, K_FOREVER);
    struct endpoint *ep = ctx->priv;
    if (ep != NULL && ep->ctx == ctx) {
        /* Circular speaker streams stop immediately for M17 decoder shutdown.
         * RF output finishes its current half to preserve the last TX frame;
         * linear output finishes its buffer. Both then drain the DSP FIFO.
         * If another callback is already waiting, detach to cancel it. */
        if (!ep->input && ep->error == 0 && !ep->busy
            && (ep->channel == SINK_RTX || ctx->bufMode == BUF_LINEAR)) {
            ep->busy = true;
            ep->stop = true;
            int64_t deadline = k_uptime_get() + IO_TIMEOUT_MS
                             + (uint64_t)ctx->bufSize * 1000 / ctx->sampleRate;
            while (ep->ctx == ctx && ctx->running && ep->error == 0) {
                int64_t remaining = deadline - k_uptime_get();
                if (remaining <= 0) {
                    fail_stream(ep, -ETIMEDOUT);
                    break;
                }
                k_condvar_wait(&changed, &audio_mutex, K_MSEC(remaining));
            }
            ep->busy = false;
        }
        if (ep->ctx == ctx)
            detach(ep);
    }
    k_condvar_broadcast(&changed);
    k_mutex_unlock(&audio_mutex);
}

const struct audioDriver csk6_codec_audio_driver = {
    .start = stream_start,
    .data = stream_data,
    .sync = stream_sync,
    .stop = stream_stop,
    .terminate = stream_terminate,
};
