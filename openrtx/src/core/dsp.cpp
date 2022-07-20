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

#include <dsp.h>
#include <arm_math.h>

void dsp_resetFilterState(filter_state_t *state)
{
    state->u[0] = 0.0f;
    state->u[1] = 0.0f;
    state->u[2] = 0.0f;

    state->y[0] = 0.0f;
    state->y[1] = 0.0f;
    state->y[2] = 0.0f;

    state->initialised = false;
}

void dsp_pwmCompensate(filter_state_t *state, audio_sample_t *buffer,
                       size_t length)
{
    static constexpr float a =  4982680082321166792352.0f;
    static constexpr float b = -6330013275146484168000.0f;
    static constexpr float c =  1871109008789062500000.0f;
    static constexpr float d =  548027992248535162477.0f;
    static constexpr float e = -24496793746948241250.0f;
    static constexpr float f =  244617462158203125.0f;

    // Initialise filter with first two values, for smooth transient.
    if(length <= 2) return;

    if(state->initialised == false)
    {
        state->u[2] = static_cast< float >(buffer[0]);
        state->u[1] = static_cast< float >(buffer[1]);
        state->initialised = true;
    }

    for(size_t i = 2; i < length; i++)
    {
        state->u[0] = static_cast< float >(buffer[i]);
        state->y[0] = (a/d)*(state->u[0])
                    + (b/d)*(state->u[1])
                    + (c/d)*(state->u[2])
                    - (e/d)*(state->y[1])
                    - (f/d)*(state->y[2]);

        state->u[2] = state->u[1];
        state->u[1] = state->u[0];
        state->y[2] = state->y[1];
        state->y[1] = state->y[0];
        buffer[i] = static_cast< audio_sample_t >((state->y[0] * 0.5f) + 0.5f);
    }
}

void dsp_dcRemoval(filter_state_t *state, audio_sample_t *buffer, size_t length)
{
    /*
     * Removal of DC component performed using an high-pass filter with
     * transfer function G(z) = (z - 1)/(z - 0.999).
     * Recursive implementation of the filter is:
     * y(k) = u(k) - u(k-1) + 0.999*y(k-1)
     */

    if(length < 2) return;

    static constexpr float alpha = 0.999f;
    size_t pos = 0;

    if(state->initialised == false)
    {
        state->u[1] = static_cast< float >(buffer[0]);
        state->initialised = true;
        pos = 1;
    }

    for(; pos < length; pos++)
    {
        state->u[0] = static_cast< float >(buffer[pos]);
        state->y[0] = (state->u[0])
                    - (state->u[1])
                    + alpha * (state->y[1]);

        state->u[1] = state->u[0];
        state->y[1] = state->y[0];
        buffer[pos] = static_cast< audio_sample_t >(state->y[0] + 0.5f);
    }
}

void dsp_invertPhase(audio_sample_t *buffer, uint16_t length)
{
    for(uint16_t i = 0; i < length; i++)
    {
        buffer[i] = -buffer[i];
    }
}

#define BLOCK_SIZE            1
#define NUM_TAPS              58

static float32_t firStateF32[BLOCK_SIZE + NUM_TAPS - 1];

const float32_t firCoeffs32[NUM_TAPS] = {
-0.000121, 
-0.000005, 
0.000093, 
-0.000625, 
-0.000263, 
0.000584, 
-0.001485, 
-0.001354, 
0.001748, 
-0.002220, 
-0.004130, 
0.003482, 
-0.001634, 
-0.009381, 
0.004850, 
0.002169, 
-0.017308, 
0.003702, 
0.011635, 
-0.027075, 
-0.003848, 
0.030382, 
-0.036782, 
-0.026561, 
0.069964, 
-0.043995, 
-0.115682, 
0.287933, 
0.620000, 
0.287933, 
-0.115682, 
-0.043995, 
0.069964, 
-0.026561, 
-0.036782, 
0.030382, 
-0.003848, 
-0.027075, 
0.011635, 
0.003702, 
-0.017308, 
0.002169, 
0.004850, 
-0.009381, 
-0.001634, 
0.003482, 
-0.004130, 
-0.002220, 
0.001748, 
-0.001354, 
-0.001485, 
0.000584, 
-0.000263, 
-0.000625, 
0.000093, 
-0.000005, 
-0.000121

};

// float32_t  snr;

void dsp_lowPassFilter(audio_sample_t *buffer, uint16_t length)
{
    uint32_t i;
    arm_fir_instance_f32 S;
    arm_status status;
    uint32_t numBlocks = length/BLOCK_SIZE;

    float32_t filterbuf[256];

    float32_t  *inputF32, *outputF32;
    inputF32 = outputF32 = &filterbuf[0];

    // Loop through all buffer
    for(i = 0; i < length; i++)
    {
        filterbuf[i] = static_cast< float32_t >(buffer[i]);
    }

    /* Call FIR init function to initialize the instance structure. */
    arm_fir_init_f32(&S, NUM_TAPS, (float32_t *)&firCoeffs32[0], &firStateF32[0], BLOCK_SIZE);

    /* ----------------------------------------------------------------------
    ** Call the FIR process function for every BLOCK_SIZE samples
    ** ------------------------------------------------------------------- */

    for(i=0; i < numBlocks; i++)
    {
        arm_fir_f32(&S, filterbuf + (i * BLOCK_SIZE), filterbuf + (i * BLOCK_SIZE), BLOCK_SIZE);
    }

    for(i = 0; i < length; i++)
    {
        buffer[i] = static_cast< int16_t >(filterbuf[i]);
    }
}
