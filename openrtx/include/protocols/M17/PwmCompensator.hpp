/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#ifndef PWMCOMPENSATOR_H
#define PWMCOMPENSATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <array>
#include <cstddef>
#include <cstdint>

/**
 * Compensation filter for MDx PWM-based baseband output.
 */
class PwmCompensator
{
public:

    /**
     * Constructor.
     */
    PwmCompensator()
    {
        reset();
    }

    /**
     * Destructor.
     */
    ~PwmCompensator() { }

    /**
     * Perform one step of the filter, computing a new output value given
     * the input value and the history of previous input values.
     *
     * @param input: input value for the current time step.
     * @return output as a function of the current and past input values.
     */
    float operator()(const float& input)
    {
        u[0] = input;
        y[0] = b[0] * u[0]
             + b[1] * u[1]
             + b[2] * u[2]
             - a[1] * y[1]
             - a[2] * y[2];

        for(size_t i = 2; i > 0; i--)
        {
            u[i] = u[i - 1];
            y[i] = y[i - 1];
        }

        return y[0];
    }

    /**
     * Reset history, clearing the memory of past values.
     */
    void reset()
    {
        u.fill(0.0f);
        y.fill(0.0f);
    }

private:

    static constexpr float b[3]={
        4.098580379420818f, -5.206850085165959f, 1.539109584496514f
    };

    static constexpr float a[3]={
        0.0f, -0.044699895066380f, 0.000446359429843f
    };

    std::array< float, 3 > u;   ///< History of past inputs.
    std::array< float, 3 > y;   ///< History of past outputs.
};

#endif /* PWMCOMPENSATOR_H */
