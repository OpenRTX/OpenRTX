/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Silvano Seva IU2KWO,                            *
 *                         Frederik Saraci IU2NRO                          *
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

#ifndef FIR_H
#define FIR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <array>
#include <cstddef>
#include <cstdint>

/**
 * Class for FIR filter with configurable coefficients.
 * Adapted from the original implementation by Rob Riggs, Mobilinkd LLC.
 */
template < size_t N >
class Fir
{
public:

    /**
     * Constructor.
     *
     * @param taps: reference to a std::array of floating poing values representing
     * the FIR filter coefficients.
     */
    Fir(const std::array< float, N >& taps) : taps(taps), pos(0)
    {
        reset();
    }

    /**
     * Destructor.
     */
    ~Fir() { }

    /**
     * Perform one step of the FIR filter, computing a new output value given
     * the input value and the history of previous input values.
     *
     * @param input: FIR input value for the current time step.
     * @return FIR output as a function of the current and past input values.
     */
    float operator()(const float& input)
    {
        hist[pos] = input;
        pos = (pos + 1) % N;

        float  result = 0.0;
        size_t index  = pos;

        for(size_t i = 0; i < N; i++)
        {
            index   = (index != 0 ? index - 1 : N - 1);
            result += hist[index] * taps[i];
        }

        return result;
    }

    /**
     * Reset FIR history, clearing the memory of past values.
     */
    void reset()
    {
        hist.fill(0);
        pos = 0;
    }

private:

    const std::array< float, N >& taps;    ///< FIR filter coefficients.
    std::array< float, N >        hist;    ///< History of past inputs.
    size_t                        pos;     ///< Current position in history.
};

#endif /* DSP_H */
