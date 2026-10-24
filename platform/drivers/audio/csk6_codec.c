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
 */
LOG_MODULE_REGISTER(csk6_codec, LOG_LEVEL_INF);

#define DSP_SAMPLE_RATE 48000
#define CAPTURE_FRAME_SAMPLES (DSP_SAMPLE_RATE / 100)
#define RECORD_CHANNELS (CHANNEL_IN_LEFT | CHANNEL_IN_RIGHT)
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
#define WORKER_STACK_SIZE (8 * 1024)

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
static enum {
    CODEC_OFF,
    CODEC_STARTING,
    CODEC_RUNNING,
    CODEC_STOPPING,
} state;
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
K_THREAD_STACK_DEFINE(worker_stack, WORKER_STACK_SIZE);

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
 * Append a sample to an FM queue, discarding the oldest sample when full.
 */
static void queue_push(struct sample_queue *q, int16_t sample)
{
    if (q->count == QUEUE_SAMPLES)
        queue_pop(q);
    q->samples[(q->head + q->count) % QUEUE_SAMPLES] = sample;
    q->count++;
}

/**
 * Samples between two boundary notifications: half the buffer for circular
 * streams, the whole buffer for linear ones.
 */
static size_t block_samples(const struct streamCtx *ctx)
{
    if (ctx->bufMode == BUF_CIRC_DOUBLE)
        return ctx->bufSize / 2;
    return ctx->bufSize;
}

/**
 * Check whether an endpoint has an attached stream that is still running.
 */
static bool endpoint_active(const struct endpoint *ep)
{
    return ep->ctx != NULL && ep->ctx->running;
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
 * Deadline for a caller waiting on a stream: one buffer period, plus an
 * allowance for a stalled device.
 */
static int64_t io_deadline(const struct streamCtx *ctx)
{
    return k_uptime_get() + IO_TIMEOUT_MS
         + (uint64_t)ctx->bufSize * MSEC_PER_SEC / ctx->sampleRate;
}

/**
 * Wait, with audio_mutex held, until the worker or a canceller signals a
 * change. Past the deadline, fail the stream instead.
 *
 * @param ep: endpoint the caller is waiting on.
 * @param deadline: from io_deadline().
 * @return false if the deadline passed.
 */
static bool wait_for_change(struct endpoint *ep, int64_t deadline)
{
    int64_t remaining = deadline - k_uptime_get();
    if (remaining <= 0) {
        fail_stream(ep, -ETIMEDOUT);
        return false;
    }
    k_condvar_wait(&changed, &audio_mutex, K_MSEC(remaining));
    return true;
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
 * Set the fixed 48 kHz hardware rate and the capture channels, before the
 * streams are opened.
 *
 * @return 0 on success.
 */
static int configure_sample_format(void)
{
    char kv[64];
    snprintf(kv, sizeof(kv), "samplingRate=%u;channels=%u",
             (unsigned int)DSP_SAMPLE_RATE, (unsigned int)RECORD_CHANNELS);
    return set_dsp_param(kv);
}

/**
 * Check the capture stream layout, which the SDK's openRecord can change,
 * against what capture_frame() expects.
 *
 * @return 0 if it matches.
 */
static int check_record_layout(void)
{
    struct ICStreamShare *share = refresh_stream_layout(record.mICStream);
    if (share->sampleCountPerFrame != record.mCblk->frameCount
        || share->channelCount != record.mCblk->channels
        || share->sampleCountPerFrame <= 0 || share->channelCount < 2) {
        LOG_ERR("DSP capture layout mismatch: frames=%d/%d channels=%d/%d",
                (int)share->sampleCountPerFrame, (int)record.mCblk->frameCount,
                (int)share->channelCount, (int)record.mCblk->channels);
        return -EINVAL;
    }
    return 0;
}

/**
 * Check that the playback stream is interleaved 16-bit stereo, as
 * play_frame() writes it.
 *
 * @return 0 if it is.
 */
static int check_playback_layout(void)
{
    struct ICStreamShare *share =
        refresh_stream_layout(playback_track.mICStream);
    if (playback_track.mCblk->channels != 2
        || playback_track.mCblk->frameSize != 2 * sizeof(int16_t)
        || share->channelCount != 2 || share->cellSize != sizeof(int16_t)
        || share->sampleCountPerFrame <= 0) {
        LOG_ERR("DSP playback layout mismatch: channels=%d frameSize=%d "
                "shared channels=%d cellSize=%d",
                playback_track.mCblk->channels, playback_track.mCblk->frameSize,
                share->channelCount, share->cellSize);
        return -ENOTSUP;
    }
    return 0;
}

/**
 * Set the codec gains and open the 48 kHz capture and playback streams.
 * Worker only; on failure, closes whatever it opened.
 *
 * @return 0 on success.
 */
static int open_dsp(void)
{
    int ret = configure_adc_gain();
    if (ret != 0) {
        LOG_ERR("Setting the ADC gains failed: %d", ret);
        return ret;
    }

    ret = configure_sample_format();
    if (ret != 0) {
        LOG_ERR("Setting the 48 kHz sample format failed: %d", ret);
        return ret;
    }

    ret = AudioRecord_ctor(&record, 0, DSP_SAMPLE_RATE, PCM_16_BIT,
                           RECORD_CHANNELS, CAPTURE_FRAME_SAMPLES, NULL);
    if (ret != 0) {
        LOG_ERR("Opening the 48 kHz capture stream failed: %d", ret);
        return ret;
    }

    ret = check_record_layout();
    if (ret != 0)
        goto close_record;

    ret = AudioTrack_ctor(&playback_track, DSP_SAMPLE_RATE, PCM_16_BIT,
                          CHANNEL_OUT_STEREO, 0, NULL);
    if (ret != 0) {
        LOG_ERR("Opening the 48 kHz playback stream failed: %d", ret);
        goto close_record;
    }

    ret = check_playback_layout();
    if (ret != 0)
        goto close_playback;

    ret = configure_dac_gain();
    if (ret != 0) {
        LOG_ERR("Setting the DAC gains failed: %d", ret);
        goto close_playback;
    }

    return 0;

close_playback:
    AudioTrack_dtor(&playback_track);
close_record:
    AudioRecord_dtor(&record);
    return ret;
}

/**
 * Start or stop the DSP capture stream. Worker only.
 */
static void set_capture(bool enable)
{
    if (enable == capture_running)
        return;

    if (enable) {
        AudioRecord_start(&record);
        disable_adc_hpf2();
    } else {
        AudioRecord_stop(&record);
    }
    capture_running = enable;
}

/**
 * Start or stop the DSP playback stream. Worker only.
 */
static void set_playback(bool enable)
{
    if (enable == playback_running)
        return;

    if (enable) {
        AudioTrack_start(&playback_track);
        playback_deadline = k_uptime_get();
        playback_fraction = 0;
    } else {
        AudioTrack_stop(&playback_track);
        AudioTrack_flush(&playback_track);
    }
    playback_running = enable;
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

    set_capture(capture);
    set_playback(playback);

    if (routes_dirty) {
        routes_dirty = false;
        k_condvar_broadcast(&changed);
    }
}

/**
 * Move an input endpoint past the sample just written. At each block
 * boundary notify the caller; at the end of the buffer wrap around, or stop
 * a linear stream.
 */
static void input_advance(struct endpoint *ep)
{
    struct streamCtx *ctx = ep->ctx;

    ep->position++;
    if (ep->position % block_samples(ctx) != 0)
        return;

    if (ep->position == ctx->bufSize) {
        ep->position = 0;
        if (ctx->bufMode == BUF_LINEAR)
            ctx->running = 0;
    }
    notify_boundary(ep);
}

/**
 * Hand one 48 kHz sample to an input stream, keeping every divisor-th
 * sample. The phase persists across DSP frames.
 */
static void capture_to_stream(struct input_state *in, int16_t sample)
{
    struct endpoint *ep = &in->endpoint;
    if (!endpoint_active(ep))
        return;

    if (in->phase == 0) {
        ep->ctx->buffer[ep->position] = sample;
        input_advance(ep);
    }
    in->phase = (in->phase + 1) % ep->divisor;
}

/**
 * Hand one 48 kHz sample to the FM queue of each output routed from this
 * source.
 */
static void capture_to_fm_queues(unsigned int source, int16_t sample)
{
    for (unsigned int sink = 0; sink < 2; sink++) {
        if (output_source[sink] == (int)source)
            queue_push(&outputs[sink].fm_queue, sample);
    }
}

/**
 * Fail every attached input stream.
 */
static void fail_inputs(int error)
{
    for (unsigned int i = 0; i < 2; i++) {
        if (inputs[i].endpoint.ctx != NULL)
            fail_stream(&inputs[i].endpoint, error);
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
    if (ICStream_Consumer_acquireFrame(ic, &frame) != IC_OK) {
        fail_inputs(-EIO);
        return;
    }

    const int16_t *samples = frame;
    const unsigned int channels = ic->share->channelCount;
    for (size_t n = 0; n < record.mCblk->frameCount; n++) {
        for (unsigned int source = 0; source < 2; source++) {
            int16_t sample =
                samples[n * channels + record.mChannelOutIdx[source]];
            capture_to_stream(&inputs[source], sample);
            capture_to_fm_queues(source, sample);
        }
    }

    int ret = ICStream_Consumer_releaseFrame(ic, frame);
    if (ret == IC_OK)
        ret = ICStream_Consumer_commitRemote(ic);
    if (ret != IC_OK)
        fail_inputs(-EIO);
}

/**
 * Move an output endpoint past the sample just read, after it has been
 * repeated divisor times. At each block boundary either notify the caller
 * and wrap around, or, for the last block of a stopping or linear stream,
 * start draining.
 */
static void output_advance(struct output_state *out)
{
    struct endpoint *ep = &out->endpoint;

    if (++out->phase < ep->divisor)
        return;
    out->phase = 0;

    ep->position++;
    if (ep->position % block_samples(ep->ctx) != 0)
        return;

    if (ep->stop || ep->ctx->bufMode == BUF_LINEAR) {
        out->draining = true;
        out->deadline = 0;
    } else {
        ep->position %= ep->ctx->bufSize;
        notify_boundary(ep);
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
    if (!endpoint_active(ep) || out->draining)
        return 0;

    int16_t sample = ep->ctx->buffer[ep->position];
    output_advance(out);
    return sample;
}

/**
 * Check whether any output has samples left to send to the DSP.
 */
static bool outputs_pending(void)
{
    for (unsigned int i = 0; i < 2; i++) {
        const struct output_state *out = &outputs[i];
        if (out->fm_queue.count
            || (endpoint_active(&out->endpoint) && !out->draining))
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
static void pace_playback(size_t frames)
{
    uint64_t duration = frames * (uint64_t)MSEC_PER_SEC + playback_fraction;
    int64_t period = duration / DSP_SAMPLE_RATE;
    playback_fraction = duration % DSP_SAMPLE_RATE;
    playback_deadline += period;
    if (playback_deadline + period <= k_uptime_get())
        playback_deadline = k_uptime_get() + MAX(1, period);
}

/**
 * Write one stereo frame (speaker left, radio right) to the DSP, paced to
 * the 48 kHz rate.
 */
static void play_frame(void)
{
    if (!outputs_pending() || !playback_running
        || k_uptime_get() < playback_deadline)
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
    pace_playback(buffer.frameCount);
}

/**
 * Time at which everything written to the DSP so far will have played: the
 * samples still in its FIFO, its latency and a margin. Publishing the final
 * frame does not mean it reached the DAC.
 */
static int64_t drain_deadline(void)
{
    ICStream_Producer_fetchRemote(playback_track.mICStream);
    const ICStreamFifo *fifo = &playback_track.mICStream->share->fifo;
    size_t frames = (fifo->size - fifo_avail(fifo)) / (2 * sizeof(int16_t));
    return k_uptime_get() + DIV_ROUND_UP(frames * MSEC_PER_SEC, DSP_SAMPLE_RATE)
         + playback_track.mLatency + DRAIN_MARGIN_MS;
}

/**
 * Complete draining outputs once their last samples have played. This is an
 * estimate; draining does not read application memory.
 */
static void finish_outputs(void)
{
    for (unsigned int i = 0; i < 2; i++) {
        struct output_state *out = &outputs[i];
        struct endpoint *ep = &out->endpoint;
        if (!endpoint_active(ep) || !out->draining)
            continue;

        if (out->deadline == 0) {
            out->deadline = drain_deadline();
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
    state = init_result == 0 ? CODEC_RUNNING : CODEC_OFF;
    k_condvar_broadcast(&changed);
    k_mutex_unlock(&audio_mutex);
    if (init_result != 0)
        return;

    for (;;) {
        k_mutex_lock(&audio_mutex, K_FOREVER);
        if (state == CODEC_STOPPING)
            break;
        apply_routes();
        if (capture_running)
            capture_frame();
        play_frame();
        finish_outputs();
        k_mutex_unlock(&audio_mutex);
        k_sleep(K_MSEC(1));
    }

    /* Still holding audio_mutex from the last iteration */
    set_capture(false);
    set_playback(false);
    AudioRecord_dtor(&record);
    AudioTrack_dtor(&playback_track);
    state = CODEC_OFF;
    k_condvar_broadcast(&changed);
    k_mutex_unlock(&audio_mutex);
}

/**
 * Clear the routes and endpoints before the worker starts.
 */
static void reset_state(void)
{
    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));
    memset(input_enabled, 0, sizeof(input_enabled));
    for (unsigned int i = 0; i < 2; i++) {
        inputs[i].endpoint.input = true;
        inputs[i].endpoint.channel = outputs[i].endpoint.channel = i;
        output_source[i] = NO_SOURCE;
    }
    capture_running = playback_running = routes_dirty = false;
}

/* The board serializes init/terminate. Wait for the worker to construct the
 * DSP streams so initialization failures are reported to the board caller. */
int csk6codec_init(void)
{
    k_mutex_lock(&audio_mutex, K_FOREVER);
    if (state != CODEC_OFF) {
        k_mutex_unlock(&audio_mutex);
        return -EBUSY;
    }

    reset_state();
    state = CODEC_STARTING;
    k_thread_create(&worker, worker_stack, K_THREAD_STACK_SIZEOF(worker_stack),
                    audio_worker, NULL, NULL, NULL, WORKER_PRIORITY, 0,
                    K_NO_WAIT);
    k_thread_name_set(&worker, "csk6_codec");
    while (state == CODEC_STARTING)
        k_condvar_wait(&changed, &audio_mutex, K_FOREVER);
    int ret = init_result;
    k_mutex_unlock(&audio_mutex);

    if (ret != 0)
        k_thread_join(&worker, K_FOREVER);
    return ret;
}

/**
 * Check whether a callback is still waiting on any endpoint.
 */
static bool any_endpoint_busy(void)
{
    for (unsigned int i = 0; i < 2; i++) {
        if (inputs[i].endpoint.busy || outputs[i].endpoint.busy)
            return true;
    }
    return false;
}

void csk6codec_terminate(void)
{
    k_mutex_lock(&audio_mutex, K_FOREVER);
    if (state != CODEC_RUNNING) {
        k_mutex_unlock(&audio_mutex);
        return;
    }
    state = CODEC_STOPPING;
    for (unsigned int i = 0; i < 2; i++) {
        detach(&inputs[i].endpoint);
        detach(&outputs[i].endpoint);
    }
    k_mutex_unlock(&audio_mutex);

    k_thread_join(&worker, K_FOREVER);

    /* Let cancelled callbacks return before a later init resets their state. */
    k_mutex_lock(&audio_mutex, K_FOREVER);
    while (any_endpoint_busy())
        k_condvar_wait(&changed, &audio_mutex, K_FOREVER);
    k_mutex_unlock(&audio_mutex);
}

void csk6codec_setRoute(enum AudioSource source, enum AudioSink sink,
                        bool enabled)
{
    if (source > SOURCE_MCU || sink > SINK_MCU)
        return;

    k_mutex_lock(&audio_mutex, K_FOREVER);
    if (state != CODEC_RUNNING) {
        k_mutex_unlock(&audio_mutex);
        return;
    }

    if (sink == SINK_MCU && source != SOURCE_MCU) {
        /* Capture into an application buffer */
        input_enabled[source] = enabled;
        if (!enabled)
            detach(&inputs[source].endpoint);
    } else if (sink != SINK_MCU) {
        /* Playback from an application buffer, or an FM passthrough */
        output_source[sink] = enabled ? (int)source : NO_SOURCE;
        queue_reset(&outputs[sink].fm_queue);
        if (!enabled)
            detach(&outputs[sink].endpoint);
    }

    /* Return only after the worker has applied the device start/stop. */
    routes_dirty = true;
    while (routes_dirty && state == CODEC_RUNNING)
        k_condvar_wait(&changed, &audio_mutex, K_FOREVER);
    k_mutex_unlock(&audio_mutex);
}

/**
 * Check a stream's channel, buffer and sample rate. Supports 8, 16, 24 and
 * 48 kHz.
 */
static bool stream_valid(uint8_t channel, const struct streamCtx *ctx)
{
    if (channel >= 2 || ctx->buffer == NULL || ctx->bufSize == 0)
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
    ep->boundaries = ep->observed = 0;
    ep->divisor = DSP_SAMPLE_RATE / ctx->sampleRate;

    if (ep->input) {
        if (!rearm)
            inputs[ep->channel].phase = 0;
    } else {
        struct output_state *out = &outputs[ep->channel];
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
static int stream_start(uint8_t channel, const void *config,
                        struct streamCtx *ctx)
{
    if (!stream_valid(channel, ctx))
        return -EINVAL;

    const bool input = (enum Csk6CodecMode)(uintptr_t)config
                    == CSK6_CODEC_MODE_INPUT;
    struct endpoint *ep = input ? &inputs[channel].endpoint :
                                  &outputs[channel].endpoint;
    int ret = 0;

    k_mutex_lock(&audio_mutex, K_FOREVER);
    const bool routed = input ? input_enabled[channel] :
                                output_source[channel] == SOURCE_MCU;
    if (state != CODEC_RUNNING || !routed) {
        ret = -ENODEV;
    } else if (ep->busy || (ep->ctx != NULL && ep->ctx != ctx)) {
        ret = -EBUSY;
    } else {
        bool rearm = ep->ctx == ctx && ctx->bufMode == BUF_LINEAR;
        if (rearm && ep->error)
            ret = ep->error;
        else
            attach(ep, ctx, rearm);
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
        count = block_samples(ctx);
        *buffer = ctx->buffer;
        /* Initially the second half is idle. After each boundary the
         * completed half is available until the next boundary. */
        if (ctx->bufMode == BUF_CIRC_DOUBLE && ep->position < (size_t)count)
            *buffer += count;
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
    int64_t deadline = io_deadline(ctx);
    while (ep->ctx == ctx && ctx->running && ep->error == 0
           && ep->boundaries == ep->observed) {
        if (!wait_for_change(ep, deadline))
            break;
    }

    int ret = ep->ctx == ctx ? ep->error : -ECANCELED;
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
 * Let an output stream finish before it is detached: RF output finishes its
 * current half, to keep the last TX frame, and linear output its buffer;
 * both then drain the DSP FIFO. Circular speaker streams stop at once, for
 * the M17 decoder's shutdown. Call with audio_mutex held.
 */
static void finish_output_stream(struct endpoint *ep, struct streamCtx *ctx)
{
    /* If another callback is already waiting, the caller detaches at once
     * to cancel it. */
    if (ep->input || ep->error != 0 || ep->busy)
        return;
    if (ep->channel != SINK_RTX && ctx->bufMode != BUF_LINEAR)
        return;

    ep->busy = true;
    ep->stop = true;
    int64_t deadline = io_deadline(ctx);
    while (ep->ctx == ctx && ctx->running && ep->error == 0) {
        if (!wait_for_change(ep, deadline))
            break;
    }
    ep->busy = false;
}

/**
 * audioDriver stop: let radio and linear output finish, then detach.
 */
static void stream_stop(struct streamCtx *ctx)
{
    k_mutex_lock(&audio_mutex, K_FOREVER);
    struct endpoint *ep = ctx->priv;
    if (ep != NULL && ep->ctx == ctx) {
        finish_output_stream(ep, ctx);
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
