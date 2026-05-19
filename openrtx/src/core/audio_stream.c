/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/audio_stream.h"
#include <errno.h>

#define MAX_NUM_STREAMS 3
#define MAX_NUM_DEVICES 3

struct streamState
{
    const struct audioDevice *dev;
    struct streamCtx          ctx;
    pathId                    path;
};

static struct streamState streams[MAX_NUM_STREAMS] = {0};


/**
 * \internal
 * Verify if the path associated to a given stream is still open and, if path is
 * closed or suspended, terminate the stream.
 *
 * @param id: stream ID.
 * @return true if the path is valid.
 */
static bool validateStream(const streamId id)
{
    if((id < 0) || (id >= MAX_NUM_STREAMS))
        return false;

    uint8_t status = audioPath_getStatus(streams[id].path);
    if(status != PATH_OPEN)
    {
        // Path has been closed or suspended: terminate the stream and free it
        streams[id].dev->driver->terminate(&(streams[id].ctx));
        streams[id].path = 0;

        return false;
    }

    // Path is still open
    return true;
}

/**
 * \internal
 * Start an audio stream. In case of failure, the stream is freed.
 *
 * @param id: stream ID.
 * @return zero if the stream started, a negative error code otherwise.
 */
static int startStream(const streamId id)
{
    const struct audioDevice *dev = streams[id].dev;

    int ret = dev->driver->start(dev->instance, dev->config, &streams[id].ctx);
    if(ret < 0)
    {
        streams[id].ctx.running = 0;
        streams[id].path = 0;
    }

    return ret;
}

streamId audioStream_start(const pathId path, stream_sample_t * const buf,
                           const size_t length, const uint32_t sampleRate,
                           const uint8_t mode)
{
    // Check for invalid stream mode or invalid buffer handling mode
    if(((mode & 0xF0) == 0) || ((mode & 0x0F) == 0))
        return -EINVAL;

    pathInfo_t pathInfo = audioPath_getInfo(path);
    if(pathInfo.status != PATH_OPEN)
        return -EPERM;

    // Search for an audio device serving the correct output endpoint.
    const struct audioDevice *dev  = NULL;
    const struct audioDevice *devs = inputDevices;
    const uint8_t streamMode = (mode & 0xF0);
    const uint8_t bufMode = (mode & 0x0F);
    uint8_t endpoint = pathInfo.source;

    if(streamMode == STREAM_OUTPUT)
    {
        devs = outputDevices;
        endpoint = pathInfo.sink;
    }

    for(size_t i = 0; i < MAX_NUM_DEVICES; i++)
    {
        if(devs[i].endpoint == endpoint)
            dev = &devs[i];
    }

    // No audio device found
    if((dev == NULL) || (dev->driver == NULL))
        return -ENODEV;

    // Search for an empty audio stream slot
    streamId id = -1;
    for(size_t i = 0; i < MAX_NUM_STREAMS; i++)
    {
        // While searching, cleanup dead streams
        if(streams[i].path > 0)
        {
            if(audioPath_getStatus(streams[i].path) != PATH_OPEN)
            {
                streams[i].dev->driver->terminate(&(streams[i].ctx));
                streams[i].path = 0;
            }
        }

        // Empty stream found
        if((streams[i].path <= 0) && (streams[i].ctx.running == 0))
            id = i;
    }

    // No stream slots available
    if(id < 0)
        return -EBUSY;

    // Setup new stream and start it
    streams[id].path           = path;
    streams[id].dev            = dev;
    streams[id].ctx.buffer     = buf;
    streams[id].ctx.bufMode    = bufMode;
    streams[id].ctx.bufSize    = length;
    streams[id].ctx.sampleRate = sampleRate;

    // In circular mode, start immediately
    if(bufMode == BUF_CIRC_DOUBLE)
    {
        int ret = startStream(id);
        if(ret < 0)
            return ret;
    }

    return id;
}

void audioStream_stop(const streamId id)
{
    if((id < 0) || (id >= MAX_NUM_STREAMS))
        return;

    if(streams[id].path == 0)
        return;

    streams[id].dev->driver->stop(&(streams[id].ctx));
    streams[id].dev->driver->sync(&(streams[id].ctx), false);
    streams[id].path = 0;
}

void audioStream_terminate(const streamId id)
{
    if((id < 0) || (id >= MAX_NUM_STREAMS))
        return;

    if(streams[id].path == 0)
        return;

    streams[id].dev->driver->terminate(&(streams[id].ctx));
    streams[id].path = 0;
}

dataBlock_t inputStream_getData(streamId id)
{
    const struct audioDevice *dev = streams[id].dev;
    dataBlock_t block = {NULL, 0};
    int ret;

    if(validateStream(id) == false)
        return block;

    if(streams[id].ctx.bufMode == BUF_LINEAR)
    {
        ret = startStream(id);
        if(ret < 0)
        {
            streams[id].ctx.running = 0;
            streams[id].path = 0;
            return block;
        }
    }

    ret = dev->driver->sync(&(streams[id].ctx), false);
    if(ret < 0)
        return block;

    ret = dev->driver->data(&(streams[id].ctx), &block.data);
    if(ret < 0)
    {
        block.data = NULL;
        return block;
    }

    block.len = (size_t) ret;
    return block;
}

stream_sample_t *outputStream_getIdleBuffer(const streamId id)
{
    if(validateStream(id) == false)
        return NULL;

    stream_sample_t *buf;
    int ret = streams[id].dev->driver->data(&(streams[id].ctx), &buf);
    if(ret < 0)
        return NULL;

    return buf;
}

bool outputStream_sync(const streamId id, const bool bufChanged)
{
    if(validateStream(id) == false)
        return false;

    int ret = streams[id].dev->driver->sync(&(streams[id].ctx), bufChanged);
    if(ret < 0)
        return false;

    return true;
}
