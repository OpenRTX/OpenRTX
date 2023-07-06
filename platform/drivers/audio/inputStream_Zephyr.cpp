/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Alain Carlucci                           *
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

#include <hwconfig.h>
#include <interfaces/audio_stream.h>
#include <stddef.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>

streamId gNextAvailableStreamId = 0;

class InputStream
{
   public:
    InputStream(enum AudioSource source,
                enum AudioPriority priority,
                stream_sample_t* buf,
                size_t bufLength,
                enum BufMode mode,
                uint32_t sampleRate)
    {
        // TODO
        ;
    }

    bool isValid() const
    {
        return m_valid;
    }

    ~InputStream()
    {
        // TODO
        ;
    }

    dataBlock_t getDataBlock()
    {
        // TODO
        return {NULL, 0};
    }

    AudioPriority priority() const
    {
        return m_prio;
    }

    streamId id() const
    {
        return m_id;
    }

    void changeId()
    {
        // TODO
        ;
    }

    void setStreamData(AudioPriority priority,
                       stream_sample_t* buf,
                       size_t bufLength,
                       BufMode mode,
                       uint32_t sampleRate)
    {
        // TODO
        ;
    }

   private:
    bool m_valid    = false;
    FILE* m_fp      = nullptr;
    uint64_t m_size = 0;

    streamId m_id;
    AudioPriority m_prio;
    BufMode m_mode;
    uint32_t m_sampleRate = 0;

    stream_sample_t* m_buf = nullptr;
    size_t m_bufLength     = 0;

    size_t m_db_curwrite = 0;
    size_t m_db_curread  = 0;

    // This is a blocking function that emulates an ADC writing to the
    // specified memory region. It takes the same time that an ADC would take
    // to sample the same quantity of data.
    bool fillBuffer(stream_sample_t* dest, size_t sz)
    {
        // TODO
        return true;
    }

    void stopThread()
    {
        // TODO
        ;
    }
};

std::map<AudioSource, std::unique_ptr<InputStream>> gOpenStreams;

streamId inputStream_start(const enum AudioSource source,
                           const enum AudioPriority priority,
                           stream_sample_t* const buf,
                           const size_t bufLength,
                           const enum BufMode mode,
                           const uint32_t sampleRate)
{
    // TODO
    return 0;
}

dataBlock_t inputStream_getData(streamId id)
{
    // TODO
    InputStream* stream = nullptr;
    if (stream == nullptr) return dataBlock_t{NULL, 0};
    return stream->getDataBlock();
}

void inputStream_stop(streamId id)
{
    // TODO
    ;
}
