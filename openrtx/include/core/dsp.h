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
 * Data structure holding the internal state of a decimation block.
 */
struct decimatorState {
    int32_t accumulator;
    uint32_t count;
};

/**
 * @brief Run a single step of the oversampling decimation filter.
 *
 * Accumulates input samples and returns their truncated integer average
 * once the oversampling factor number of samples have been collected.
 *
 * @param state: pointer to the decimator state.
 * @param sample: pointer to the audio sample. Contains the value of the
 * decimated output when the function returns true.
 * @param ratio: sample decimation ratio.
 * @return true when a decimated sample is ready, false otherwise.
 */
bool dsp_decimator(struct decimatorState *state, int16_t *sample,
                   uint16_t ratio);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DSP_H */
