/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <memory>
#include <new>
#include "core/dsp.h"
#include "core/utils.h"
#include "C6000_fmcopy.h"
#include "C6000_symbols.h"
#include "C6000_modem.h"

static constexpr uint32_t SYMBOL_RATE = 4800;

static const struct audioDriver *outDriver;
static const struct audioDriver *inDriver;
static const struct audioDriver *micDriver;
static bool useChip;

/*
 * The microphone stream has the rate of the codec's samples, which only need
 * their gain. The analog path is sampled faster into a buffer of its own, on
 * the heap for the DMA to reach it, then low pass filtered down to the
 * stream's rate and given its own gain.
 */
static const struct C6000micPaths *micPaths;
static bool micChip;
static std::unique_ptr<stream_sample_t[]> analogBuf;
static size_t analogSize;
static struct streamCtx analogCtx;
static struct lowPassDecimator micLowPass;
static struct dcBlock micDcBlock;
static bool micPrimed;

void C6000modem_useChip(const bool chip)
{
    useChip = chip;
}

static int out_start(const uint8_t instance, const void *config,
                     struct streamCtx *ctx)
{
    const struct C6000modemPath *analog =
        static_cast<const struct C6000modemPath *>(config);

    if (ctx->sampleRate == SYMBOL_RATE) {
        outDriver = &C6000_symbols_audio_driver;
        return outDriver->start(instance, NULL, ctx);
    }

    outDriver = analog->driver;
    return outDriver->start(instance, analog->config, ctx);
}

static int out_data(struct streamCtx *ctx, stream_sample_t **buf)
{
    return outDriver->data(ctx, buf);
}

static int out_sync(struct streamCtx *ctx, uint8_t dirty)
{
    return outDriver->sync(ctx, dirty);
}

static void out_stop(struct streamCtx *ctx)
{
    outDriver->stop(ctx);
}

static void out_terminate(struct streamCtx *ctx)
{
    if (outDriver != NULL)
        outDriver->terminate(ctx);
}

static int in_start(const uint8_t instance, const void *config,
                    struct streamCtx *ctx)
{
    const struct C6000modemPath *analog =
        static_cast<const struct C6000modemPath *>(config);

    if (useChip) {
        inDriver = &C6000_fmcopy_audio_driver;
        return inDriver->start(instance, NULL, ctx);
    }

    inDriver = analog->driver;
    return inDriver->start(instance, analog->config, ctx);
}

static int in_data(struct streamCtx *ctx, stream_sample_t **buf)
{
    return inDriver->data(ctx, buf);
}

static int in_sync(struct streamCtx *ctx, uint8_t dirty)
{
    return inDriver->sync(ctx, dirty);
}

static void in_stop(struct streamCtx *ctx)
{
    inDriver->stop(ctx);
}

static void in_terminate(struct streamCtx *ctx)
{
    if (inDriver != NULL)
        inDriver->terminate(ctx);
}

static int mic_start(const uint8_t instance, const void *config,
                     struct streamCtx *ctx)
{
    micPaths = static_cast<const struct C6000micPaths *>(config);
    micChip = useChip;

    if (micChip) {
        // The analog path's buffer is not needed with the codec
        if (analogCtx.running == 0) {
            analogBuf.reset();
            analogSize = 0;
        }

        micDriver = micPaths->chip.driver;
        return micDriver->start(instance, micPaths->chip.config, ctx);
    }

    int ret = dsp_lowPassInit(&micLowPass, micPaths->analogRatio);
    if (ret < 0)
        return ret;

    const size_t size = ctx->bufSize * micPaths->analogRatio;
    if (size != analogSize) {
        analogBuf.reset(new (std::nothrow) stream_sample_t[size]);
        analogSize = (analogBuf != nullptr) ? size : 0;
    }

    if (analogBuf == nullptr)
        return -ENOMEM;

    dsp_resetState(micDcBlock);
    micPrimed = false;
    micDriver = micPaths->analog.driver;
    analogCtx.buffer = analogBuf.get();
    analogCtx.bufSize = size;
    analogCtx.bufMode = ctx->bufMode;
    analogCtx.sampleRate = ctx->sampleRate * micPaths->analogRatio;

    ret = micDriver->start(instance, micPaths->analog.config, &analogCtx);
    ctx->running = analogCtx.running;

    return ret;
}

static int mic_data(struct streamCtx *ctx, stream_sample_t **buf)
{
    if (micChip) {
        int len = micDriver->data(ctx, buf);
        for (int i = 0; i < len; i++)
            (*buf)[i] = s16_mulSat((*buf)[i], micPaths->chipGain);

        return len;
    }

    stream_sample_t *in;
    int len = micDriver->data(&analogCtx, &in);
    if (len < 0)
        return len;

    // Start the filter settled on the first sample, so the microphone's DC
    // level gives no step
    for (size_t i = 0; (micPrimed == false) && (i < micLowPass.len); i++) {
        int16_t sample = in[0];
        dsp_lowPassDecimate(&micLowPass, &sample);
    }

    micPrimed = true;

    stream_sample_t *out =
        &ctx->buffer[(in - analogCtx.buffer) / micPaths->analogRatio];
    int frames = 0;
    for (int i = 0; i < len; i++) {
        int16_t sample = in[i];
        if (!dsp_lowPassDecimate(&micLowPass, &sample))
            continue;

        sample = dsp_dcBlockFilter(&micDcBlock, sample);
        out[frames++] = s16_mulSat(sample, micPaths->analogGain);
    }

    *buf = out;
    return frames;
}

static int mic_sync(struct streamCtx *ctx, uint8_t dirty)
{
    if (micChip)
        return micDriver->sync(ctx, dirty);

    int ret = micDriver->sync(&analogCtx, dirty);
    ctx->running = analogCtx.running;

    return ret;
}

static void mic_stop(struct streamCtx *ctx)
{
    micDriver->stop(micChip ? ctx : &analogCtx);
}

static void mic_terminate(struct streamCtx *ctx)
{
    if (micDriver == NULL)
        return;

    if (micChip) {
        micDriver->terminate(ctx);
        return;
    }

    micDriver->terminate(&analogCtx);
    ctx->running = 0;
}

#pragma GCC diagnostic ignored "-Wpedantic"
const struct audioDriver C6000_modem_out_audio_driver = {
    .start = out_start,
    .data = out_data,
    .sync = out_sync,
    .stop = out_stop,
    .terminate = out_terminate,
};

const struct audioDriver C6000_modem_in_audio_driver = {
    .start = in_start,
    .data = in_data,
    .sync = in_sync,
    .stop = in_stop,
    .terminate = in_terminate,
};

const struct audioDriver C6000_modem_mic_audio_driver = {
    .start = mic_start,
    .data = mic_data,
    .sync = mic_sync,
    .stop = mic_stop,
    .terminate = mic_terminate,
};
#pragma GCC diagnostic pop
