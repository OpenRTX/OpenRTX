/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NOISYCHANNEL_H
#define NOISYCHANNEL_H

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
class NoisyChannel
{
public:
    /**
     * Constructor.
     *
     * @param rate: sample rate, in Hz.
     * @param seed: noise generator seed.
     * @param idleRms: noise rms without a signal.
     * @param signalRms: noise rms added to a signal.
     */
    NoisyChannel(const uint32_t rate, const uint32_t seed, const float idleRms,
                 const float signalRms);

    /**
     * Append noise only.
     *
     * @param seconds: duration.
     */
    void idle(const double seconds);

    /**
     * Append noise only until the sample count is phase modulo period.
     *
     * @param period: period, in samples.
     * @param phase: sample count to reach, modulo period.
     */
    void align(const size_t period, const size_t phase);

    /**
     * Append a signal with its noise.
     *
     * @param signal: signal samples.
     */
    void transmit(const std::vector<float> &signal);

    /**
     * Get the samples so far, clipped to 16 bits.
     *
     * @return the samples.
     */
    const std::vector<int16_t> &samples() const;

    /**
     * Get the sample rate.
     *
     * @return sample rate, in Hz.
     */
    uint32_t rate() const;

private:
    /**
     * Draw a Gaussian sample with unit rms.
     *
     * @return the sample.
     */
    float gaussian();

    /**
     * Append one sample of signal plus noise.
     *
     * @param signal: signal value.
     * @param noiseRms: noise rms.
     */
    void append(const float signal, const float noiseRms);

    uint32_t sampleRate;           ///< Sample rate, in Hz.
    float idleRms;                 ///< Noise rms without a signal.
    float signalRms;               ///< Noise rms added to a signal.
    std::mt19937 rng;              ///< Noise generator.
    std::vector<int16_t> baseband; ///< Samples so far.
    float spare;                   ///< Second Box-Muller sample.
    bool haveSpare;                ///< spare holds an unused sample.
};

#endif // NOISYCHANNEL_H
