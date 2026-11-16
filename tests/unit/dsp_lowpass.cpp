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

/**
 * Level of a tone after the low pass decimator, relative to its input level.
 *
 * @param ratio: decimation ratio.
 * @param freq: tone frequency, in Hz.
 * @param amp: tone amplitude, full scale 32767.
 * @return output RMS over input RMS, in dB.
 */
static float toneGainDb(uint8_t ratio, float freq, float amp)
{
    struct lowPassDecimator lpd;
    REQUIRE(dsp_lowPassInit(&lpd, ratio) == 0);

    double sum = 0.0;
    size_t n = 0;
    for (size_t i = 0; i < 48000; i++) {
        const float t = static_cast<float>(i) / FS;
        int16_t sample = static_cast<int16_t>(
            std::lrint(amp * std::sin(2.0f * PI * freq * t)));

        // Skip the filter's start
        if ((!dsp_lowPassDecimate(&lpd, &sample)) || (i < 480))
            continue;

        const double y = sample;
        sum += y * y;
        n++;
    }

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

    return sample;
}

TEST_CASE("Low pass decimator supports ratios 5 and 6", "[dsp][lowpass]")
{
    struct lowPassDecimator lpd;

    REQUIRE(dsp_lowPassInit(&lpd, 5) == 0);
    REQUIRE(dsp_lowPassInit(&lpd, 6) == 0);
    REQUIRE(dsp_lowPassInit(&lpd, 4) == -ENOTSUP);
}

TEST_CASE("Low pass decimator outputs every ratio-th sample", "[dsp][lowpass]")
{
    for (uint8_t ratio = 5; ratio <= 6; ratio++) {
        struct lowPassDecimator lpd;
        REQUIRE(dsp_lowPassInit(&lpd, ratio) == 0);

        for (size_t i = 1; i <= 3 * ratio; i++) {
            int16_t sample = 1000;
            REQUIRE(dsp_lowPassDecimate(&lpd, &sample) == ((i % ratio) == 0));
        }
    }
}

TEST_CASE("Low pass decimator passes DC at unity gain", "[dsp][lowpass]")
{
    int16_t input[DSP_LOWPASS_MAX_TAPS];
    for (size_t i = 0; i < DSP_LOWPASS_MAX_TAPS; i++)
        input[i] = 1000;

    REQUIRE(std::abs(lastOutput(5, input, 40) - 1000) <= 1);
    REQUIRE(std::abs(lastOutput(6, input, 48) - 1000) <= 1);
}

TEST_CASE("Low pass decimator passes the band", "[dsp][lowpass]")
{
    SECTION("Ratio 6, voice")
    {
        REQUIRE(std::fabs(toneGainDb(6, 1000.0f, 10000.0f)) < 0.5f);
        REQUIRE(toneGainDb(6, 3000.0f, 10000.0f) > -3.0f);
    }

    SECTION("Ratio 5, AFSK tones")
    {
        REQUIRE(std::fabs(toneGainDb(5, 1200.0f, 10000.0f)) < 0.5f);
        REQUIRE(std::fabs(toneGainDb(5, 2200.0f, 10000.0f)) < 0.5f);
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

    SECTION("Ratio 5, 9.6 kHz output")
    {
        // 6 kHz is in the transition band: about -24 dB by design
        REQUIRE(toneGainDb(5, 6000.0f, 10000.0f) < -20.0f);
        REQUIRE(toneGainDb(5, 7200.0f, 10000.0f) < -60.0f);
        REQUIRE(toneGainDb(5, 8400.0f, 10000.0f) < -60.0f);
    }
}

TEST_CASE("Low pass decimator saturates full scale input without wrapping",
          "[dsp][lowpass]")
{
    // Signs matching the taps': the largest output the input range allows
    static constexpr int8_t SIGNS5[40] = {
        -1, -1, -1, -1, 1, 1, 1, 1, 1,  -1, -1, -1, -1, -1,
        -1, 1,  1,  1,  1, 1, 1, 1, 1,  1,  1,  -1, -1, -1,
        -1, -1, -1, 1,  1, 1, 1, 1, -1, -1, -1, -1,
    };
    static constexpr int8_t SIGNS6[48] = {
        -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1,
        -1, -1, 1,  1,  1,  1, 1, 1, 1, 1, 1, 1,  1,  1,  -1, -1,
        -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1,
    };

    int16_t input[DSP_LOWPASS_MAX_TAPS];

    for (size_t i = 0; i < 40; i++)
        input[i] = static_cast<int16_t>(SIGNS5[i] * 32767);
    REQUIRE(lastOutput(5, input, 40) == INT16_MAX);

    for (size_t i = 0; i < 48; i++)
        input[i] = static_cast<int16_t>(SIGNS6[i] * 32767);
    REQUIRE(lastOutput(6, input, 48) == INT16_MAX);
}
