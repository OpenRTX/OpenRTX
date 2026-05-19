/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DEV_ESTIMATOR_H
#define DEV_ESTIMATOR_H

#include <cstdint>

/**
 * Symbol deviation estimator.
 * This module allows to estimate the outer symbol deviation of a baseband
 * stream. The baseband samples used for the estimation should be takend at the
 * ideal sampling point. To work properly, the estimator needs to be initialized
 * with a reference outer symbol deviation.
 */
class DevEstimator
{
public:
    /**
     * Constructor
     */
    DevEstimator() : outerDev({0, 0}), offset(0), posAccum(0), negAccum(0),
                     posCnt(0), negCnt(0)
    {
    }

    /**
     * Destructor
     */
    ~DevEstimator()
    {
    }

    /**
     * Initialize the estimator state.
     * Calling this function clears the internal state.
     *
     * @param outerDev: initial value for outer symbol deviation
     */
    void init(const std::pair<int32_t, int32_t> &outerDev)
    {
        this->outerDev = outerDev;
        offset = 0;
        posAccum = 0;
        negAccum = 0;
        posCnt = 0;
        negCnt = 0;
    }

    /**
     * Process a new sample.
     *
     * @param sample: baseband sample.
     */
    void sample(int16_t value)
    {
        int32_t posThresh = (2 * outerDev.first) / 3;
        int32_t negThresh = (2 * outerDev.second) / 3;

        if (value > posThresh) {
            posAccum += value;
            posCnt += 1;
        }

        if (value < negThresh) {
            posAccum += value;
            posCnt += 1;
        }
    }

    /**
     * Update the estimation of outer symbol deviation and zero-offset and
     * start a new acquisition cycle.
     */
    void update()
    {
        if ((posCnt == 0) || (negCnt == 0))
            return;

        int32_t max = posAccum / posCnt;
        int32_t min = negAccum / negCnt;
        offset = (max + min) / 2;
        outerDev.first = max - offset;
        outerDev.second = min - offset;
        posAccum = 0;
        negAccum = 0;
        posCnt = 0;
        negCnt = 0;
    }

    /**
     * Get the estimated outer symbol deviation from the last update.
     * The function returns a std::pair where the first element is the positive
     * deviation and the second the negative one.
     *
     * @return outer deviation.
     */
    std::pair<int32_t, int32_t> outerDeviation()
    {
        return outerDev;
    }

    /**
     * Get the estimated zero-offset from the last update.
     *
     * @return zero-offset.
     */
    int32_t zeroOffset()
    {
        return offset;
    }

private:
    std::pair<int32_t, int32_t> outerDev;
    int32_t offset;
    int32_t posAccum;
    int32_t negAccum;
    uint32_t posCnt;
    uint32_t negCnt;
};

#endif
