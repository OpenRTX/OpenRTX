/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "file_source.h"

static int fileSource_start(const uint8_t instance, const void *config, struct streamCtx *ctx)
{
    (void) instance;

    if(ctx == NULL)
        return -EINVAL;

    if(ctx->running != 0)
        return -EBUSY;

    ctx->running = 1;

    FILE *fp = fopen(config, "rb");
    if(fp == NULL)
    {
        ctx->running = 0;
        return -EINVAL;
    }

    ctx->priv = fp;

    return 0;
}

static int fileSource_data(struct streamCtx *ctx, stream_sample_t **buf)
{
    if(ctx->running == 0)
        return -1;

    FILE *fp  = (FILE *) ctx->priv;
    stream_sample_t *dest = *buf;
    size_t size = ctx->bufSize;
    size_t i;

    if(ctx->bufMode == BUF_CIRC_DOUBLE)
        size /= 2;

    // Read data from the file, rollover when end is reached
    while (i < size)
    {
        size_t n = fread(dest + i, sizeof(stream_sample_t), size - i, fp);
        if (n < (size - i))
            fseek(fp, 0, SEEK_SET);

        i += n;
    }

    return size;
}

static int fileSource_sync(struct streamCtx *ctx, uint8_t dirty)
{
    (void) dirty;

    size_t size = ctx->bufSize;
    if(ctx->bufMode == BUF_CIRC_DOUBLE)
        size /= 2;

    // Simulate the time needed to get a new chunck of data from an equivalent
    // hardware peripheral.
    uint32_t waitTime = (1000000 * size) / ctx->sampleRate;
    usleep(waitTime);

    return 0;
}

static void fileSource_stop(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    FILE *fp = (FILE *) ctx->priv;
    fclose(fp);
}

static void fileSource_halt(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    FILE *fp = (FILE *) ctx->priv;
    fclose(fp);
}

#pragma GCC diagnostic ignored "-Wpedantic"
const struct audioDriver file_source_audio_driver =
{
    .start     = fileSource_start,
    .data      = fileSource_data,
    .sync      = fileSource_sync,
    .stop      = fileSource_stop,
    .terminate = fileSource_halt
};
#pragma GCC diagnostic pop
