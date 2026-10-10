/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CHANNEL_H
#define CHANNEL_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

/**
 * Receiver input for tests: signals plus Gaussian noise, at any rate. The
 * noise is Box-Muller on std::mt19937, the same with every standard library.
 */
class Channel
{
public:
    /** Noise rms is idleRms without a signal and signalRms added to one. */
    Channel(const uint32_t rate, const uint32_t seed, const float idleRms,
            const float signalRms);

    /** Append noise only. */
    void idle(const double seconds);

    /** Append noise only until the sample count is phase modulo period. */
    void align(const size_t period, const size_t phase);

    /** Append a signal with its noise. */
    void transmit(const std::vector<float> &signal);

    /** Samples so far, clipped to 16 bits. */
    const std::vector<int16_t> &samples() const;

    uint32_t rate() const;

private:
    float gaussian();
    void append(const float signal, const float noiseRms);

    uint32_t sampleRate;
    float idleRms;
    float signalRms;
    std::mt19937 rng;
    std::vector<int16_t> baseband;
    float spare;
    bool haveSpare;
};

#endif // CHANNEL_H
