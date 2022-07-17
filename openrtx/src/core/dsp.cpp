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

#include "core/dsp.h"

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
