/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Silvano Seva IU2KWO,                     *
 *                                Frederik Saraci IU2NRO                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef DSP_H
#define DSP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
 * Convert a floating point value to unsigned Q1.31 format.
 * To avoid conversion errors, the input value should range between 0 and 1.0
 */
#define float_to_UQ31(x) ((uint32_t)(x * 4294967296.0f))

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
 * Data structure holding the internal state of a power squelch gate.
 */
struct pwrSquelch {
    uint32_t prevOut;
    uint8_t decayCnt;
};

/**
 * Run a single step of the power squelch gate.
 * After the envelope of the signal power falls below the threshold, the gate
 * will effectively close after the specified number of decay steps.
 *
 * @param sq: pointer to state data structure.
 * @param sample: input sample.
 * @param filtAlpha: envelope filter time constant.
 * @param openTsh: squelch opening threshold.
 * @param closeTsh: squelch closing threshold.
 * @param decay: squelch decay steps.
 * @return the input sample or zero if the squelch is closed.
 */
int16_t dsp_pwrSquelch(struct pwrSquelch *sq, int16_t sample,
                       uint32_t filtAlpha, uint32_t openTsh, uint32_t closeTsh,
                       uint8_t decay);
/**
 * Apply the power quelch gate on collection of samples, processing data
 * in-place.
 *
 * @param sq: pointer to state data structure.
 * @param buffer: buffer containing the audio samples.
 * @param length: number of samples contained in the buffer.
 * @param filtAlpha: envelope filter time constant.
 * @param openTsh: squelch opening threshold.
 * @param closeTsh: squelch closing threshold.
 * @param decay: squelch decay steps.
 */
static inline void dsp_applyPwrSquelch(struct pwrSquelch *sq, int16_t *buffer,
                                       size_t length, uint32_t filtAlpha,
                                       uint32_t openTsh, uint32_t closeTsh,
                                       uint8_t decay)
{
    for (size_t i = 0; i < length; i++)
        buffer[i] =
            dsp_pwrSquelch(sq, buffer[i], filtAlpha, openTsh, closeTsh, decay);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DSP_H */
