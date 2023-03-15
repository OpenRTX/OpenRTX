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
#include <audio_stream.h>
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
        : m_run_thread(true), m_func_running(false)
    {
        if (bufLength % 2)
        {
            fprintf(stderr, "InputStream error: invalid bufLength %lu\n",
                    bufLength);
            return;
        }

        m_db_ready[0] = m_db_ready[1] = false;

        std::string sourceString;
        switch (source)
        {
            case SOURCE_MIC:
                sourceString = "MIC";
                break;
            case SOURCE_MCU:
                sourceString = "MCU";
                break;
            case SOURCE_RTX:
                sourceString = "RTX";
                break;
            default:
                break;
        }

        m_fp = fopen((sourceString + ".raw").c_str(), "rb");
        if (!m_fp)
        {
            fprintf(stderr, "InputStream error: cannot open: %s.raw\n",
                    sourceString.c_str());
            return;
        }

        fseek(m_fp, 0, SEEK_END);
        m_size = ftell(m_fp);
        fseek(m_fp, 0, SEEK_SET);
        if (m_size % 2 || m_size == 0)
        {
            fprintf(stderr, "InputStream error: invalid file: %s.raw\n",
                    sourceString.c_str());
            return;
        }

        m_valid = true;

        changeId();
        setStreamData(priority, buf, bufLength, mode, sampleRate);
    }

    bool isValid() const
    {
        return m_valid;
    }

    ~InputStream()
    {
        stopThread();

        if (m_fp) fclose(m_fp);
    }

    dataBlock_t getDataBlock()
    {
        if (!m_valid) return {nullptr, 0};

        switch (m_mode)
        {
            case BufMode::BUF_LINEAR:
            {
                // With this mode, just sleep for the right amount of time
                // and return the buffer content
                if (!fillBuffer(m_buf, m_bufLength)) return {NULL, 0};

                return {m_buf, m_bufLength};
            }
            case BufMode::BUF_CIRC_DOUBLE:
            {
                // If this mode is selected, wait for the readiness of the
                // current slice and return it

                int id = m_db_curread;

                // Wait for `m_buf` to be ready
                while (!m_db_ready[id] && m_run_thread)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                if (!m_run_thread) return {NULL, 0};

                // Return the buffer contents
                auto* pos      = m_buf + id * (m_bufLength / 2);
                m_db_ready[id] = 0;

                // Update the read buffer
                m_db_curread = (id + 1) % 2;
                return {pos, m_bufLength / 2};
            }
            default:
                return {NULL, 0};
        }
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
        m_id = gNextAvailableStreamId;
        gNextAvailableStreamId += 1;
    }

    void setStreamData(AudioPriority priority,
                       stream_sample_t* buf,
                       size_t bufLength,
                       BufMode mode,
                       uint32_t sampleRate)
    {
        if (!m_valid) return;

        stopThread();
        m_run_thread = true;  // set it as runnable again

        // HERE stop thread
        m_prio       = priority;
        m_buf        = buf;
        m_bufLength  = bufLength;
        m_mode       = mode;
        m_sampleRate = sampleRate;

        switch (m_mode)
        {
            case BufMode::BUF_LINEAR:
                // TODO: stop a running thread
                break;
            case BufMode::BUF_CIRC_DOUBLE:
                m_thread =
                    std::thread(std::bind(&InputStream::threadFunc, this));
                // TODO: start thread
                break;
        }
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
    std::atomic<bool> m_db_ready[2];
    std::atomic<bool> m_run_thread;
    std::atomic<bool> m_func_running;
    std::thread m_thread;

    // Emulate an ADC that reads to the circular buffer
    void threadFunc()
    {
        m_db_ready[0] = m_db_ready[1] = false;
        while (m_run_thread)
        {
            m_db_ready[0] = false;
            m_db_curwrite = 0;
            fillBuffer(m_buf, m_bufLength / 2);
            m_db_ready[0] = true;
            if (!m_run_thread) break;

            m_db_curwrite = 1;
            m_db_ready[1] = false;
            fillBuffer(m_buf + m_bufLength / 2, m_bufLength / 2);
            m_db_ready[1] = true;
        }
    }

    // This is a blocking function that emulates an ADC writing to the
    // specified memory region. It takes the same time that an ADC would take
    // to sample the same quantity of data.
    bool fillBuffer(stream_sample_t* dest, size_t sz)
    {
        size_t i = 0;
        if (!m_run_thread) return false;

        assert(m_func_running == false);
        m_func_running = true;

        auto reset_func_running = [&]()
        {
            assert(m_func_running == true);
            m_func_running = false;
        };

        using std::chrono::microseconds;

        if (m_sampleRate > 0)
        {
            // Do a piecewise-sleep so that it's easily interruptible
            uint64_t microsec = sz * 1000000 / m_sampleRate;
            while (microsec > 10000)
            {
                if (!m_run_thread)
                {
                    // Early exit if the class is being deallocated
                    reset_func_running();
                    return false;
                }

                std::this_thread::sleep_for(microseconds(10000));
                microsec -= 10000;
            }
            std::this_thread::sleep_for(microseconds(microsec));
        }

        if (!m_run_thread)
        {
            // Early exit if the class is being deallocated
            reset_func_running();
            return false;
        }

        // Fill the buffer
        while (i < sz)
        {
            auto n = fread(dest + i, 2, sz - i, m_fp);
            if (n < (sz - i)) fseek(m_fp, 0, SEEK_SET);
            i += n;
        }

        assert(i == sz);
        reset_func_running();
        return true;
    }

    void stopThread()
    {
        m_run_thread = false;

        while (m_func_running)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (m_thread.joinable()) m_thread.join();
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
    auto it = gOpenStreams.find(source);
    if (it != gOpenStreams.end())
    {
        auto& inputStream = it->second;
        if (inputStream->priority() >= priority) return -1;

        inputStream->changeId();
        inputStream->setStreamData(priority, buf, bufLength, mode, sampleRate);

        return inputStream->id();
    }

    auto stream = std::make_unique<InputStream>(source, priority, buf,
                                                bufLength, mode, sampleRate);

    if (!stream->isValid()) return -1;

    const auto id = stream->id();

    // New stream, move it into the map
    gOpenStreams[source] = std::move(stream);

    return id;
}

dataBlock_t inputStream_getData(streamId id)
{
    InputStream* stream = nullptr;
    for (auto& i : gOpenStreams)
        if (i.second->id() == id)
        {
            stream = i.second.get();
            break;
        }

    if (stream == nullptr) return dataBlock_t{NULL, 0};

    return stream->getDataBlock();
}

void inputStream_stop(streamId id)
{
    AudioSource src;
    bool found = false;
    for (auto& i : gOpenStreams)
        if (i.second->id() == id)
        {
            found = true;
            src   = i.first;
            break;
        }

    if (!found) return;

    gOpenStreams.erase(src);
}
