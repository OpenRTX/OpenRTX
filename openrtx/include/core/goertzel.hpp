/***************************************************************************
 *   Copyright (C) 2025 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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
 ***************************************************************************/

#ifndef GOERTZEL_H
#define GOERTZEL_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <array>

/**
 * Class for modified Goertzel filter with configurable coefficients.
 */
template < size_t N >
class Goertzel
{
public:

    /**
     * Constructor.
     *
     * @param coeffs: Goertzel coefficients.
     */
    Goertzel(const std::array< float, N >& coeffs) : k(coeffs)
    {
        reset();
    }

    /**
     * Destructor.
     */
    ~Goertzel() { }

    /**
     * Update the internal states of the Goertzel filter.
     *
     * @param input: input value for the current time step.
     */
    void sample(const int16_t value)
    {
        for(size_t i = 0; i < N; i++)
        {
            float u = static_cast< float >(value) + (k[i] * u0[i]) - u1[i];
            u1[i] = u0[i];
            u0[i] = u;
        }
    }

    /**
     * Update the internal states of the Goertzel filter.
     *
     * @param samples: pointer to new input values.
     * @param numSamples: number of new input values.
     */
    void samples(const int16_t *samples, const size_t numSamples)
    {
        for(size_t i = 0; i < numSamples; i++)
            sample(samples[i]);
    }

    /**
     * Get signal power at a given frequency index.
     *
     * @param index: frequency index.
     * @return signal power.
     */
    float power(const size_t freq)
    {
        if(freq >= N)
            return 0;

        return (u0[freq] * u0[freq]) +
               (u1[freq] * u1[freq]) -
               (u0[freq] * u1[freq] * k[freq]);
    }

    /**
     * Reset filter history.
     */
    void reset()
    {
        u0.fill(0.0f);
        u1.fill(0.0f);
    }

private:

    const std::array< float, N >& k;
    std::array< float, N > u0;
    std::array< float, N > u1;
};

#endif /* GOERTZEL_H */
