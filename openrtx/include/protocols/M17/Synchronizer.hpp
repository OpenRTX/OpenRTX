/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SYNCHRONIZER_H
#define SYNCHRONIZER_H

#include <cstdint>
#include <array>
#include "Correlator.hpp"

/**
 * Frame syncronizer class. It allows to find the best sampling point for a
 * baseband stream, given a syncword.
 */
template < size_t SYNCW_SIZE, size_t SAMPLES_PER_SYM >
class Synchronizer
{
public:

    /**
     * Constructor.
     *
     * @param sync_word: symbols of the target syncword.
     */
    Synchronizer(std::array< int8_t, SYNCW_SIZE >&& sync_word) :
        syncword(std::move(sync_word)), triggered(false) { }

    /**
     * Destructor.
     */
    ~Synchronizer() { }

    /**
     * Perform an update step of the syncronizer.
     *
     * @param correlator: correlator object to be used to compute the convolution
     * product with the syncword.
     * @param posTh: threshold to detect a positive correlation peak.
     * @param negTh: threshold to detect a negative correlation peak.
     * @return +1 if a positive correlation peak has been found, -1 if a negative
     * correlation peak has been found an zero otherwise.
     */
    int8_t update(Correlator< SYNCW_SIZE, SAMPLES_PER_SYM >& correlator,
                  const int32_t posTh, const int32_t negTh)
    {
        int32_t sign    = 0;
        int32_t corr    = correlator.convolve(syncword);
        bool    trigger = (corr > posTh) || (corr < negTh);

        if(trigger == true)
        {
            if(triggered == false)
            {
                values.fill(0);
                triggered = true;
            }

            values[correlator.sampleIndex()] = corr;
        }
        else
        {
            if(triggered)
            {
                // Calculate the sampling index on the falling edge.
                triggered = false;
                sampIndex = 0;

                int32_t peak  = corr;
                uint8_t index = 0;
                for(auto val : values)
                {
                    if(std::abs(val) > std::abs(peak))
                    {
                        peak = val;
                        sampIndex = index;
                    }

                    index += 1;
                }

                if(peak >= 0)
                    sign = 1;
                else
                    sign = -1;
            }
        }

        return sign;
    }

    /**
     * Get the best sampling index equivalent to the last correlation peak
     * found. This value is meaningful only when the update() function returned
     * a value different from zero.
     *
     * @return the optimal sampling index.
     */
    size_t samplingIndex()
    {
        return sampIndex;
    }

private:

    std::array< int8_t, SYNCW_SIZE >       syncword;    ///< Target syncword
    std::array< int32_t, SAMPLES_PER_SYM > values;      ///< Correlation history
    bool                                   triggered;   ///< Peak found
    uint8_t                                sampIndex;   ///< Optimal sampling point
};

#endif
