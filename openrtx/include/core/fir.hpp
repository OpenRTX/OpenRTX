/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
        float acc = 0.0f;
        size_t i;

        pos = (pos == 0 ? N - 1 : pos - 1);
        hist[pos] = input;
        hist[pos + N] = input;

        // Unroll loop by 4
        for (i = 0; i < (N & ~size_t(3)); i += 4) {
            acc += hist[pos + i] * taps[i];
            acc += hist[pos + i + 1] * taps[i + 1];
            acc += hist[pos + i + 2] * taps[i + 2];
            acc += hist[pos + i + 3] * taps[i + 3];
        }

        // Remaining taps
        for (; i < N; i++)
            acc += hist[pos + i] * taps[i];

        return acc;
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
    std::array< float, 2 * N >    hist;    ///< History of past inputs.
    size_t                        pos;     ///< Current position in history.
};

#endif /* DSP_H */
