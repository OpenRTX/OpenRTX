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

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <type_traits>

/**
 * Class for FIR filter with configurable coefficients.
 * Adapted from the original implementation by Rob Riggs, Mobilinkd LLC.
 *
 * By default samples, coefficients and the accumulator are floats. For a
 * fixed-point filter, give integer types: the output is the accumulator
 * rounded, shifted right by FRAC and saturated to the sample type. FRAC
 * defaults to the value bits of an integer coefficient type, 15 for Q15
 * int16_t taps.
 *
 * @tparam N: number of coefficients.
 * @tparam T: type of the input and output samples.
 * @tparam C: type of the coefficients.
 * @tparam A: type of the accumulator.
 * @tparam FRAC: fractional bits of the coefficients, 0 for floats.
 */
template < size_t N, typename T = float, typename C = T, typename A = T,
           unsigned FRAC = (std::numeric_limits< C >::is_integer
                                ? std::numeric_limits< C >::digits
                                : 0) >
class Fir
{
public:

    /**
     * Constructor.
     *
     * @param taps: reference to a std::array of the FIR filter coefficients.
     */
    Fir(const std::array< C, N >& taps) : taps(taps), pos(0)
    {
        reset();
    }

    /**
     * Destructor.
     */
    ~Fir() { }

    /**
     * Add an input value to the history of the FIR filter.
     *
     * @param input: FIR input value for the current time step.
     */
    void sample(const T& input)
    {
        pos = (pos == 0 ? N - 1 : pos - 1);
        hist[pos] = input;
        hist[pos + N] = input;
    }

    /**
     * Compute the FIR output from the history of input values.
     *
     * @return FIR output as a function of the current and past input values.
     */
    T operator()() const
    {
        const A acc = std::inner_product(taps.begin(), taps.end(),
                                         hist.begin() + pos, A(0));

        return scale(acc, std::is_integral< T >());
    }

    /**
     * Perform one step of the FIR filter, computing a new output value given
     * the input value and the history of previous input values.
     *
     * @param input: FIR input value for the current time step.
     * @return FIR output as a function of the current and past input values.
     */
    T operator()(const T& input)
    {
        sample(input);
        return (*this)();
    }

    /**
     * Reset FIR history, filling the memory of past values with a value.
     *
     * @param value: value of every past input, zero by default.
     */
    void reset(const T& value = T(0))
    {
        hist.fill(value);
        pos = 0;
    }

private:

    /**
     * Bring a fixed-point accumulator back to the scale of the samples,
     * rounding to nearest and saturating.
     */
    static T scale(const A acc, std::true_type)
    {
        static_assert(FRAC > 0, "Fixed-point taps need fractional bits");

        const A out = (acc + (A(1) << (FRAC - 1))) >> FRAC;
        return static_cast< T >(std::min< A >(
            std::max< A >(out, std::numeric_limits< T >::min()),
            std::numeric_limits< T >::max()));
    }

    /**
     * A floating point accumulator is already at the scale of the samples.
     */
    static T scale(const A acc, std::false_type)
    {
        return static_cast< T >(acc);
    }

    const std::array< C, N >& taps;    ///< FIR filter coefficients.
    std::array< T, 2 * N >    hist;    ///< History of past inputs.
    size_t                    pos;     ///< Current position in history.
};

#endif /* FIR_H */
