/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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
