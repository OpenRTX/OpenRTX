/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <string.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

#include "core/audio_stream.h"

static const char* files[] = {"MIC.raw", "RTX.raw", "MCU.raw"};

static void test_linear()
{
    for (int fn = 0; fn < 3; fn++)
    {
        // Write a mock audio file
        FILE* fp = fopen(files[fn], "wb");
        REQUIRE(fp);
        for (int i = 0; i < 13; i++)
        {
            REQUIRE(fputc(i, fp) == i);
            REQUIRE(fputc(0, fp) == 0);
        }
        fclose(fp);

        // Start inputStream using that file as source
        stream_sample_t tmp[128];
        auto id = inputStream_start(
            static_cast<AudioSource>(fn), AudioPriority::PRIO_PROMPT, tmp,
            sizeof(tmp) / sizeof(tmp[0]), BufMode::BUF_LINEAR, 44100);
        REQUIRE(id != -1);

        // Should fail
        auto id_2 = inputStream_start(
            static_cast<AudioSource>(fn), AudioPriority::PRIO_BEEP, tmp,
            sizeof(tmp) / sizeof(tmp[0]), BufMode::BUF_LINEAR, 44100);
        REQUIRE(id_2 == -1);

        auto id_3 = inputStream_start(
            static_cast<AudioSource>(fn), AudioPriority::PRIO_RX, tmp,
            sizeof(tmp) / sizeof(tmp[0]), BufMode::BUF_LINEAR, 44100);
        REQUIRE(id_3 == -1);

        // This should work, as it has higher priority
        auto id_4 = inputStream_start(
            static_cast<AudioSource>(fn), AudioPriority::PRIO_TX, tmp,
            sizeof(tmp) / sizeof(tmp[0]), BufMode::BUF_LINEAR, 44100);
        REQUIRE(id_4 != -1);

        {
            // id is now invalid (overridden by id_4), getData should fail
            auto should_fail = inputStream_getData(id);
            REQUIRE(should_fail.data == NULL);
            REQUIRE(should_fail.len == 0);
        }

        // id_4 is now the only valid pointer, should work
        id = id_4;

        int c_ptr = 0;
        // getData multiple times
        for (int i = 0; i < 20; i++)
        {
            using namespace std::chrono;
            auto t1              = steady_clock::now();
            auto db              = inputStream_getData(id);
            auto t2              = steady_clock::now();
            const uint64_t delta = duration_cast<microseconds>(t2 - t1).count();
            const uint64_t expected = (128 * 1000000 / 44100);

            // Check that the getData() sleeps the right amount of time
            REQUIRE((delta > expected));
            REQUIRE(delta < (expected + 10000));

            // Check the contents
            REQUIRE(db.len == 128);
            for (int i = 0; i < 128; i++)
            {
                REQUIRE(tmp[i] == (c_ptr % 13));
                REQUIRE(db.data[i] == (c_ptr % 13));
                c_ptr++;
            }
        }

        // Close inputStream and remove the temporary file
        inputStream_stop(id);
        REQUIRE(remove(files[fn]) == 0);
    }
}

static void test_ring_buffer(uint64_t n_bytes,
                              uint64_t n_iter,
                              const uint64_t buf_size)
{
    for (int fn = 0; fn < 3; fn++)
    {
        FILE* fp = fopen(files[fn], "wb");
        REQUIRE(fp);

        for (uint64_t i = 0; i < n_bytes; i++)
        {
            uint16_t j   = i;
            uint8_t* buf = (uint8_t*)&j;
            REQUIRE(fputc(buf[0], fp) == buf[0]);
            REQUIRE(fputc(buf[1], fp) == buf[1]);
        }
        fclose(fp);

        std::vector<stream_sample_t> tmp(buf_size);
        auto id = inputStream_start(
            static_cast<AudioSource>(fn), AudioPriority::PRIO_BEEP, tmp.data(),
            tmp.size(), BufMode::BUF_CIRC_DOUBLE, 44100);
        REQUIRE(id != -1);

        using namespace std::chrono;
        time_point<steady_clock> t0 = steady_clock::now();

        uint64_t ctr = 0;
        std::vector<stream_sample_t> tmp2(buf_size / 2);
        for (uint64_t i = 0; i < n_iter; i++)
        {
            {
                auto db = inputStream_getData(id);

                REQUIRE(db.len > 0);
                REQUIRE(db.data == &tmp[0]);
                memcpy(tmp2.data(), db.data, db.len * sizeof(stream_sample_t));
                for (uint64_t i = 0; i < db.len; i++)
                {
                    REQUIRE(uint16_t(tmp2[i]) == uint16_t(ctr % n_bytes));
                    ctr++;
                }
            }

            {
                auto db = inputStream_getData(id);

                REQUIRE(db.len > 0);
                REQUIRE(db.data == &tmp[buf_size / 2]);
                memcpy(tmp2.data(), db.data, db.len * sizeof(stream_sample_t));
                for (uint64_t i = 0; i < db.len; i++)
                {
                    REQUIRE(uint16_t(tmp2[i]) == uint16_t(ctr % n_bytes));
                    ctr++;
                }
            }
        }

        auto t2                 = steady_clock::now();
        const uint64_t delta    = duration_cast<microseconds>(t2 - t0).count();
        const uint64_t expected = (buf_size * n_iter * 1000000lu / 44100);
        REQUIRE(delta > expected);
        REQUIRE(delta < expected * 2);

        inputStream_stop(id);
        REQUIRE(remove(files[fn]) == 0);
    }
}

TEST_CASE("InputStream linear mode", "[audio]")
{
    test_linear();
}

TEST_CASE("InputStream ring buffer mode", "[audio]")
{
    SECTION("13 bytes, 10 iterations, buffer 256")
    {
        test_ring_buffer(13, 10, 256);
    }
    SECTION("128 bytes, 10 iterations, buffer 256")
    {
        test_ring_buffer(128, 10, 256);
    }
    SECTION("256 bytes, 10 iterations, buffer 128")
    {
        test_ring_buffer(256, 10, 128);
    }
    SECTION("1234 bytes, 10 iterations, buffer 768")
    {
        test_ring_buffer(1234, 10, 768);
    }
}
