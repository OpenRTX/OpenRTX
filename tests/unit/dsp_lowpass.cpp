/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include <cerrno>
#include <cmath>
#include <cstdlib>

#include "core/dsp.h"

static constexpr float FS = 48000.0f;
static constexpr float PI = 3.14159265f;

// Taps of the longest filter, ratio 8
static constexpr size_t MAX_TAPS = 64;

/**
 * Level of a tone after the low pass decimator, relative to its input level.
 *
 * @param ratio: decimation ratio.
 * @param freq: tone frequency, in Hz.
 * @param amp: tone amplitude, full scale 32767.
 * @param fs: input sample rate, in Hz.
 * @return output RMS over input RMS, in dB.
 */
static float toneGainDb(uint8_t ratio, float freq, float amp, float fs = FS)
{
    struct lowPassDecimator lpd;
    REQUIRE(dsp_lowPassInit(&lpd, ratio) == 0);

    double sum = 0.0;
    size_t n = 0;
    for (size_t i = 0; i < 48000; i++) {
        const float t = static_cast<float>(i) / fs;
        int16_t sample = static_cast<int16_t>(
            std::lrint(amp * std::sin(2.0f * PI * freq * t)));

        // Skip the filter's start
        if ((!dsp_lowPassDecimate(&lpd, &sample)) || (i < 480))
            continue;

        const double y = sample;
        sum += y * y;
        n++;
    }

    dsp_lowPassTerminate(&lpd);

    const double rms = std::sqrt(sum / static_cast<double>(n));
    return static_cast<float>(
        20.0 * std::log10(rms / (amp / std::sqrt(2.0)) + 1e-12));
}

/**
 * Last output of the low pass decimator after a run of input samples.
 *
 * @param ratio: decimation ratio.
 * @param input: input samples, a multiple of ratio long.
 * @param len: number of input samples.
 * @return the last decimated sample.
 */
static int16_t lastOutput(uint8_t ratio, const int16_t *input, size_t len)
{
    struct lowPassDecimator lpd;
    REQUIRE(dsp_lowPassInit(&lpd, ratio) == 0);

    int16_t sample = 0;
    for (size_t i = 0; i < len; i++) {
        sample = input[i];
        dsp_lowPassDecimate(&lpd, &sample);
    }

    dsp_lowPassTerminate(&lpd);

    return sample;
}

TEST_CASE("Low pass decimator supports ratios 6 and 8", "[dsp][lowpass]")
{
    struct lowPassDecimator lpd;

    for (uint8_t ratio : { 6, 8 }) {
        REQUIRE(dsp_lowPassInit(&lpd, ratio) == 0);
        dsp_lowPassTerminate(&lpd);
    }

    REQUIRE(dsp_lowPassInit(&lpd, 4) == -ENOTSUP);
}

TEST_CASE("Low pass decimator is safe to terminate when not set up",
          "[dsp][lowpass]")
{
    struct lowPassDecimator lpd;
    REQUIRE(dsp_lowPassInit(&lpd, 4) == -ENOTSUP);

    int16_t sample = 1000;
    REQUIRE(dsp_lowPassDecimate(&lpd, &sample) == false);
    dsp_lowPassTerminate(&lpd);

    REQUIRE(dsp_lowPassInit(&lpd, 6) == 0);
    dsp_lowPassTerminate(&lpd);
    dsp_lowPassTerminate(&lpd);
    REQUIRE(dsp_lowPassDecimate(&lpd, &sample) == false);
}

TEST_CASE("Low pass decimator outputs every ratio-th sample", "[dsp][lowpass]")
{
    for (uint8_t ratio : { 6, 8 }) {
        struct lowPassDecimator lpd;
        REQUIRE(dsp_lowPassInit(&lpd, ratio) == 0);

        for (size_t i = 1; i <= 3 * ratio; i++) {
            int16_t sample = 1000;
            REQUIRE(dsp_lowPassDecimate(&lpd, &sample) == ((i % ratio) == 0));
        }

        dsp_lowPassTerminate(&lpd);
    }
}

TEST_CASE("Low pass decimator passes DC at unity gain", "[dsp][lowpass]")
{
    int16_t input[MAX_TAPS];
    for (size_t i = 0; i < MAX_TAPS; i++)
        input[i] = 1000;

    REQUIRE(std::abs(lastOutput(6, input, 48) - 1000) <= 1);
    REQUIRE(std::abs(lastOutput(8, input, 64) - 1000) <= 1);
}

TEST_CASE("Low pass decimator starts settled on a DC level", "[dsp][lowpass]")
{
    // A mic ADC idles at mid-scale: the first output must already be there,
    // or the DC block after the decimator sees a step at every key-up
    for (uint8_t ratio : { 6, 8 }) {
        struct lowPassDecimator lpd;
        REQUIRE(dsp_lowPassInit(&lpd, ratio) == 0);

        for (size_t i = 0; i < 4 * MAX_TAPS; i++) {
            int16_t sample = 16384;
            if (dsp_lowPassDecimate(&lpd, &sample))
                REQUIRE(std::abs(sample - 16384) <= 1);
        }

        dsp_lowPassTerminate(&lpd);
    }
}

TEST_CASE("Low pass decimator passes the band", "[dsp][lowpass]")
{
    SECTION("Ratio 6, voice")
    {
        REQUIRE(std::fabs(toneGainDb(6, 1000.0f, 10000.0f)) < 0.5f);
        REQUIRE(toneGainDb(6, 3000.0f, 10000.0f) > -3.0f);
    }

    SECTION("Ratio 8, voice from 64 kHz")
    {
        REQUIRE(std::fabs(toneGainDb(8, 1000.0f, 10000.0f, 64000.0f)) < 0.5f);
        REQUIRE(toneGainDb(8, 3000.0f, 10000.0f, 64000.0f) > -3.0f);
    }
}

TEST_CASE("Low pass decimator rejects tones that would alias", "[dsp][lowpass]")
{
    SECTION("Ratio 6, 8 kHz output")
    {
        // 5 kHz is in the transition band: about -24 dB by design
        REQUIRE(toneGainDb(6, 5000.0f, 10000.0f) < -20.0f);
        REQUIRE(toneGainDb(6, 6000.0f, 10000.0f) < -60.0f);
        REQUIRE(toneGainDb(6, 7000.0f, 10000.0f) < -60.0f);
    }

    SECTION("Ratio 8, 8 kHz output from 64 kHz")
    {
        // 5 kHz is in the transition band: about -24 dB by design
        REQUIRE(toneGainDb(8, 5000.0f, 10000.0f, 64000.0f) < -20.0f);
        REQUIRE(toneGainDb(8, 6000.0f, 10000.0f, 64000.0f) < -60.0f);
        REQUIRE(toneGainDb(8, 7000.0f, 10000.0f, 64000.0f) < -60.0f);
    }
}

TEST_CASE("Low pass decimator saturates full scale input without wrapping",
          "[dsp][lowpass]")
{
    // Signs matching the taps': the largest output the input range allows
    static constexpr int8_t SIGNS6[48] = {
        -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1,
        -1, -1, 1,  1,  1,  1, 1, 1, 1, 1, 1, 1,  1,  1,  -1, -1,
        -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1,
    };

    static constexpr int8_t SIGNS8[64] = {
        -1, -1, -1, -1, -1, -1, 1,  1, 1, 1,  1,  1,  1,  1,  1,  -1,
        -1, -1, -1, -1, -1, -1, -1, 1, 1, 1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1, 1, -1, -1, -1, -1, -1, -1, -1,
        -1, 1,  1,  1,  1,  1,  1,  1, 1, 1,  -1, -1, -1, -1, -1, -1,
    };

    int16_t input[MAX_TAPS];

    for (size_t i = 0; i < 48; i++)
        input[i] = static_cast<int16_t>(SIGNS6[i] * 32767);
    REQUIRE(lastOutput(6, input, 48) == INT16_MAX);

    for (size_t i = 0; i < 64; i++)
        input[i] = static_cast<int16_t>(SIGNS8[i] * 32767);
    REQUIRE(lastOutput(8, input, 64) == INT16_MAX);
}
