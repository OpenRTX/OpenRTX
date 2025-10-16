/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/baseband/HR_C6000.h"
#include <pthread.h>
#include <errno.h>
#include "Cx000_dac.h"

#define TONE_BASE_FREQ    125 // [Hz]
#define DAC_FIFO_SIZE     32

enum FuncMode
{
    DAC_OFF    = 0,
    DAC_BEEP   = 1,
    DAC_STREAM = 2
};

/*
 * Sine table for beep tone generation, composed of 64 samples of a 125Hz
 * sinewave sampled at 8kHz. Data is in big endian format as required by the
 * HR_Cx000 DAC.
 */
static const uint16_t sineTable[] =
{
    0x0000, 0x8c0c, 0xf918, 0x2825, 0xfb30, 0x563c, 0x1c47, 0x3351,
    0x825a, 0xf162, 0x6d6a, 0xe270, 0x4176, 0x7c7a, 0x897d, 0x617f,
    0xff7f, 0x617f, 0x897d, 0x7c7a, 0x4176, 0xe270, 0x6d6a, 0xf162,
    0x825a, 0x3351, 0x1c47, 0x563c, 0xfb30, 0x2825, 0xf918, 0x8c0c,
    0x0000, 0x74f3, 0x07e7, 0xd8da, 0x05cf, 0xaac3, 0xe4b8, 0xcdae,
    0x7ea5, 0x0f9d, 0x9395, 0x1e8f, 0xbf89, 0x8485, 0x7782, 0x9f80,
    0x0180, 0x9f80, 0x7782, 0x8485, 0xbf89, 0x1e8f, 0x9395, 0x0f9d,
    0x7ea5, 0xcdae, 0xe4b8, 0xaac3, 0x05cf, 0xd8da, 0x07e7, 0x74f3
};

static HR_C6000 *c6000;
static uint8_t funcMode = DAC_OFF;
static bool syncPoint = false;
static bool stopReq   = false;
static size_t readPos;
static size_t beepIncr;
static struct streamCtx *stream;
static pthread_mutex_t  mutex;
static pthread_cond_t   wakeup_cond;

static inline void stopStream()
{
    funcMode = DAC_OFF;
    stream->running = 0;

    // Clear the "OpenMusic" bit
    c6000->writeCfgRegister(0x06, 0x20);
}

void Cx000dac_init(HR_C6000 *device)
{
    c6000 = device;
    c6000->init();

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&wakeup_cond, NULL);
}

void Cx000dac_terminate()
{
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&wakeup_cond);
}

void Cx000dac_task()
{
    if(funcMode == DAC_OFF)
        return;

    // Check if FIFO is empty
    uint8_t reg = c6000->readCfgRegister(0x88);
    if((reg & 0x01) == 1)
        return;

    // Need to refill the FIFO
    bool isSyncPoint = false;
    size_t prevRdPos = readPos;

    // In beep mode, just refill the DAC FIFO and return since there is no
    // thread to wake up.
    if(funcMode == DAC_BEEP)
    {
        uint16_t data[DAC_FIFO_SIZE];
        for(size_t i = 0; i < DAC_FIFO_SIZE; i += 1)
        {
            readPos += beepIncr;
            data[i] = sineTable[(readPos >> 16) & 0x3F];
        }

        c6000->sendAudio((uint8_t *) data);
        return;
    }

    // Stream mode: copy a new block of samples, update the read index and
    // manage thread synchronization.
    uint8_t *sound = (uint8_t *)(&stream->buffer[readPos]);
    c6000->sendAudio(sound);
    readPos += 32;

    // For circular buffer mode, check if the half of the buffer has been
    // crossed: this is a thread sync point.
    if(stream->bufMode == BUF_CIRC_DOUBLE)
    {
        size_t half = stream->bufSize / 2;
        if((prevRdPos < half) && (readPos >= half))
            isSyncPoint = true;
    }

    // Check if buffer end has been reached, this is a thread sync point for
    // both linear and circular buffer modes. When in linear mode, transfer
    // ends.
    if(readPos >= stream->bufSize)
    {
        isSyncPoint = true;
        readPos     = 0;

        if(stream->bufMode == BUF_LINEAR)
            stopReq = true;
    }

    // Wake up thread(s) waiting to be synced with the stream.
    if(isSyncPoint == true)
    {
        pthread_mutex_lock(&mutex);
        syncPoint = true;

        if(stopReq == true)
            stopStream();

        pthread_cond_signal(&wakeup_cond);
        pthread_mutex_unlock(&mutex);
    }
}

int Cx000dac_startBeep(const uint16_t freq)
{
    if(freq < TONE_BASE_FREQ)
        return -EINVAL;

    if(funcMode != DAC_OFF)
        return -EBUSY;

    beepIncr = (freq << 16) / TONE_BASE_FREQ;
    readPos  = 0;
    funcMode = DAC_BEEP;

    // Set the "OpenMusic" bit
    c6000->writeCfgRegister(0x06, 0x22);

    return 0;
}

void Cx000dac_stopBeep()
{
    // Stop only beeps, streams have an higher priority
    if(funcMode != DAC_BEEP)
        return;

    // Clear the "OpenMusic" bit
    c6000->writeCfgRegister(0x06, 0x20);
    funcMode = DAC_OFF;
}

static int Cx000dac_start(const uint8_t instance, const void *config,
                          struct streamCtx *ctx)
{
    (void) config;
    (void) instance;

    if((ctx == NULL) || (ctx->running != 0))
        return -EINVAL;

    // Require that buffer size is an integer multiple of 32 samples.
    if((ctx->bufSize % 32) != 0)
        return -EINVAL;

    if(funcMode == DAC_STREAM)
        return -EBUSY;

    // Stream not running and thread idle, set up a new stream
    pthread_mutex_lock(&mutex);
    ctx->running = 1;
    pthread_mutex_unlock(&mutex);

    stopReq   = false;
    syncPoint = false;
    readPos   = 0;
    stream    = ctx;

    // HR_Cx000 DAC requires data to be in big endian format
    for(size_t i = 0; i < ctx->bufSize; i++)
    {
        stream_sample_t tmp = ctx->buffer[i];
        ctx->buffer[i] = __builtin_bswap16(tmp);
    }

    // Set the "OpenMusic" bit
    c6000->writeCfgRegister(0x06, 0x22);

    // Audio stream mode takes over beep: switching to stream mode will start
    // the new stream as soon as the HR_Cx000 sample buffer is empty.
    //
    // TODO: the audio management module ensures that the DAC is accessed by
    // only one thread at a time, so we *should* be safe setting the funcMode
    // without a critical section.
    funcMode = DAC_STREAM;

    return 0;
}

static int Cx000dac_idleBuf(struct streamCtx *ctx, stream_sample_t **buf)
{
    // Idle buffer is present only in circular mode
    if(ctx->bufMode != BUF_CIRC_DOUBLE)
    {
        *buf = NULL;
        return 0;
    }

    // If reading the first half, the second half is free and vice-versa
    if(readPos < (ctx->bufSize/2))
        *buf = ctx->buffer + (ctx->bufSize/2);
    else
        *buf = ctx->buffer;

    return ctx->bufSize/2;
}

static int Cx000dac_sync(struct streamCtx *ctx, uint8_t dirty)
{
    if(ctx->running == 0)
        return -1;

    // HR_Cx000 DAC requires data to be in big endian format
    if((ctx->bufMode == BUF_CIRC_DOUBLE) && (dirty != 0))
    {
        stream_sample_t *ptr;
        Cx000dac_idleBuf(ctx, &ptr);

        for(size_t i = 0; i < ctx->bufSize/2; i++)
        {
            stream_sample_t tmp = ptr[i];
            ptr[i] = __builtin_bswap16(tmp);
        }
    }

    pthread_mutex_lock(&mutex);

    // Check for buffer overruns
    if(syncPoint == true)
    {
        syncPoint = false;
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    // Wait for sync point
    while(syncPoint == false)
    {
        pthread_cond_wait(&wakeup_cond, &mutex);
    }

    syncPoint = false;
    pthread_mutex_unlock(&mutex);
    return 0;
}

static void Cx000dac_stop(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    stopReq = true;
}

static void Cx000dac_halt(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    stopStream();
}

#pragma GCC diagnostic ignored "-Wpedantic"
const struct audioDriver Cx000_dac_audio_driver =
{
    .start     = Cx000dac_start,
    .data      = Cx000dac_idleBuf,
    .sync      = Cx000dac_sync,
    .stop      = Cx000dac_stop,
    .terminate = Cx000dac_halt
};
#pragma GCC diagnostic pop
