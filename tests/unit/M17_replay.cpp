/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include "M17Replay.hpp"

// Reference voice baseband committed in 2021: 48 kHz, 26.6 s, generated with
// a software modulator and free of noise. The recording was cut before the
// end of the transmission, so there is no end-of-stream frame to check.
static const char *const ASSET = REPLAY_ASSET_DIR "/M17_test_baseband_dc.raw";

TEST_CASE("Reference M17 voice baseband decodes through the receive chain",
          "[m17][replay]")
{
    M17Replay replay(false, false);
    REQUIRE(replay.replay(ASSET, 48000));

    const M17Replay::Counts &c = replay.counts();
    REQUIRE(c.locks == 1);
    REQUIRE(c.lsfValid == 1);
    REQUIRE(c.lsfInvalid == 0);
    REQUIRE(c.streamFrames >= 660);
    REQUIRE(c.streamMissed == 0);
}

TEST_CASE("M17 voice transmission joined late is heard at every symbol phase",
          "[m17][replay][!shouldfail]")
{
    // Join 2 s in, after the LSF, at each sampling phase of a symbol. 615
    // frames remain; rebuilding the LSF from the LICH takes up to 12.
    for (int phase = 0; phase < 5; phase++) {
        M17Replay replay(false, false);
        REQUIRE(replay.replay(ASSET, 48000, 2.0 + phase / 24000.0));
        REQUIRE(replay.counts().streamWithLsf >= 603);
    }
}

TEST_CASE("Reference M17 voice baseband is rejected with inverted polarity",
          "[m17][replay]")
{
    M17Replay replay(true, false);
    REQUIRE(replay.replay(ASSET, 48000));

    const M17Replay::Counts &c = replay.counts();
    REQUIRE(c.lsfValid == 0);
    REQUIRE(c.streamFrames < 10);
}

TEST_CASE("M17 replay rejects an unsupported sample rate and a missing file",
          "[m17][replay]")
{
    M17Replay replay(false, false);
    REQUIRE_FALSE(replay.replay(ASSET, 44100));
    REQUIRE_FALSE(replay.replay(REPLAY_ASSET_DIR "/does_not_exist.raw", 48000));
}
