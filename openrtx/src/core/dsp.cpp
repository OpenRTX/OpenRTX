/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cerrno>

#include "core/dsp.h"
#include "core/fir.hpp"

// Low pass decimator taps, Q15, generated with scipy.signal as
//   round(firwin(len, 0.4625 * 48000 / ratio, window=('kaiser', 7),
//                fs=48000) * 32768)
// with len = 8 * ratio. The sum of products fits an int32 for any input.
static constexpr std::array<int16_t, 40> LOWPASS5_TAPS = {
    -3,   -9,   -13,  -6,   23,  73,  131,  157,  103,  -65,
    -335, -627, -794, -659, -80, 977, 2399, 3927, 5221, 5963,
    5963, 5221, 3927, 2399, 977, -80, -659, -794, -627, -335,
    -65,  103,  157,  131,  73,  23,  -6,   -13,  -9,   -3,
};

static constexpr std::array<int16_t, 48> LOWPASS6_TAPS = {
    -2,   -6,   -10,  -10,  -1,   22,  58,  100,  129,  124,  61,   -71,
    -260, -469, -629, -656, -467, -8,  728, 1687, 2750, 3763, 4557, 4994,
    4994, 4557, 3763, 2750, 1687, 728, -8,  -467, -656, -629, -469, -260,
    -71,  61,   124,  129,  100,  58,  22,  -1,   -10,  -10,  -6,   -2,
};

static constexpr std::array<int16_t, 64> LOWPASS8_TAPS = {
    -2,   -4,   -6,   -8,   -8,   -4,   5,    20,   41,   65,   87,   100,  97,
    70,   15,   -70,  -178, -297, -407, -484, -499, -428, -247, 52,   468,  982,
    1562, 2164, 2735, 3222, 3577, 3764, 3764, 3577, 3222, 2735, 2164, 1562, 982,
    468,  52,   -247, -428, -499, -484, -407, -297, -178, -70,  15,   70,   97,
    100,  87,   65,   41,   20,   5,    -4,   -8,   -8,   -6,   -4,   -2,
};

static_assert((LOWPASS5_TAPS.size() <= DSP_LOWPASS_MAX_TAPS)
                  && (LOWPASS6_TAPS.size() <= DSP_LOWPASS_MAX_TAPS)
                  && (LOWPASS8_TAPS.size() <= DSP_LOWPASS_MAX_TAPS),
              "DSP_LOWPASS_MAX_TAPS too small");

// Half an LSB of a Q15 product, to round to nearest
static constexpr int32_t Q15_HALF = 1 << 14;

int16_t dsp_dcBlockFilter(struct dcBlock *dcb, int16_t sample)
{
    /*
     * Implementation of a fixed-point DC block filter with noise shaping,
     * ensuring zero DC component at the output.
     * Filter pole set at 0.995
     *
     * Code adapted from https://dspguru.com/dsp/tricks/fixed-point-dc-blocking-filter-with-noise-shaping/
     */
    int32_t in = static_cast<int32_t>(sample) * 32768;

    // Start settled on the first sample, so a DC level gives no step
    if (dcb->initialized == false) {
        dcb->prevIn = in;
        dcb->initialized = true;
    }

    dcb->accum -= dcb->prevIn;
    dcb->prevIn = in;
    dcb->accum += dcb->prevIn;
    dcb->accum -= 164 * dcb->prevOut; // 32768.0 * (1.0 - pole)
    dcb->prevOut = dcb->accum >> 15;

    return static_cast<int16_t>(dcb->prevOut);
}

bool dsp_decimator(struct decimatorState *state, int16_t *sample,
                   uint16_t ratio)
{
    state->accumulator += *sample;
    state->count++;
    if (state->count >= ratio) {
        // Integer division truncates toward zero; this is intentional as
        // rounding is unnecessary for audio signal averaging.
        *sample = state->accumulator / ratio;
        state->accumulator = 0;
        state->count = 0;
        return true;
    }

    return false;
}

int dsp_lowPassInit(struct lowPassDecimator *state, uint8_t ratio)
{
    memset(state, 0, sizeof(*state));

    switch (ratio) {
        case 5:
            state->taps = LOWPASS5_TAPS.data();
            state->len = LOWPASS5_TAPS.size();
            break;

        case 6:
            state->taps = LOWPASS6_TAPS.data();
            state->len = LOWPASS6_TAPS.size();
            break;

        case 8:
            state->taps = LOWPASS8_TAPS.data();
            state->len = LOWPASS8_TAPS.size();
            break;

        default:
            return -ENOTSUP;
    }

    state->ratio = ratio;

    return 0;
}

bool dsp_lowPassDecimate(struct lowPassDecimator *state, int16_t *sample)
{
    firPush(state->history, state->len, state->pos, *sample);
    state->count++;
    if (state->count < state->ratio)
        return false;

    state->count = 0;

    const int32_t acc = firDot<int32_t>(state->taps, state->history, state->len,
                                        state->pos);
    const int32_t out = (acc + Q15_HALF) >> 15;
    if (out > INT16_MAX)
        *sample = INT16_MAX;
    else if (out < INT16_MIN)
        *sample = INT16_MIN;
    else
        *sample = static_cast<int16_t>(out);

    return true;
}
