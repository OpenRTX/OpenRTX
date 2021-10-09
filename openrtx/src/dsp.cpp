/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Silvano Seva IU2KWO,                            *
 *                         Frederik Saraci IU2NRO                          *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <dsp.h>

void dsp_pwmCompensate(audio_sample_t *buffer, size_t length)
{
    float u   = 0.0f;   // Current input value
    float y   = 0.0f;   // Current output value
    float uo  = 0.0f;   // u(k-1)
    float uoo = 0.0f;   // u(k-2)
    float yo  = 0.0f;   // y(k-1)
    float yoo = 0.0f;   // y(k-2)

    static constexpr float a =  4982680082321166792352.0f;
    static constexpr float b = -6330013275146484168000.0f;
    static constexpr float c =  1871109008789062500000.0f;
    static constexpr float d =  548027992248535162477.0f;
    static constexpr float e = -24496793746948241250.0f;
    static constexpr float f =  244617462158203125.0f;

    // Initialise filter with first two values, for smooth transient.
    if(length <= 2) return;
    uoo = static_cast< float >(buffer[0]);
    uo  = static_cast< float >(buffer[1]);

    for(size_t i = 2; i < length; i++)
    {
        u   = static_cast< float >(buffer[i]);
        y   = (a/d)*u + (b/d)*uo + (c/d)*uoo - (e/d)*yo - (f/d)*yoo;
        uoo = uo;
        uo  = u;
        yoo = yo;
        yo  = y;
        buffer[i] = static_cast< audio_sample_t >(y * 0.5f);
    }
}

void dsp_dcRemoval(audio_sample_t *buffer, size_t length)
{
    /*
     * Removal of DC component performed using an high-pass filter with
     * transfer function G(z) = (z - 1)/(z - 0.99).
     * Recursive implementation of the filter is:
     * y(k) = u(k) - u(k-1) + 0.99*y(k-1)
     */

    if(length < 2) return;

    audio_sample_t uo = buffer[0];
    audio_sample_t yo = 0;
    static constexpr float alpha = 0.99f;

    for(size_t i = 1; i < length; i++)
    {
        float yold = static_cast< float >(yo) * alpha;
        audio_sample_t u = buffer[i];
        buffer[i]  = u - uo + static_cast< audio_sample_t >(yold);
        uo = u;
        yo = buffer[i];
    }
}

void dsp_invertPhase(audio_sample_t *buffer, uint16_t length)
{
    for(uint16_t i = 0; i < length; i++)
    {
        buffer[i] = -buffer[i];
    }
}

template<size_t order>
void dsp_applyFIR(audio_sample_t *buffer,
                  uint16_t length,
                  std::array<float, order> taps)
{
    for(int i = length - 1; i >= 0; i--)
    {
        float acc = 0.0f;
        for(uint16_t j = 0; j < order; j++)
        {
            if (i >= j)
                acc += buffer[i - j] * taps[j];
        }
        buffer[i] = (audio_sample_t) acc;
    }
}
