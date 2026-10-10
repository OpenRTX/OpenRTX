/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

#include "protocols/M17/Correlator.hpp"
#include "protocols/M17/Constants.hpp"
#include "protocols/M17/Demodulator.hpp"
#include "protocols/M17/DevEstimator.hpp"
#include "protocols/M17/DSP.hpp"
#include "protocols/M17/Synchronizer.hpp"
#include "core/fir.hpp"
#include "M17Signal.hpp"

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

// ---------------------------------------------------------------------------
// End-to-end demodulator lock tests
// ---------------------------------------------------------------------------

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
    M17Signal::appendPreamble(allSyms, PREAMBLE_SYMS);

    // NUM_FRAMES encoded stream frames, without the LSF
    auto frames = M17Signal::voiceFrames(NUM_FRAMES, false);
    for (size_t f = 1; f <= NUM_FRAMES; f++)
        M17Signal::appendFrame(allSyms, frames[f]);

    // Trailing silence so the last frame can finish processing
    for (size_t i = 0; i < PREAMBLE_SYMS; i++)
        allSyms.push_back(0);

    std::vector<int16_t> baseband = M17Signal::rrcBaseband(allSyms);

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

// ---------------------------------------------------------------------------
// Symbol deviation estimator
// ---------------------------------------------------------------------------

// One frame of samples at the four symbol levels; scale is the +1 level
static void feedLevels(DevEstimator &est, const int32_t scale,
                       const int32_t offset = 0)
{
    static constexpr std::array<int8_t, 4> SYMBOLS = { +3, +1, -1, -3 };

    for (size_t i = 0; i < M17::FRAME_SYMBOLS; i++)
        est.sample(static_cast<int16_t>(SYMBOLS[i % 4] * scale + offset));
}

TEST_CASE("DevEstimator updates the outer deviation from a frame",
          "[m17][demodulator]")
{
    // Initial estimate 20% low, as from a noisy syncword
    DevEstimator est;
    est.init({ 2400, -2400 });

    feedLevels(est, SCALE);
    est.update();

    REQUIRE(est.outerDeviation().first == 3 * SCALE);
}

TEST_CASE("DevEstimator measures a negative outer deviation",
          "[m17][demodulator]")
{
    DevEstimator est;
    est.init({ 2400, -2400 });

    feedLevels(est, SCALE);
    est.update();

    REQUIRE(est.outerDeviation().second == -3 * SCALE);
    REQUIRE(est.zeroOffset() == 0);
}

// A stream frame: the stream syncword followed by a random payload.
static M17::frame_t randomStreamFrame(std::minstd_rand &rng)
{
    M17::frame_t frame;

    frame[0] = M17::STREAM_SYNC_WORD[0];
    frame[1] = M17::STREAM_SYNC_WORD[1];
    for (size_t i = 2; i < frame.size(); i++)
        frame[i] = static_cast<uint8_t>(rng());

    return frame;
}

// A synthetic stream: preamble, random stream frames, then silence.
struct TestStream {
    std::vector<M17::frame_t> frames; // Transmitted frames
    std::vector<size_t> frameStart;   // First baseband sample of each frame
    std::vector<int16_t> baseband;
};

static TestStream makeStream(const size_t numFrames, const float amplitude)
{
    static constexpr size_t PREAMBLE_SYMS = 200;
    std::minstd_rand rng(1);
    std::vector<int8_t> syms;
    TestStream s;

    M17Signal::appendPreamble(syms, PREAMBLE_SYMS);

    for (size_t f = 0; f < numFrames; f++) {
        M17::frame_t frame = randomStreamFrame(rng);

        s.frames.push_back(frame);
        s.frameStart.push_back(syms.size() * SAMPLES_PER_SYM);
        M17Signal::appendFrame(syms, frame);
    }

    for (size_t i = 0; i < PREAMBLE_SYMS; i++)
        syms.push_back(0);

    s.baseband = M17Signal::rrcBaseband(syms, amplitude);
    return s;
}

// Transmitted frames received without errors
static std::vector<bool> receivedFrames(const TestStream &s)
{
    std::vector<bool> received(s.frames.size(), false);
    M17::Demodulator demod;
    demod.init();

    for (int16_t sample : s.baseband) {
        if (demod.sample(sample) == false)
            continue;

        const M17::frame_t &frame = demod.getFrame();
        for (size_t f = 0; f < s.frames.size(); f++) {
            if (frame == s.frames[f])
                received[f] = true;
        }
    }

    return received;
}

TEST_CASE("Demodulator follows an increase of the signal level",
          "[m17][demodulator]")
{
    // +8 dB in frame 4: inner symbols cross the lock-time outer threshold
    static constexpr size_t NUM_FRAMES = 12;
    static constexpr size_t STEP_FRAME = 4;
    static constexpr float GAIN = 2.5f;

    TestStream s = makeStream(NUM_FRAMES, 800.0f);
    size_t step = (s.frameStart[STEP_FRAME] + s.frameStart[STEP_FRAME + 1]) / 2;
    for (size_t i = step; i < s.baseband.size(); i++)
        s.baseband[i] = static_cast<int16_t>(s.baseband[i] * GAIN);

    auto received = receivedFrames(s);

    // All but the frame with the step and the next one must be received
    for (size_t f = 0; f < NUM_FRAMES; f++) {
        INFO("Frame " << f);
        if ((f < STEP_FRAME) || (f > STEP_FRAME + 1))
            REQUIRE(received[f]);
    }
}
