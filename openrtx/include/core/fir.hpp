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
 * Store a new input in a FIR history of 2 * len samples. Each input is stored
 * twice, so the newest len inputs are always contiguous, newest first, from
 * the updated position.
 *
 * @param hist: history, 2 * len samples.
 * @param len: number of filter taps.
 * @param pos: position of the newest input, updated.
 * @param input: new input.
 */
template < typename T >
void firPush(T *hist, const size_t len, size_t &pos, const T input)
{
    pos = (pos == 0 ? len - 1 : pos - 1);
    hist[pos] = input;
    hist[pos + len] = input;
}

/**
 * Sum of the products of the taps with the newest len inputs of a history
 * filled by firPush(), accumulated in type A.
 *
 * @param taps: len filter coefficients.
 * @param hist: history.
 * @param len: number of filter taps.
 * @param pos: position of the newest input.
 * @return the sum of products.
 */
template < typename A, typename T, typename C >
A firDot(const C *taps, const T *hist, const size_t len, const size_t pos)
{
    A acc = 0;
    size_t i;

    // Unroll loop by 4
    for (i = 0; i < (len & ~size_t(3)); i += 4) {
        acc += static_cast< A >(hist[pos + i]) * taps[i];
        acc += static_cast< A >(hist[pos + i + 1]) * taps[i + 1];
        acc += static_cast< A >(hist[pos + i + 2]) * taps[i + 2];
        acc += static_cast< A >(hist[pos + i + 3]) * taps[i + 3];
    }

    // Remaining taps
    for (; i < len; i++)
        acc += static_cast< A >(hist[pos + i]) * taps[i];

    return acc;
}

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
        firPush(hist.data(), N, pos, input);
        return firDot< float >(taps.data(), hist.data(), N, pos);
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
