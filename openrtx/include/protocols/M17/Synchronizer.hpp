/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 **************************************************************************/

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
        syncword(std::move(sync_word)) { }

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

                if(values[index] >= 0)
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
