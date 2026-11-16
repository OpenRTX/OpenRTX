/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>

#include "core/dsp.h"

using Catch::Matchers::WithinAbs;

static constexpr float FS = 8000.0f;
static constexpr size_t BLK = 160;
static constexpr float TARGET = -18.0f;
static constexpr float PI = 3.14159265f;
// Base gain of the samples, +18 dB: the levels below are after it, so the
// range from -40 to +10 dBFS fits in int16 samples
static constexpr float BASE = 8.0f / 32768.0f;

/**
 * RMS level of output samples.
 *
 * @param x: samples.
 * @param n: number of samples.
 * @return level in dBFS.
 */
static float rmsDb(const int16_t *x, size_t n)
{
    double p = 0.0;
    for (size_t i = 0; i < n; i++) {
        const double v = (double)x[i] / 32767.0;
        p += v * v;
    }
    return (float)(10.0 * std::log10((p / (double)n) + 1e-12));
}

/**
 * Run a sine through the AGC.
 *
 * @param agc: AGC state.
 * @param levelDb: sine level after the base gain, dBFS RMS.
 * @param blocks: number of blocks.
 * @param phase: sample counter, updated.
 * @param peak: largest output magnitude, full scale 1.0, written.
 * @return level of the last output block, dBFS RMS.
 */
static float runTone(struct agc *agc, float levelDb, size_t blocks,
                     size_t *phase, float *peak)
{
    const float amp = std::sqrt(2.0f) * std::pow(10.0f, levelDb / 20.0f) / BASE;
    int16_t buf[BLK];
    *peak = 0.0f;
    for (size_t b = 0; b < blocks; b++) {
        for (size_t i = 0; i < BLK; i++, (*phase)++) {
            const float t = (float)(*phase) / FS;
            buf[i] =
                (int16_t)std::lrint(amp * std::sin(2.0f * PI * 440.0f * t));
        }
        dsp_agcProcess(agc, buf, BLK);
        for (size_t i = 0; i < BLK; i++)
            *peak = std::max(*peak, std::fabs((float)buf[i] / 32767.0f));
    }
    return rmsDb(buf, BLK);
}

/**
 * Run uniform noise through the AGC.
 *
 * @param agc: AGC state.
 * @param levelDb: noise level after the base gain, dBFS RMS.
 * @param blocks: number of blocks.
 * @param seed: generator state, updated.
 */
static void runNoise(struct agc *agc, float levelDb, size_t blocks,
                     uint32_t *seed)
{
    const float amp = std::sqrt(3.0f) * std::pow(10.0f, levelDb / 20.0f) / BASE;
    int16_t buf[BLK];
    for (size_t b = 0; b < blocks; b++) {
        for (size_t i = 0; i < BLK; i++) {
            *seed = (*seed * 1664525u) + 1013904223u;
            const float u = ((float)(*seed >> 8) / 8388608.0f) - 1.0f;
            buf[i] = (int16_t)std::lrint(amp * u);
        }
        dsp_agcProcess(agc, buf, BLK);
    }
}

/**
 * Run a talker through the AGC: 300 ms words of a sine, 200 ms gaps of quiet
 * noise. A steady tone without gaps is the noise floor to the AGC.
 *
 * @param agc: AGC state.
 * @param levelDb: word level after the base gain, dBFS RMS.
 * @param words: number of words.
 * @param phase: sample counter, updated.
 * @param seed: noise generator state, updated.
 * @return level of the last word's last block, dBFS RMS.
 */
static float runTalker(struct agc *agc, float levelDb, size_t words,
                       size_t *phase, uint32_t *seed)
{
    float peak;
    float out = -120.0f;
    for (size_t w = 0; w < words; w++) {
        out = runTone(agc, levelDb, 15, phase, &peak);
        runNoise(agc, -80.0f, 10, seed);
    }
    return out;
}

TEST_CASE("AGC brings speech of any level to the target", "[dsp][agc]")
{
    // +10 dBFS: the base gain is applied without clipping, so loud input
    // arrives over full scale
    for (float level : { -40.0f, -30.0f, -18.0f, -6.0f, 10.0f }) {
        struct agc agc;
        dsp_agcInit(&agc, TARGET, BASE, 0.0f);

        size_t phase = 0;
        uint32_t seed = 1;
        const float out = runTalker(&agc, level, 24, &phase, &seed); // 12 s
        REQUIRE_THAT(out, WithinAbs(TARGET, 1.5));
    }
}

TEST_CASE("AGC limits every sample under -1 dBFS", "[dsp][agc]")
{
    struct agc agc;
    dsp_agcInit(&agc, TARGET, BASE, 0.0f);

    // A sudden full-scale burst after settling on a quiet talker
    float peak;
    size_t phase = 0;
    uint32_t seed = 2;
    runTalker(&agc, -40.0f, 16, &phase, &seed);
    REQUIRE(agc.gainDb > 15.0f);
    runTone(&agc, 0.0f, 20, &phase, &peak);
    REQUIRE(peak <= std::pow(10.0f, -1.0f / 20.0f) + 1e-4f);
}

TEST_CASE("AGC holds the gain through a pause", "[dsp][agc]")
{
    struct agc agc;
    dsp_agcInit(&agc, TARGET, BASE, 0.0f);

    size_t phase = 0;
    uint32_t seed = 1;
    runTalker(&agc, -35.0f, 20, &phase, &seed);
    const float settled = agc.gainDb;
    REQUIRE(settled > 10.0f);

    runNoise(&agc, -70.0f, 250, &seed); // 5 s of quiet background
    REQUIRE_THAT(agc.gainDb, WithinAbs(settled, 0.01));
}

TEST_CASE("AGC turns digital silence into silence", "[dsp][agc]")
{
    struct agc agc;
    dsp_agcInit(&agc, TARGET, BASE, 0.0f);

    int16_t buf[BLK] = { 0 };
    for (size_t b = 0; b < 100; b++)
        dsp_agcProcess(&agc, buf, BLK);

    for (size_t i = 0; i < BLK; i++)
        REQUIRE(buf[i] == 0);
    REQUIRE(std::isfinite(agc.gainDb));
}

TEST_CASE("AGC finds a loud noise floor within its window", "[dsp][agc]")
{
    struct agc agc;
    dsp_agcInit(&agc, TARGET, BASE, 0.0f);

    float peak;
    size_t phase = 0;
    uint32_t seed = 3;
    runTone(&agc, -20.0f, 50, &phase, &peak);
    runNoise(&agc, -40.0f, 100, &seed);

    REQUIRE(agc.floorDb > -45.0f);
    REQUIRE(agc.floorDb < -39.0f);
}

TEST_CASE("AGC does not raise the gain on short noise bursts", "[dsp][agc]")
{
    struct agc agc;
    dsp_agcInit(&agc, TARGET, BASE, 0.0f);

    // Settle on a talker over quiet noise, then 10 s of 80 ms bursts
    size_t phase = 0;
    uint32_t seed = 5;
    runTalker(&agc, -20.0f, 10, &phase, &seed);
    runNoise(&agc, -50.0f, 100, &seed);
    const float settled = agc.gainDb;
    for (size_t i = 0; i < 20; i++) {
        runNoise(&agc, -50.0f, 21, &seed);
        runNoise(&agc, -32.0f, 4, &seed);
    }

    REQUIRE(std::fabs(agc.gainDb - settled) < 0.5f);
}

TEST_CASE("AGC bounds the gain rise when a noise bed starts", "[dsp][agc]")
{
    struct agc agc;
    dsp_agcInit(&agc, TARGET, BASE, 0.0f);

    // Until the floor window forgets the quiet gaps, about 1.6 s, the new
    // noise reads as speech: the gain may creep up, then holds
    size_t phase = 0;
    uint32_t seed = 7;
    runTalker(&agc, -20.0f, 10, &phase, &seed);
    const float settled = agc.gainDb;
    runNoise(&agc, -50.0f, 100, &seed);
    const float risen = agc.gainDb - settled;
    runNoise(&agc, -50.0f, 250, &seed);

    REQUIRE(risen < 7.0f);
    REQUIRE_THAT(agc.gainDb - settled, WithinAbs(risen, 0.01));
}

TEST_CASE("AGC finds the noise floor again at each transmission", "[dsp][agc]")
{
    struct agc agc;
    dsp_agcInit(&agc, TARGET, BASE, 0.0f);

    // Settle on a talker in a quiet room, then key up again in a louder one
    // with nobody talking yet: the louder noise is not speech, the gain holds
    size_t phase = 0;
    uint32_t seed = 11;
    runTalker(&agc, -20.0f, 10, &phase, &seed);
    const float settled = agc.gainDb;
    dsp_agcRestart(&agc);
    runNoise(&agc, -45.0f, 100, &seed);

    REQUIRE(std::fabs(agc.gainDb - settled) < 0.5f);
}

TEST_CASE("AGC turns down a loud talker from the first block after a restart",
          "[dsp][agc]")
{
    struct agc agc;
    dsp_agcInit(&agc, TARGET, BASE, 0.0f);

    // A quiet talker leaves a high gain; the next over starts loud at once,
    // with no pause yet to show the noise floor
    size_t phase = 0;
    uint32_t seed = 13;
    float peak;
    runTalker(&agc, -40.0f, 20, &phase, &seed);
    const float settled = agc.gainDb;
    REQUIRE(settled > 10.0f);
    dsp_agcRestart(&agc);
    runTone(&agc, -8.0f, 10, &phase, &peak);

    // 1.5 dB a block for 10 blocks
    REQUIRE(agc.gainDb < settled - 10.0f);
}

TEST_CASE("AGC holds a starting gain until speech says otherwise", "[dsp][agc]")
{
    // A start of +12 dB: a talker who needs about that is at the target
    // from the first word, and a loud one is turned down at once
    size_t phase = 0;
    uint32_t seed = 17;
    float peak;
    struct agc quiet;
    dsp_agcInit(&quiet, TARGET, BASE, 12.0f);
    const float first = runTalker(&quiet, TARGET - 12.0f, 2, &phase, &seed);
    REQUIRE_THAT(first, WithinAbs(TARGET, 1.5));
    REQUIRE_THAT(quiet.gainDb, WithinAbs(12.0f, 1.5));

    struct agc loud;
    dsp_agcInit(&loud, TARGET, BASE, 12.0f);
    runTone(&loud, -6.0f, 10, &phase, &peak);
    REQUIRE(loud.gainDb < 0.0f);
}
