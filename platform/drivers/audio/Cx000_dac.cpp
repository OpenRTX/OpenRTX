/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
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

#include <HR_C6000.h>
#include <pthread.h>
#include <errno.h>
#include "Cx000_dac.h"

#include <stdio.h>

static HR_C6000 *c6000;
static bool running   = false;
static bool syncPoint = false;
static bool stopReq   = false;
static size_t readPos;
static struct streamCtx *stream;
static pthread_mutex_t  mutex;
static pthread_cond_t   wakeup_cond;

static inline void stopStream()
{
    running = false;
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
    if(running == false)
        return;

    // Check if FIFO is empty
    uint8_t reg = c6000->readCfgRegister(0x88);
    if((reg & 0x01) == 1)
        return;

    // Need to refill the FIFO
    bool isSyncPoint = false;
    size_t prevRdPos = readPos;
    uint8_t *sound   = (uint8_t *)(&stream->buffer[readPos]);

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

    if(running == true)
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
    running = true;

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
