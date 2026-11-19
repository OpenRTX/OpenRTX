/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <array>
#include <errno.h>
#include <memory>
#include <new>
#include "core/fir.hpp"
#include "interfaces/delays.h"
#include "C6000_fmcopy.h"

/*
 * sync() waits for the chip's system interrupt, reads its next
 * 256 samples at 8 kHz, interpolates them to 24 kHz and gives the stream one
 * half of its buffer. The samples left over wait in a carry buffer for the
 * next half: one copy gives 768 samples, a stream half takes fewer, and both
 * run on the chip's clock.
 */

static constexpr size_t RATIO = 3;          // 8 kHz to 24 kHz
static constexpr size_t COPY_OUT = RATIO * HR_C6000::FM_COPY_SAMPLES;
static constexpr int SHIFT = 4;             // Copy at +-16k to about +-1k
static constexpr uint32_t TIMEOUT_MS = 100; // Three copies

/*
 * Hamming windowed sinc low-pass at 3.6 kHz for 24 kHz, with a gain of three
 * for the zero stuffing.
 */
static constexpr std::array<float, 36> TAPS = {
    -0.0030868f, +0.0007912f, +0.0060016f, +0.0085151f, +0.0021421f,
    -0.0136773f, -0.0264105f, -0.0164267f, +0.0218249f, +0.0622092f,
    +0.0578327f, -0.0165653f, -0.1227407f, -0.1622074f, -0.0389315f,
    +0.2577909f, +0.6183586f, +0.8654523f, +0.8654523f, +0.6183586f,
    +0.2577909f, -0.0389315f, -0.1622074f, -0.1227407f, -0.0165653f,
    +0.0578327f, +0.0622092f, +0.0218249f, -0.0164267f, -0.0264105f,
    -0.0136773f, +0.0021421f, +0.0085151f, +0.0060016f, +0.0007912f,
    -0.0030868f,
};

static HR_C6000 *c6000;
static volatile uint32_t sysCount; // System interrupts
static uint32_t sysSeen;
static Fir<std::tuple_size<decltype(TAPS)>::value> interp(TAPS);
static std::unique_ptr<int16_t[]> carry; // Carried samples, then one copy
static size_t carried;
static uint8_t readyHalf;
static uint8_t nextHalf;

void C6000fmcopy_init(HR_C6000 *device)
{
    c6000 = device;
}

void C6000fmcopy_systemIrq()
{
    sysCount = sysCount + 1;
}

static int fmcopy_start(const uint8_t instance, const void *config,
                        struct streamCtx *ctx)
{
    (void)instance;
    (void)config;

    if ((ctx == NULL) || (c6000 == NULL))
        return -EINVAL;

    if (ctx->running != 0)
        return -EBUSY;

    if ((ctx->sampleRate != RATIO * 8000) || (ctx->bufMode != BUF_CIRC_DOUBLE)
        || ((ctx->bufSize % 2) != 0))
        return -EINVAL;

    // At most a half less one sample carried, then one copy and its input
    const size_t half = ctx->bufSize / 2;
    carry.reset(new (std::nothrow)
                    int16_t[half - 1 + COPY_OUT + HR_C6000::FM_COPY_SAMPLES]);
    if (carry == nullptr)
        return -ENOMEM;

    interp.reset();
    carried = 0;
    readyHalf = 0;
    nextHalf = 0;
    sysSeen = sysCount;
    c6000->startFmCopy();
    ctx->running = 1;

    return 0;
}

static int fmcopy_data(struct streamCtx *ctx, stream_sample_t **buf)
{
    const size_t half = ctx->bufSize / 2;

    *buf = &ctx->buffer[readyHalf * half];
    return half;
}

static int fmcopy_sync(struct streamCtx *ctx, uint8_t dirty)
{
    (void)dirty;

    if (ctx->running == 0)
        return -1;

    const size_t half = ctx->bufSize / 2;
    int16_t *copy = &carry[half - 1 + COPY_OUT];
    uint32_t waited = 0;

    while (carried < half) {
        if (sysCount == sysSeen) {
            if (waited++ >= TIMEOUT_MS)
                return -1;

            sleepFor(0, 1);
            continue;
        }

        sysSeen = sysCount;
        c6000->readFmCopy(copy);

        for (size_t i = 0; i < HR_C6000::FM_COPY_SAMPLES; i++) {
            const float in = static_cast<float>(copy[i] >> SHIFT);
            for (size_t p = 0; p < RATIO; p++) {
                const float out = interp((p == 0) ? in : 0.0f);
                carry[carried++] = static_cast<int16_t>(
                    std::min(std::max(out, -32768.0f), 32767.0f));
            }
        }
    }

    std::copy(&carry[0], &carry[half], &ctx->buffer[nextHalf * half]);
    std::copy(&carry[half], &carry[carried], &carry[0]);
    carried -= half;
    readyHalf = nextHalf;
    nextHalf ^= 1;

    return 0;
}

static void fmcopy_halt(struct streamCtx *ctx)
{
    if (ctx->running == 0)
        return;

    c6000->stopFmCopy();
    carry.reset();
    ctx->running = 0;
}

#pragma GCC diagnostic ignored "-Wpedantic"
const struct audioDriver C6000_fmcopy_audio_driver = {
    .start = fmcopy_start,
    .data = fmcopy_data,
    .sync = fmcopy_sync,
    .stop = fmcopy_halt,
    .terminate = fmcopy_halt,
};
#pragma GCC diagnostic pop
