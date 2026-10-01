/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include "../platform/m17_replay.hpp"

// Replay the reference voice baseband committed in 2021 (48 kHz, 26.6 s,
// generated with a software modulator and no noise) through the receive
// chain. The recording was cut before the end of the transmission, so there
// is no end-of-stream frame to check.
TEST_CASE("Reference M17 voice baseband decodes through the receive chain",
          "[m17][replay]")
{
    replay::Options opt;
    opt.path = REPLAY_ASSET_DIR "/M17_test_baseband_dc.raw";
    opt.rate = 48000;

    SECTION("correct polarity")
    {
        replay::Counts count;
        REQUIRE(replay::run(opt, count));
        CHECK(count["locks"] == 1);
        CHECK(count["lsf_valid"] == 1);
        CHECK(count["lsf_invalid"] == 0);
        CHECK(count["stream_frames"] >= 660);
        CHECK(count["stream_missed"] == 0);
    }

    SECTION("inverted polarity is rejected")
    {
        opt.invert = true;
        replay::Counts count;
        REQUIRE(replay::run(opt, count));
        CHECK(count["lsf_valid"] == 0);
        CHECK(count["stream_frames"] < 10);
    }
}
