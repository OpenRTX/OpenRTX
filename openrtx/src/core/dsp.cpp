/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cerrno>
#include <new>

#include "core/dsp.h"
#include "core/fir.hpp"

// Low pass decimator taps, Q15, generated with scipy.signal as
//   round(firwin(8 * ratio, 0.4625 * fs / ratio, window=('kaiser', 7),
//                fs=fs) * 32768)
// with fs the input rate. The sum of products fits an int32 for any input.
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

using LowPass6Fir = Fir<LOWPASS6_TAPS.size(), int16_t, int16_t, int32_t>;
using LowPass8Fir = Fir<LOWPASS8_TAPS.size(), int16_t, int16_t, int32_t>;

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

/**
 * \internal
 * Run a single step of a low pass decimator with a filter of type F.
 *
 * @param state: pointer to the decimator state.
 * @param sample: pointer to the input sample, the output when returning true.
 * @return true when a decimated sample is ready.
 */
template <typename F>
static bool lowPassStep(struct lowPassDecimator *state, int16_t *sample)
{
    F *fir = static_cast<F *>(state->fir);

    // Start settled on the first sample, so a DC level gives no step
    if (state->initialized == false) {
        fir->reset(*sample);
        state->initialized = true;
    }

    fir->sample(*sample);
    state->count++;
    if (state->count < state->ratio)
        return false;

    state->count = 0;
    *sample = (*fir)();

    return true;
}

int dsp_lowPassInit(struct lowPassDecimator *state, uint8_t ratio)
{
    memset(state, 0, sizeof(*state));

    switch (ratio) {
        case 6:
            state->fir = new (std::nothrow) LowPass6Fir(LOWPASS6_TAPS);
            break;

        case 8:
            state->fir = new (std::nothrow) LowPass8Fir(LOWPASS8_TAPS);
            break;

        default:
            return -ENOTSUP;
    }

    if (state->fir == nullptr)
        return -ENOMEM;

    state->ratio = ratio;

    return 0;
}

bool dsp_lowPassDecimate(struct lowPassDecimator *state, int16_t *sample)
{
    switch (state->ratio) {
        case 6:
            return lowPassStep<LowPass6Fir>(state, sample);

        case 8:
            return lowPassStep<LowPass8Fir>(state, sample);

        default:
            return false;
    }
}

void dsp_lowPassTerminate(struct lowPassDecimator *state)
{
    switch (state->ratio) {
        case 6:
            delete static_cast<LowPass6Fir *>(state->fir);
            break;

        case 8:
            delete static_cast<LowPass8Fir *>(state->fir);
            break;

        default:
            break;
    }

    state->fir = nullptr;
    state->ratio = 0;
}
