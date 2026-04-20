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
#include "protocols/M17/Demodulator.hpp"
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
    Fir<float, NTAPS> rrc(M17::rrc_taps_24k);
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

// ---------------------------------------------------------------------------
// End-to-end demodulator lock tests
// ---------------------------------------------------------------------------

// Stream syncword symbols: 0xFF5D → dibits 11 11 11 11 01 01 11 01
// Mapped to ±3/±1: +3→11, +1→01, -1→10, -3→00  →  -3,-3,-3,-3,+3,+3,-3,+3
static constexpr std::array<int8_t, M17::SYNCWORD_SYMBOLS> STREAM_SW_SYMS = {
    -3, -3, -3, -3, +3, +3, -3, +3
};

// Generate an RRC-shaped baseband signal from a sequence of M17 symbols.
// Each symbol is placed at sample-rate position (one every SAMPLES_PER_SYM
// samples) with zero-fill in between, then shaped by the RRC transmit filter.
static std::vector<int16_t> rrcBaseband(const std::vector<int8_t> &symbols,
                                        float amplitude = 2000.0f)
{
    static constexpr size_t SPS = SAMPLES_PER_SYM;
    static constexpr size_t NTAPS = M17::rrc_taps_24k.size();

    Fir<float, NTAPS> txRrc(M17::rrc_taps_24k);

    std::vector<int16_t> out;
    out.reserve(symbols.size() * SPS);

    for (size_t i = 0; i < symbols.size(); i++) {
        // First sample of the symbol period carries the impulse
        float imp = static_cast<float>(symbols[i]) * amplitude;
        out.push_back(static_cast<int16_t>(txRrc(imp) * SPS));

        // Remaining SPS-1 samples are zero (interpolation)
        for (size_t s = 1; s < SPS; s++)
            out.push_back(static_cast<int16_t>(txRrc(0.0f) * SPS));
    }

    return out;
}

// Build the symbol sequence for one complete M17 stream frame (192 symbols):
// 8 sync-word symbols + 184 payload symbols (alternating +1/-1 pattern).
static std::vector<int8_t> makeStreamFrame()
{
    std::vector<int8_t> syms(M17::FRAME_SYMBOLS);
    for (size_t i = 0; i < STREAM_SW_SYMS.size(); i++)
        syms[i] = STREAM_SW_SYMS[i];
    for (size_t i = STREAM_SW_SYMS.size(); i < syms.size(); i++)
        syms[i] = (i % 2 == 0) ? +1 : -1;
    return syms;
}

TEST_CASE("Demodulator maintains lock across multiple consecutive stream frames",
          "[m17][demodulator]")
{
    // --- Build the synthetic baseband signal ---
    // Preamble: silence long enough to pass the INIT state (480 samples)
    // plus RRC filter settling time.
    static constexpr size_t PREAMBLE_SYMS = 200; // 200 * 5 = 1000 samples
    static constexpr size_t NUM_FRAMES = 10;

    std::vector<int8_t> allSyms;

    // Preamble: alternating +3/-3 to build up correlator energy
    for (size_t i = 0; i < PREAMBLE_SYMS; i++)
        allSyms.push_back((i % 2 == 0) ? +3 : -3);

    // Concatenate NUM_FRAMES complete stream frames
    auto oneFrame = makeStreamFrame();
    for (size_t f = 0; f < NUM_FRAMES; f++)
        allSyms.insert(allSyms.end(), oneFrame.begin(), oneFrame.end());

    // Trailing silence so the last frame can finish processing
    for (size_t i = 0; i < PREAMBLE_SYMS; i++)
        allSyms.push_back(0);

    std::vector<int16_t> baseband = rrcBaseband(allSyms);

    // --- Feed samples through the demodulator ---
    M17::Demodulator demod;
    demod.init();

    bool everLocked = false;
    size_t lockSample = 0;
    bool lostLock = false;
    size_t lostSample = 0;

    for (size_t i = 0; i < baseband.size(); i++) {
        demod.sample(baseband[i]);

        if (!everLocked && demod.isLocked()) {
            everLocked = true;
            lockSample = i;
        }

        // Once locked, the demodulator must stay locked for at least the
        // duration of the remaining frames (until the signal ends).
        // Allow a grace zone at the very end where the trailing silence
        // causes a natural unlock (last ~3 frame-lengths).
        size_t endGrace = baseband.size()
                        - 3 * M17::FRAME_SYMBOLS * SAMPLES_PER_SYM;
        if (everLocked && !demod.isLocked() && i < endGrace) {
            lostLock = true;
            lostSample = i;
            break; // No need to keep going
        }
    }

    INFO("Lock first acquired at sample " << lockSample);
    REQUIRE(everLocked);

    INFO("Lock lost at sample " << lostSample << " ("
                                << (lostSample - lockSample)
                                << " samples after lock)");
    REQUIRE_FALSE(lostLock);
}
