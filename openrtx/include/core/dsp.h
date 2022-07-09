/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

typedef int16_t audio_sample_t;

/*
 * This header contains various DSP utilities which can be used to condition
 * input or output signals when implementing digital modes on OpenRTX.
 */

/**
 * Data structure holding the internal state of a filter.
 */
typedef struct
{
    float u[3];         // input values u(k), u(k-1), u(k-2)
    float y[3];         // output values y(k), y(k-1), y(k-2)
    bool  initialised;  // state variables initialised
}
filter_state_t;


/**
 * Reset the filter state variables.
 *
 * @param state: pointer to the data structure containing the filter state.
 */
void dsp_resetFilterState(filter_state_t *state);

/**
 * Compensate for the filtering applied by the PWM output over the modulated
 * signal. The buffer is be processed in place to save memory.
 *
 * @param state: pointer to the data structure containing the filter state.
 * @param buffer: the buffer to be used as both source and destination.
 * @param length: the length of the input buffer.
 */
void dsp_pwmCompensate(filter_state_t *state, audio_sample_t *buffer,
                       size_t length);

/**
 * Remove the DC offset from a collection of audio samples, processing data
 * in-place.
 *
 * @param state: pointer to the data structure containing the filter state.
 * @param buffer: buffer containing the audio samples.
 * @param length: number of samples contained in the buffer.
 */
void dsp_dcRemoval(filter_state_t *state, audio_sample_t *buffer, size_t length);

/*
 * Inverts the phase of the audio buffer passed as paramenter.
 * The buffer will be processed in place to save memory.
 *
 * @param buffer: the buffer to be used as both source and destination.
 * @param length: the length of the input buffer.
 */
void dsp_invertPhase(audio_sample_t *buffer, uint16_t length);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DSP_H */
