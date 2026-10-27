/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include "core/audio_codec.h"

/**
 * Wait for the codec queue of a path to empty.
 *
 * @param path: audio path of the decoding operation.
 * @return true if the queue emptied within one second.
 */
static bool waitQueueEmpty(const pathId path)
{
    for (int i = 0; i < 1000; i++) {
        if (codec_queueEmpty(path))
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return false;
}

TEST_CASE("Codec queue empties as the decoded frames play", "[codec]")
{
    // The speaker is an SDL stream: the dummy driver plays it in real time
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    REQUIRE(SDL_InitSubSystem(SDL_INIT_AUDIO) == 0);

    codec_init();
    pathId path = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_RX);
    REQUIRE(path > 0);
    REQUIRE(codec_queueEmpty(path));
    REQUIRE(codec_startDecode(path));

    // Push past the queue size, so the decoder is paced by playback
    const uint8_t frame[8] = { 0 };
    for (int i = 0; i < 8; i++)
        REQUIRE(codec_pushFrame(frame, true) == 0);

    REQUIRE_FALSE(codec_queueEmpty(path));
    REQUIRE(codec_queueEmpty(path + 1));
    REQUIRE(waitQueueEmpty(path));

    // Frames left behind by a stop are not queued for that path anymore
    for (int i = 0; i < 4; i++)
        REQUIRE(codec_pushFrame(frame, false) == 0);

    codec_stop(path);
    REQUIRE(codec_queueEmpty(path));

    audioPath_release(path);
    codec_terminate();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
