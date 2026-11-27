/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <new>
#include <numeric>

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

// Automatic gain control, per 20 ms block. Range of the gain over the mic
// gain: down far enough for any input, up enough for a quiet talker.
static constexpr float AGC_MIN_GAIN_DB = -40.0f;
static constexpr float AGC_MAX_GAIN_DB = 24.0f;
// Largest fall per block, about 75 dB/s, and largest rise, about 4 dB/s
static constexpr float AGC_ATTACK_DB = 1.5f;
static constexpr float AGC_RELEASE_DB = 0.08f;
// While acquiring, the gain may rise this many times faster
static constexpr float AGC_ACQUIRE_SPEEDUP = 10.0f;
// A block this far over the target aims at the target at once
static constexpr float AGC_FAST_DOWN_DB = 6.0f;
// Speech is a block this far over the noise floor
static constexpr float AGC_GATE_DB = 5.0f;
static constexpr float AGC_GATE_MIN_DB = -65.0f;
// Share of the gap by which the speech level rises or falls per block
static constexpr float AGC_LEVEL_RISE = 0.5f;
static constexpr float AGC_LEVEL_FALL = 0.05f;
// Soft limiter knee and ceiling, in samples: -6 and -1 dBFS
static constexpr int32_t AGC_KNEE = 16423;
static constexpr int32_t AGC_CEILING = 29205;
// Gains on the samples are in Q16.16
static constexpr int32_t AGC_GAIN_ONE = 1 << 16;
// The limiter's curve, tanh's (3,2) Padé approximant t (27 + t^2) /
// (27 + 9 t^2), at t = 3 i / 64 in Q15: it leaves the knee with slope 1 and
// reaches 1 with slope 0 at t = 3, where it is held
static constexpr int32_t AGC_CURVE_STEPS = 64;
static constexpr int32_t AGC_CURVE_SPAN = 3 * (AGC_CEILING - AGC_KNEE);
static constexpr std::array<uint16_t, AGC_CURVE_STEPS + 1> AGC_CURVE = {
    0,     1535,  3064,  4581,  6081,  7557,  9006,  10421, 11799, 13136, 14428,
    15673, 16869, 18013, 19104, 20142, 21127, 22057, 22935, 23759, 24533, 25256,
    25931, 26559, 27142, 27682, 28180, 28640, 29063, 29451, 29807, 30132, 30427,
    30696, 30940, 31161, 31360, 31539, 31700, 31844, 31972, 32086, 32187, 32276,
    32354, 32423, 32482, 32533, 32577, 32615, 32647, 32673, 32696, 32714, 32728,
    32740, 32749, 32755, 32760, 32764, 32766, 32767, 32768, 32768, 32768,
};
// Power added before taking a logarithm, -120 dBFS
static constexpr float AGC_POWER_EPS = 1e-12f;
// Window length, 400 ms, and speech needed before a rise, 120 ms
static constexpr uint16_t AGC_WINDOW_BLOCKS = 20;
static constexpr uint16_t AGC_CONFIRM_BLOCKS = 6;
// Blocks of speech with a faster rise after initialisation, 1 s
static constexpr uint16_t AGC_ACQUIRE_BLOCKS = 50;

/**
 * Convert a level in dB to a linear amplitude ratio.
 *
 * @param db: level, in dB.
 * @return the amplitude ratio.
 */
static inline float dbToLin(float db)
{
    return powf(10.0f, db / 20.0f);
}

/**
 * Soft limiter: linear below the knee, then the curve of AGC_CURVE, linearly
 * interpolated, which flattens onto the ceiling. All 32-bit, and the only
 * division is by a constant.
 *
 * @param x: sample, before limiting, at the output's scale.
 * @return the limited sample, within the ceiling.
 */
static int16_t softLimit(int32_t x)
{
    const int32_t a = (x < 0) ? -x : x;
    if (a <= AGC_KNEE)
        return static_cast<int16_t>(x);

    // Position on the curve, Q8: at most 38346 * 64 * 256, inside 32 bits
    const int32_t over = std::min<int32_t>(a - AGC_KNEE, AGC_CURVE_SPAN);
    const int32_t pos = (over * AGC_CURVE_STEPS * 256) / AGC_CURVE_SPAN;
    const int32_t i = pos >> 8;

    int32_t curve = AGC_CURVE[AGC_CURVE_STEPS];
    if (i < AGC_CURVE_STEPS) {
        const int32_t rise = AGC_CURVE[i + 1] - AGC_CURVE[i];
        curve = AGC_CURVE[i] + ((rise * (pos & 255)) >> 8);
    }

    const int32_t y = AGC_KNEE + (((AGC_CEILING - AGC_KNEE) * curve) >> 15);
    return static_cast<int16_t>((x < 0) ? -y : y);
}

/**
 * Update the noise floor with a block's level: the quietest block of the
 * last windows, as speech leaves gaps within a few hundred ms.
 *
 * @param agc: pointer to the AGC state.
 * @param blkDb: the block's level, in dBFS.
 */
static void agcTrackFloor(struct agc *agc, float blkDb)
{
    if ((agc->winCount == 0) || (blkDb < agc->winMin[0]))
        agc->winMin[0] = blkDb;

    float floor = agc->winMin[0];
    for (size_t i = 1; i < DSP_AGC_WINDOWS; i++)
        floor = std::min(floor, agc->winMin[i]);
    agc->floorDb = floor;

    agc->winCount++;
    if (agc->winCount >= AGC_WINDOW_BLOCKS) {
        agc->winCount = 0;
        for (size_t i = DSP_AGC_WINDOWS - 1; i > 0; i--)
            agc->winMin[i] = agc->winMin[i - 1];
    }
}

void dsp_agcInit(struct agc *agc, float targetDb, float baseGain)
{
    agc->targetDb = targetDb;
    agc->baseGain = baseGain;
    agc->gainDb = 0.0f;
    agc->levelDb = targetDb;
    agc->acquire = AGC_ACQUIRE_BLOCKS;
    dsp_agcRestart(agc);
}

void dsp_agcRestart(struct agc *agc)
{
    agc->floorDb = AGC_GATE_MIN_DB;
    // Above any noise: the first window decides
    for (size_t i = 0; i < DSP_AGC_WINDOWS; i++)
        agc->winMin[i] = 0.0f;
    agc->winCount = 0;
    agc->run = 0;
}

/**
 * Decide the gain for a block from its level: it falls on any speech block
 * over the target, rises only on confirmed speech, holds in pauses, and
 * falls on any block far over the target, gated or not, so a loud first
 * word after a restart is not left at a quiet talker's gain.
 *
 * @param agc: pointer to the AGC state.
 * @param blkDb: the block's level before the AGC, in dBFS.
 */
static void agcUpdateGain(struct agc *agc, float blkDb)
{
    const float gate = std::max(agc->floorDb + AGC_GATE_DB, AGC_GATE_MIN_DB);
    const bool speech = (blkDb > gate);
    if (speech)
        agc->run = std::min<uint16_t>(agc->run + 1, AGC_CONFIRM_BLOCKS);
    else
        agc->run = 0;

    // Below the gate the gain holds, so pauses keep their level
    if (speech) {
        const bool confirmed = (agc->run >= AGC_CONFIRM_BLOCKS);

        // Speech level: rises fast, falls slowly, so the gain follows
        // phrases rather than syllables
        if (confirmed) {
            float k = AGC_LEVEL_FALL;
            if (blkDb > agc->levelDb)
                k = AGC_LEVEL_RISE;
            agc->levelDb += k * (blkDb - agc->levelDb);
        }

        float want = agc->targetDb - agc->levelDb;
        want = std::min(std::max(want, AGC_MIN_GAIN_DB), AGC_MAX_GAIN_DB);
        if ((confirmed == false) && (want > agc->gainDb))
            want = agc->gainDb;

        // A block far over the target aims the gain at the target at once,
        // confirmed or not; the fall is still rate limited and the limiter
        // holds the peaks meanwhile
        const float over = (blkDb + agc->gainDb) - agc->targetDb;
        if ((over > AGC_FAST_DOWN_DB) && (want > agc->gainDb - over))
            want = agc->gainDb - over;

        if (want < agc->gainDb) {
            agc->gainDb -= std::min(agc->gainDb - want, AGC_ATTACK_DB);
        } else {
            float rise = AGC_RELEASE_DB;
            if (agc->acquire > 0)
                rise *= AGC_ACQUIRE_SPEEDUP;
            agc->gainDb += std::min(want - agc->gainDb, rise);
        }

        if (confirmed && (agc->acquire > 0))
            agc->acquire--;
    } else {
        // Not speech to the gate, but far over the target: lowering the
        // gain never lifts the noise, so it falls as it would on speech
        const float over = (blkDb + agc->gainDb) - agc->targetDb;
        if (over > AGC_FAST_DOWN_DB) {
            agc->gainDb = std::max(agc->gainDb - std::min(over, AGC_ATTACK_DB),
                                   AGC_MIN_GAIN_DB);
        }
    }
}

void dsp_agcProcess(struct agc *agc, int16_t *block, size_t len)
{
    const uint64_t energy = std::inner_product(block, block + len, block,
                                               uint64_t(0));

    const float power = static_cast<float>(energy) * agc->baseGain
                      * agc->baseGain / static_cast<float>(len);
    const float blkDb = 10.0f * log10f(power + AGC_POWER_EPS);
    agcTrackFloor(agc, blkDb);

    const float prevGainDb = agc->gainDb;
    agcUpdateGain(agc, blkDb);

    // Ramp the gain across the block, then limit: the result stays under
    // the ceiling, so it fits the samples without clipping. Gains in Q16.16,
    // from full scale back to samples, at most 132 946 432 at +24 dB with a
    // mic gain of 128; one division per block.
    const float toSamples = agc->baseGain * 32767.0f
                          * static_cast<float>(AGC_GAIN_ONE);
    const int32_t g0 = lrintf(dbToLin(prevGainDb) * toSamples);
    const int32_t g1 = lrintf(dbToLin(agc->gainDb) * toSamples);
    const int32_t step = (g1 - g0) / static_cast<int32_t>(len);

    int32_t g = g0;
    for (size_t i = 0; i < len; i++) {
        g = (i + 1 < len) ? (g + step) : g1;
        const int64_t x = (static_cast<int64_t>(block[i]) * g) / AGC_GAIN_ONE;
        block[i] = softLimit(static_cast<int32_t>(x));
    }
}
