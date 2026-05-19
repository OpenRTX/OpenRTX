/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
        y[0] = (a/d)*(u[0])
             + (b/d)*(u[1])
             + (c/d)*(u[2])
             - (e/d)*(y[1])
             - (f/d)*(y[2]);

        for(size_t i = 2; i > 0; i--)
        {
            u[i] = u[i - 1];
            y[i] = y[i - 1];
        }

        return y[0] * 0.5f;
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

    static constexpr float a =  4982680082321166792352.0f;
    static constexpr float b = -6330013275146484168000.0f;
    static constexpr float c =  1871109008789062500000.0f;
    static constexpr float d =  548027992248535162477.0f;
    static constexpr float e = -24496793746948241250.0f;
    static constexpr float f =  244617462158203125.0f;

    std::array< float, 3 > u;   ///< History of past inputs.
    std::array< float, 3 > y;   ///< History of past outputs.
};

#endif /* PWMCOMPENSATOR_H */
