/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "protocols/M17/Correlator.hpp"
#include "protocols/M17/Constants.hpp"
#include "protocols/M17/DSP.hpp"
#include "protocols/M17/Synchronizer.hpp"
#include "core/fir.hpp"

// M17 demodulation constants
static constexpr size_t SAMPLES_PER_SYM = 5; // 24000 Hz / 4800 baud
static constexpr size_t N = M17::SYNCWORD_SYMBOLS;
static constexpr int32_t SCALE = 1000;

// M17 stream syncword symbols as used in Demodulator
static constexpr std::array<int8_t, N> STREAM_SYNC = { -3, -3, -3, -3,
                                                       +3, +3, -3, +3 };

// Prime the correlator with zeros to fill its internal history
static void primeCorrelator(Correlator<N, SAMPLES_PER_SYM> &corr)
{
    for (size_t i = 0; i < N * SAMPLES_PER_SYM; i++)
        corr.sample(0);
}

// Feed one full syncword pattern (each symbol repeated SAMPLES_PER_SYM times)
static void feedSyncword(Correlator<N, SAMPLES_PER_SYM> &corr,
                         const std::array<int8_t, N> &syncword)
{
    for (auto sym : syncword)
        for (size_t s = 0; s < SAMPLES_PER_SYM; s++)
            corr.sample(static_cast<int16_t>(sym * SCALE));
}

TEST_CASE("Correlator peaks at maximum for matching M17 stream syncword",
          "[m17][demodulator]")
{
    // Maximum possible correlation: num_symbols * amplitude^2 * SCALE
    // symbol amplitude is 3, so 8 * 9 * 1000 = 72000
    static constexpr int32_t PEAK_CORR = static_cast<int32_t>(N) * 9 * SCALE;

    Correlator<N, SAMPLES_PER_SYM> corr;
    primeCorrelator(corr);
    feedSyncword(corr, STREAM_SYNC);

    int32_t conv = corr.convolve(STREAM_SYNC);
    REQUIRE(conv == PEAK_CORR);
}

TEST_CASE("Correlator convolution is proportional to syncword sum for DC signal",
          "[m17][demodulator]")
{
    // The expected convolution against a constant-value signal equals the sum
    // of all syncword symbols multiplied by the signal value.
    // STREAM_SYNC = {-3,-3,-3,-3,+3,+3,-3,+3}, sum = -6.
    static constexpr int32_t SYNC_SUM = -6;
    static constexpr int32_t EXPECTED = SYNC_SUM * SCALE;

    Correlator<N, SAMPLES_PER_SYM> corr;

    // Fill with constant positive value
    for (size_t i = 0; i < N * SAMPLES_PER_SYM * 2; i++)
        corr.sample(SCALE);

    int32_t conv = corr.convolve(STREAM_SYNC);
    REQUIRE(conv == EXPECTED);
}

TEST_CASE(
    "Synchronizer detects M17 stream syncword and returns valid sampling point",
    "[m17][demodulator]")
{
    static constexpr int32_t PEAK_CORR = static_cast<int32_t>(N) * 9 * SCALE;
    static constexpr int32_t THRESHOLD = PEAK_CORR / 2;

    Correlator<N, SAMPLES_PER_SYM> corr;
    Synchronizer<N, SAMPLES_PER_SYM> sync{ std::array<int8_t, N>{
        -3, -3, -3, -3, +3, +3, -3, +3 } };

    primeCorrelator(corr);
    for (size_t i = 0; i < N * SAMPLES_PER_SYM; i++)
        sync.update(corr, THRESHOLD, -THRESHOLD);

    feedSyncword(corr, STREAM_SYNC);
    for (size_t i = 0; i < N * SAMPLES_PER_SYM; i++)
        sync.update(corr, THRESHOLD, -THRESHOLD);

    // Feed zeros to let the trigger window fall and produce a detection
    bool detected = false;
    for (size_t i = 0; i < N * SAMPLES_PER_SYM && !detected; i++) {
        corr.sample(0);
        int8_t r = sync.update(corr, THRESHOLD, -THRESHOLD);
        if (r != 0)
            detected = true;
    }

    REQUIRE(detected);
    REQUIRE(sync.samplingIndex() < SAMPLES_PER_SYM);
}

TEST_CASE(
    "Synchronizer does not trigger when threshold exceeds maximum correlation",
    "[m17][demodulator]")
{
    // A threshold higher than the theoretical maximum should never trigger
    static constexpr int32_t PEAK_CORR = static_cast<int32_t>(N) * 9 * SCALE;
    static constexpr int32_t HIGH_THRESHOLD = PEAK_CORR + 1;

    Correlator<N, SAMPLES_PER_SYM> corr;
    Synchronizer<N, SAMPLES_PER_SYM> sync{ std::array<int8_t, N>{
        -3, -3, -3, -3, +3, +3, -3, +3 } };

    primeCorrelator(corr);
    feedSyncword(corr, STREAM_SYNC);

    bool triggered = false;
    for (size_t i = 0; i < N * SAMPLES_PER_SYM; i++) {
        corr.sample(0);
        int8_t r = sync.update(corr, HIGH_THRESHOLD, -HIGH_THRESHOLD);
        if (r != 0)
            triggered = true;
    }

    REQUIRE_FALSE(triggered);
}

TEST_CASE("RRC 24kHz filter impulse response matches tap coefficients",
          "[m17][demodulator]")
{
    constexpr size_t NTAPS = M17::rrc_taps_24k.size();
    Fir<NTAPS> rrc(M17::rrc_taps_24k);
    std::array<float, NTAPS> output{};

    output[0] = rrc(1.0f);
    for (size_t i = 1; i < NTAPS; i++)
        output[i] = rrc(0.0f);

    for (size_t i = 0; i < NTAPS; i++) {
        INFO("Tap " << i << ": expected " << M17::rrc_taps_24k[i] << " got "
                    << output[i]);
        REQUIRE(std::abs(output[i] - M17::rrc_taps_24k[i]) < 1e-5f);
    }
}

TEST_CASE("RRC 24kHz filter has symmetric (linear phase) coefficients",
          "[m17][demodulator]")
{
    const auto &taps = M17::rrc_taps_24k;
    const size_t N2 = taps.size();

    for (size_t i = 0; i < N2 / 2; i++) {
        INFO("Taps " << i << " and " << (N2 - 1 - i) << " should be equal");
        REQUIRE(std::abs(taps[i] - taps[N2 - 1 - i]) < 1e-10f);
    }
}

TEST_CASE("RRC 24kHz filter has unity DC gain", "[m17][demodulator]")
{
    // Sum of all taps equals the DC gain; for a unit-gain RRC filter this
    // should be very close to 1.0.
    float sum = 0.0f;
    for (float t : M17::rrc_taps_24k)
        sum += t;

    REQUIRE(std::abs(sum - 1.0f) < 1e-3f);
}
