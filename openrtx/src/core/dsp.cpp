/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/dsp.h"

int16_t dsp_dcBlockFilter(struct dcBlock *dcb, int16_t sample)
{
    /*
     * Implementation of a fixed-point DC block filter with noise shaping,
     * ensuring zero DC component at the output.
     * Filter pole set at 0.995
     *
     * Code adapted from https://dspguru.com/dsp/tricks/fixed-point-dc-blocking-filter-with-noise-shaping/
     */
    dcb->accum -= dcb->prevIn;
    dcb->prevIn = static_cast<int32_t>(sample) << 15;
    dcb->accum += dcb->prevIn;
    dcb->accum -= 164 * dcb->prevOut; // 32768.0 * (1.0 - pole)
    dcb->prevOut = dcb->accum >> 15;

    return static_cast<int16_t>(dcb->prevOut);
}

void dsp_oversamplingSetOversampling(
    struct oversamplingBlock *oversamplingBlock, uint8_t oversampling)
{
    oversamplingBlock->oversampling = oversampling;
}

bool dsp_oversamplingDecimate(struct oversamplingBlock *oversamplingBlock,
                              stream_sample_t *sample)
{
    oversamplingBlock->accumulator += (uint16_t)*sample;
    oversamplingBlock->count++;
    if (oversamplingBlock->count >= oversamplingBlock->oversampling) {
        // Integer division truncates toward zero; this is intentional as
        // rounding is unnecessary for audio signal averaging.
        *sample = oversamplingBlock->accumulator
                / oversamplingBlock->oversampling;
        oversamplingBlock->accumulator = 0;
        oversamplingBlock->count = 0;
        return true;
    }

    return false;
}
