/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CLOCK_RECOVERY_H
#define CLOCK_RECOVERY_H

#include <cstdint>
#include <cstdio>
#include <array>

/**
 * Class to construct clock recovery objects.
 * The clock recovery algorithm estimates the best sampling point by finding,
 * within a symbol, the point with maximum energy. The algorithm will work
 * properly only if correctly synchronized with the baseband stream.
 */
template <size_t SAMPLES_PER_SYMBOL>
class ClockRecovery
{
public:
    /**
     * Constructor
     */
    ClockRecovery()
    {
        reset();
    }

    /**
     * Destructor
     */
    ~ClockRecovery()
    {
    }

    /**
     * Reset the internal state.
     */
    void reset()
    {
        curIdx = 0;
        prevSample = 0;
        numSamples = 0;
        updateReq = false;
        energy.fill(0);
    }

    /**
     * Process a new sample.
     *
     * @param sample: baseband sample.
     */
    void sample(int16_t &sample)
    {
        int32_t delta = static_cast<int32_t>(sample)
                      - static_cast<int32_t>(prevSample);

        if ((sample + prevSample) < 0)
            delta = -delta;

        energy[curIdx] += delta;
        prevSample = sample;
        curIdx = (curIdx + 1) % SAMPLES_PER_SYMBOL;
        numSamples += 1;
    }

    /**
     * Update the best sampling point estimate.
     */
    void update()
    {
        if (numSamples == 0)
            return;

        uint8_t index = 0;
        bool is_positive = false;

        for (size_t i = 0; i < SAMPLES_PER_SYMBOL; i++) {
            int32_t phase = energy[i];

            if (!is_positive && phase > 0) {
                is_positive = true;
            } else if (is_positive && phase < 0) {
                index = i;
                break;
            }
        }

        if (index == 0)
            sp = SAMPLES_PER_SYMBOL - 1;
        else
            sp = index - 1;

        energy.fill(0);
        numSamples = 0;
    }

    /**
     * Get the best sampling point estimate.
     * The returned value is within the space of a simbol, that is in the
     * range [0 SAMPLES_PER_SYMBOL - 1].
     *
     * @return sampling point.
     */
    uint8_t samplingPoint()
    {
        return sp;
    }

private:
    std::array<int32_t, SAMPLES_PER_SYMBOL> energy;
    size_t curIdx;
    size_t numSamples;
    int16_t prevSample;
    uint8_t sp;
    bool updateReq;
};

#endif
