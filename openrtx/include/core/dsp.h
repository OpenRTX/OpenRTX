/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DSP_H
#define DSP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "interfaces/audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This header contains various DSP utilities which can be used to condition
 * input or output signals when implementing digital modes on OpenRTX.
 */

/**
 * Reset the state variables of a DSP object.
 *
 * @param state: state variable.
 */
#define dsp_resetState(state) memset(&state, 0x00, sizeof(state))

/**
 * Data structure holding the internal state of a DC blocking filter.
 */
struct dcBlock {
    int32_t accum;
    int32_t prevIn;
    int32_t prevOut;
};

/**
 * Run a single step of the DC blocking filter.
 *
 * @param dcb: pointer to DC filter state.
 * @param sample: input sample.
 * @return filtered sample
 */
int16_t dsp_dcBlockFilter(struct dcBlock *dcb, int16_t sample);

/**
 * Remove the DC offset from a collection of samples, processing data in-place.
 *
 * @param state: pointer to the data structure containing the filter state.
 * @param buffer: buffer containing the audio samples.
 * @param length: number of samples contained in the buffer.
 */
static inline void dsp_removeDcOffset(struct dcBlock *dcb, int16_t *buffer,
                                      const size_t length)
{
    for (size_t i = 0; i < length; i++)
        buffer[i] = dsp_dcBlockFilter(dcb, buffer[i]);
}

/**
 * Data structure holding the internal state of an oversampling filter.
 */
struct oversamplingBlock {
    uint8_t oversampling;
    uint8_t count;
    uint32_t accumulator;
};

/**
 * @brief Set the oversampling factor for a decimation filter.
 *
 * @param oversamplingBlock: pointer to the oversampling filter state.
 * @param oversampling: oversampling factor, must be >= 1.
 */
void dsp_oversamplingSetOversampling(
    struct oversamplingBlock *oversamplingBlock, uint8_t oversampling);

/**
 * @brief Run a single step of the oversampling decimation filter.
 *
 * Accumulates input samples and returns their truncated integer average
 * once the oversampling factor number of samples have been collected.
 *
 * Input samples are treated as unsigned values for accumulation. Callers
 * must ensure that the samples are non-negative (e.g. raw ADC readings)
 * before the DC offset has been removed.
 *
 * @param oversamplingBlock: pointer to the oversampling filter state.
 * @param sample: pointer to the input sample; overwritten with the
 * decimated output when the function returns true.
 * @return true when a decimated sample is ready, false otherwise.
 */
bool dsp_oversamplingDecimate(struct oversamplingBlock *oversamplingBlock,
                              stream_sample_t *sample);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DSP_H */
