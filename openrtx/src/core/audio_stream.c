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

#include <audio_stream.h>
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
    uint8_t endpoint = pathInfo.source;

    if((mode & 0xF0) == STREAM_OUTPUT)
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
    if(dev == NULL)
        return -ENODEV;

    // Search for an empty audio stream slot
    streamId id = -1;
    for(size_t i = 0; i < MAX_NUM_STREAMS; i++)
    {
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
    streams[id].ctx.bufMode    = (mode & 0x0F);
    streams[id].ctx.bufSize    = length;
    streams[id].ctx.sampleRate = sampleRate;

    int ret = dev->driver->start(dev->instance, dev->config, &streams[id].ctx);
    if(ret < 0)
    {
        streams[id].ctx.running = 0;
        streams[id].path = 0;
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
    dataBlock_t block;
    block.data = NULL;
    block.len  = 0;

    if(validateStream(id) == false)
        return block;

    int ret = streams[id].dev->driver->sync(&(streams[id].ctx), false);
    if(ret < 0)
        return block;

    ret = streams[id].dev->driver->data(&(streams[id].ctx), &block.data);
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
