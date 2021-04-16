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

/*
 * Applies a generic FIR filter on the audio buffer passed as parameter.
 * The buffer will be processed in place to save memory.
 */
template<size_t order>
void dsp_applyFIR(audio_sample_t *buffer,
                  uint16_t length,
                  std::array<float, order> taps)
{
    for(int i = length - 1; i >= 0; i--) {
        float acc = 0.0f;
        for(uint16_t j = 0; j < order; j++) {
            if (i >= j)
                acc += buffer[i - j] * taps[j];
        }
        buffer[i] = (audio_sample_t) acc;
    }
}

/*
 * Compensate for the filtering applied by the PWM output over the modulated
 * signal. The buffer will be processed in place to save memory.
 */
void dsp_pwmCompensate(audio_sample_t *buffer, uint16_t length)
{
    // FIR filter designed by Wojciech SP5WWP
    std::array<float, 5> taps = { 0.01f, -0.05f, 0.88, -0.05f, 0.01f };
    dsp_applyFIR(buffer, length, taps);
}

/*
 * Remove any DC bias from the audio buffer passed as parameter.
 * The buffer will be processed in place to save memory.
 */
void dsp_dcRemoval(audio_sample_t *buffer, uint16_t length)
{
    // Compute the average of all the samples
    float acc = 0.0f;
    for (int i = 0; i < length; i++) {
        acc += buffer[i];
    }
    float mean = acc / (float) length;

    // Subtract it to all the samples
    for (int i = 0; i < length; i++) {
        buffer[i] -= mean;
    }
}
