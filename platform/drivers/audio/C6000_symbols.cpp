/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <array>
#include <errno.h>
#include <pthread.h>
#include "interfaces/delays.h"
#include "C6000_symbols.h"

/*
 * Each timeslot interrupt starts a slot, during which the chip sends what the
 * previous interrupt gave it: the data written at interrupt n goes out
 * between interrupts n + 1 and n + 2. After the stream's last symbol, one
 * slot of idle follows and the stream ends at the interrupt after it.
 */

static constexpr uint32_t SYMBOL_RATE = 4800;
static constexpr size_t SLOT_SYMBOLS = 4 * HR_C6000::SYMBOL_SLOT_BYTES;
static constexpr uint32_t TAIL_SLOTS = 2;  // The last data, then idle
static constexpr long long STALL_MS = 200; // Slot clock stopped

static HR_C6000 *c6000;
static struct streamCtx *stream;
static volatile uint32_t tsCount; // Timeslot interrupts
static uint32_t tsSeen;
static uint32_t slot;             // Interrupts since the start
static long long slotTime;
static size_t readPos;
static uint32_t tail; // Slots left after the data
static bool stopReq;
static bool syncPoint;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wakeup = PTHREAD_COND_INITIALIZER;

/**
 * Dibit of a symbol, as the chip takes it.
 *
 * @param sym: -3, -1, +1 or +3, or 0 for idle.
 * @param pos: position of the symbol in its slot.
 * @return +1 00, +3 01, -1 10, -3 11; idle is +3 at even positions and -3 at
 * odd ones.
 */
static uint8_t dibit(const stream_sample_t sym, const size_t pos)
{
    if (sym == 0)
        return ((pos % 2) == 0) ? 0x1 : 0x3;

    const uint8_t sign = (sym < 0) ? 0x2 : 0x0;
    const uint8_t outer = ((sym > 1) || (sym < -1)) ? 0x1 : 0x0;

    return sign | outer;
}

/**
 * End the stream: stop the chip and wake up the thread waiting on it. Call
 * with the mutex held.
 */
static void endStream()
{
    c6000->stopSymbolTx();
    stream->running = 0;
    stream = NULL;
    syncPoint = true;
    pthread_cond_signal(&wakeup);
}

void C6000symbols_init(HR_C6000 *device)
{
    c6000 = device;
}

void C6000symbols_timeslotIrq()
{
    tsCount = tsCount + 1;
}

void C6000symbols_task()
{
    pthread_mutex_lock(&mutex);

    if (stream == NULL) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    const uint32_t now = tsCount;
    if (now == tsSeen) {
        if ((getTick() - slotTime) > STALL_MS)
            endStream();

        pthread_mutex_unlock(&mutex);
        return;
    }

    slot += now - tsSeen;
    tsSeen = now;
    slotTime = getTick();

    // The data has gone out: done
    if ((tail > 0) && (--tail == 0)) {
        endStream();
        pthread_mutex_unlock(&mutex);
        return;
    }

    // The next slot's symbols, idle once the data has ended
    const size_t half = stream->bufSize / 2;
    std::array<uint8_t, HR_C6000::SYMBOL_SLOT_BYTES> bytes = {};

    for (size_t i = 0; i < SLOT_SYMBOLS; i++) {
        stream_sample_t sym = 0;

        if (tail == 0) {
            sym = stream->buffer[readPos++];

            if ((readPos == half) || (readPos == stream->bufSize)) {
                if (readPos == stream->bufSize)
                    readPos = 0;

                if (stopReq) {
                    tail = TAIL_SLOTS;
                } else {
                    syncPoint = true;
                    pthread_cond_signal(&wakeup);
                }
            }
        }

        bytes[i / 4] = (bytes[i / 4] << 2) | dibit(sym, i);
    }

    c6000->writeSymbolSlot(slot, bytes.data());
    pthread_mutex_unlock(&mutex);
}

static int symbols_start(const uint8_t instance, const void *config,
                         struct streamCtx *ctx)
{
    (void)instance;
    (void)config;

    if ((ctx == NULL) || (c6000 == NULL))
        return -EINVAL;

    if ((ctx->running != 0) || (stream != NULL))
        return -EBUSY;

    if ((ctx->sampleRate != SYMBOL_RATE) || (ctx->bufMode != BUF_CIRC_DOUBLE)
        || ((ctx->bufSize % 2) != 0))
        return -EINVAL;

    // Idle until the stream's first data, not what the buffer held before
    std::fill(ctx->buffer, ctx->buffer + ctx->bufSize, 0);

    pthread_mutex_lock(&mutex);

    readPos = 0;
    tail = 0;
    slot = 0;
    stopReq = false;
    syncPoint = false;
    tsSeen = tsCount;
    slotTime = getTick();
    c6000->startSymbolTx();
    ctx->running = 1;
    stream = ctx;

    pthread_mutex_unlock(&mutex);

    return 0;
}

static int symbols_data(struct streamCtx *ctx, stream_sample_t **buf)
{
    const size_t half = ctx->bufSize / 2;

    // While the chip takes one half, the other is free
    pthread_mutex_lock(&mutex);
    *buf = (readPos < half) ? &ctx->buffer[half] : ctx->buffer;
    pthread_mutex_unlock(&mutex);

    return half;
}

static int symbols_sync(struct streamCtx *ctx, uint8_t dirty)
{
    (void)dirty;
    int ret = 0;

    pthread_mutex_lock(&mutex);

    if (ctx->running == 0) {
        ret = -1;
    } else if (stopReq) {
        // Stopping: wait for the last symbols to go out
        while (ctx->running != 0)
            pthread_cond_wait(&wakeup, &mutex);
    } else if (syncPoint) {
        // The chip already took the half: it went out late
        ret = -1;
    } else {
        while ((syncPoint == false) && (ctx->running != 0))
            pthread_cond_wait(&wakeup, &mutex);

        ret = (ctx->running != 0) ? 0 : -1;
    }

    syncPoint = false;
    pthread_mutex_unlock(&mutex);

    return ret;
}

static void symbols_stop(struct streamCtx *ctx)
{
    (void)ctx;

    pthread_mutex_lock(&mutex);
    stopReq = true;
    pthread_mutex_unlock(&mutex);
}

static void symbols_halt(struct streamCtx *ctx)
{
    pthread_mutex_lock(&mutex);
    if ((stream != NULL) && (stream == ctx))
        endStream();
    pthread_mutex_unlock(&mutex);
}

#pragma GCC diagnostic ignored "-Wpedantic"
const struct audioDriver C6000_symbols_audio_driver = {
    .start = symbols_start,
    .data = symbols_data,
    .sync = symbols_sync,
    .stop = symbols_stop,
    .terminate = symbols_halt,
};
#pragma GCC diagnostic pop
