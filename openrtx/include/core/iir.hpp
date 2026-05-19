/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef IIR_H
#define IIR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <array>
#include <cstddef>
#include <cstdint>

/**
 * Class for IIR filter with configurable coefficients.
 * Adapted from the original implementation by Rob Riggs, Mobilinkd LLC.
 */
template < size_t N >
class Iir
{
public:

    /**
     * Constructor.
     *
     * @param num: coefficients of the IIR filter numerator.
     * @param den: coefficients of the IIR filter denominator.
     */
    Iir(const std::array< float, N >& num, const std::array< float, N >& den) :
        num(num), den(den), pos(0)
    {
        reset();
    }

    /**
     * Destructor.
     */
    ~Iir() { }

    /**
     * Perform one step of the IIR filter, computing a new output value given
     * the input value and the history of previous input values.
     *
     * @param input: IIR input value for the current time step.
     * @return IIR output as a function of the current and past input values.
     */
    float operator()(const float& input)
    {
        float accNum = 0.0f;
        float accDen = 0.0f;
        size_t index = pos;

        for(size_t i = 1; i < N; i++)
        {
            index   = (index != 0 ? index - 1 : N - 1);
            accNum += hist[index] * num[i];
            accDen += hist[index] * den[i];
        }

        accNum   += num[0] * (input - accDen);
        hist[pos] = input - accDen;
        pos       = (pos + 1) % N;

        return accNum;
    }

    /**
     * Reset IIR history, clearing the memory of past values.
     */
    void reset()
    {
        hist.fill(0);
        pos = 0;
    }

private:

    const std::array< float, N >& num;    ///< IIR filter numerator coefficients.
    const std::array< float, N >& den;    ///< IIR filter denominator coefficients.
    std::array< float, N >        hist;   ///< History of past inputs.
    size_t                        pos;    ///< Current position in history.
};

#endif /* IIR_H */
