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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <dsp.h>

void dsp_pwmCompensate(audio_sample_t *buffer, uint16_t length)
{
    // FIR filter designed by Wojciech SP5WWP
    std::array<float, 5> taps = { 0.01f, -0.05f, 0.88, -0.05f, 0.01f };
    dsp_applyFIR(buffer, length, taps);
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
