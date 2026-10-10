/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

#include "M17Replay.hpp"
#include "M17Signal.hpp"

using namespace M17;
using namespace M17Signal;

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
          "[m17][replay]")
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

// Replay the channel through the receive chain.
static M17Replay::Counts receive(const Channel &channel)
{
    M17Replay replay(false, false);
    replay.replay(channel.samples().data(), channel.samples().size(),
                  channel.rate());
    return replay.counts();
}

// The first clock recovery update after a lock sets the sampling point of
// the second frame; it must ignore what was heard before the lock. Seeds
// avoid transmissions starting inside a false lock on the noise.

TEST_CASE("M17 SMS sent after idle channel noise is received", "[m17][replay]")
{
    // 2 to 9 s of noise before each SMS.
    static constexpr uint32_t TRIALS = 24;

    for (size_t numFrames : { 4, 12 }) {
        const uint32_t seed = (numFrames == 4) ? 12300 : 22300;
        uint32_t received = 0;

        for (uint32_t trial = 0; trial < TRIALS; trial++) {
            M17Channel channel(seed + trial);
            channel.idle(2.0 + (trial % 8));
            channel.transmit(m17Symbols(smsFrames(numFrames, 'a' + trial)),
                             2 * (trial % 5));
            channel.idle(0.05);

            M17Replay::Counts c = receive(channel);
            if ((c.packetsOk == 1) && (c.packetsCrc == 0)
                && (c.packetsAborted == 0))
                received++;
        }

        CAPTURE(numFrames);
        CHECK(received == TRIALS);
    }
}

TEST_CASE("M17 SMS is received after a transmission with other symbol timing",
          "[m17][replay]")
{
    // Another 4FSK mode, 0.2 s of noise, then an SMS half a symbol off.
    static constexpr uint32_t TRIALS = 20;
    uint32_t received = 0;

    for (uint32_t trial = 0; trial < TRIALS; trial++) {
        const size_t phase = (2 * (trial % 5)) + 1;
        M17Channel channel(32300 + trial);
        channel.idle(0.1);
        channel.transmit(otherSymbols(33300 + trial, 1.0 + (trial % 4) * 0.5),
                         phase);
        channel.idle(0.2);
        channel.transmit(m17Symbols(smsFrames(4, 'a' + trial)),
                         (phase + 5) % M17Channel::SAMPLES_PER_SYMBOL);
        channel.idle(0.05);

        M17Replay::Counts c = receive(channel);
        if ((c.packetsOk == 1) && (c.packetsCrc == 0)
            && (c.packetsAborted == 0))
            received++;
    }

    REQUIRE(received == TRIALS);
}

TEST_CASE("M17 voice transmission after idle channel noise is heard whole",
          "[m17][replay]")
{
    // 2 to 9 s of noise, then every stream frame must be decoded.
    static constexpr uint32_t TRIALS = 24;
    static constexpr size_t FRAMES = 10;
    const std::vector<int8_t> symbols = m17Symbols(voiceFrames(FRAMES));
    uint32_t heard = 0;

    for (uint32_t trial = 0; trial < TRIALS; trial++) {
        M17Channel channel(42300 + trial);
        channel.idle(2.0 + (trial % 8));
        channel.transmit(symbols, 2 * (trial % 5));
        channel.idle(0.05);

        M17Replay::Counts c = receive(channel);
        if ((c.lsfValid == 1) && ((c.streamFrames - c.streamRejected) == FRAMES)
            && (c.streamMissed == 0))
            heard++;
    }

    REQUIRE(heard == TRIALS);
}

TEST_CASE("M17 voice transmission joined after idle channel noise is heard",
          "[m17][replay]")
{
    // After noise, the transmission starts 100 symbols into its sixth stream
    // frame: every frame from the seventh must be decoded.
    static constexpr uint32_t TRIALS = 24;
    static constexpr size_t FRAMES = 30;
    static constexpr size_t HEARD = FRAMES - 6;
    const std::vector<int8_t> symbols = m17Symbols(voiceFrames(FRAMES));
    const size_t join = (7 * FRAME_SYMBOLS) + 100;
    uint32_t heard = 0;

    for (uint32_t trial = 0; trial < TRIALS; trial++) {
        M17Channel channel(52400 + trial);
        channel.idle(2.0 + (trial % 8));
        channel.transmit(std::vector<int8_t>(symbols.begin() + join,
                                             symbols.end()),
                         2 * (trial % 5));
        channel.idle(0.05);

        M17Replay::Counts c = receive(channel);
        if (((c.streamFrames - c.streamRejected) == HEARD)
            && (c.streamMissed == 0))
            heard++;
    }

    REQUIRE(heard == TRIALS);
}
