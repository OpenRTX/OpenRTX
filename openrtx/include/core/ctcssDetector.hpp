/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CTCSS_DETECTOR_H
#define CTCSS_DETECTOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include "core/goertzel.hpp"

/*
 * Goertzel filter coefficients for the 50 CTCSS tones, computed for a sampling
 * frequency of 2kHz.
 */
static constexpr std::array < float, 50 > ctcssCoeffs2k =
{
    1.955858f, 1.952788f, 1.949194f, 1.945616f, 1.941767f, 1.937634f, 1.933200f,
    1.928450f, 1.923195f, 1.917936f, 1.911955f, 1.907097f, 1.902113f, 1.895202f,
    1.887648f, 1.879838f, 1.871331f, 1.862315f, 1.852531f, 1.842180f, 1.830988f,
    1.818907f, 1.806161f, 1.792725f, 1.778002f, 1.762507f, 1.753218f, 1.745912f,
    1.735704f, 1.728163f, 1.717311f, 1.709207f, 1.697685f, 1.688992f, 1.676770f,
    1.667463f, 1.654514f, 1.644208f, 1.630498f, 1.619878f, 1.605010f, 1.593692f,
    1.577610f, 1.548608f, 1.517951f, 1.503955f, 1.485167f, 1.450171f, 1.412880f,
    1.395880f
};

/**
 * CTCSS detector class, based on the modified Goertzel filter.
 * The detector is designed to be called recursively: each time the number of
 * accumulated samples reaches the detection window, the energy of each CTCSS
 * frequency is evaluated to find the one with the maximum value. If the ratio
 * between the peak energy and the average of all the energies is above the
 * threshold, the tone corresponding to the peak is considered detected.
 */
class CtcssDetector
{
public:

    /**
     * Constructor.
     *
     * @param coeffs: Goertzel filter coefficients.
     * @param window: size of the detection window.
     * @param threshold: detection threshold.
     */
    CtcssDetector(const std::array< float, 50 >& coeffs,
                  const uint32_t window, const float threshold) :
                  threshold(threshold), window(window), goertzel(coeffs),
                  totSamples(0), overThresh(false) { }

    /**
     * Destructor.
     */
    ~CtcssDetector() { }

    /**
     * Update the internal states of the Goertzel filter.
     *
     * @param samples: pointer to new input values.
     * @param numSamples: number of new input values.
     */
    void update(const int16_t *samples, const size_t numSamples)
    {
        goertzel.samples(samples, numSamples);
        totSamples += numSamples;

        if(totSamples >= window)
        {
            analyze();
            goertzel.reset();
            totSamples = 0;
        }
    }

    /**
     * Check if a CTCSS tone is being detected.
     *
     * @param toneIdx: index of the CTCSS tone.
     * @return true if the CTCSS tone is active.
     */
    bool toneDetected(const uint8_t toneIdx)
    {
        return (toneIdx == activeToneIdx) && (overThresh == true);
    }

    /**
     * Reset detector state.
     */
    void reset()
    {
        goertzel.reset();
        totSamples = 0;
        overThresh = false;
    }

private:

    /**
     * Compute signal power and determine if a CTCSS tone is active.
     */
    void analyze()
    {
        float avgPower = 0.0f;
        float maxPower = 0.0f;

        for(size_t i = 0; i < 50; i++)
        {
            float power = goertzel.power(i);
            avgPower += power;
            if(maxPower < power)
            {
                maxPower = power;
                activeToneIdx = i;
            }
        }

        avgPower /= 50.0f;
        overThresh = (maxPower >= (avgPower * threshold)) ? true : false;
    }

    const float    threshold;
    const uint32_t window;
    Goertzel< 50 > goertzel;
    uint32_t       totSamples;
    uint8_t        activeToneIdx;
    bool           overThresh;
};

#endif /* CTCSS_DETECTOR_H */
