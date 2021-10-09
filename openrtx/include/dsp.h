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

#ifndef DSP_H
#define DSP_H

#include <inttypes.h>
#include <stdlib.h>

typedef int16_t audio_sample_t;

/*
 * This header contains various DSP utilities which can be used to condition
 * input or output signals when implementing digital modes on OpenRTX.
 */

#ifdef __cplusplus

#include <array>
#include <cstddef>

extern "C" {
#endif

/**
 * Compensate for the filtering applied by the PWM output over the modulated
 * signal. The buffer is be processed in place to save memory.
 *
 * @param buffer: the buffer to be used as both source and destination.
 * @param length: the length of the input buffer.
 */
void dsp_pwmCompensate(audio_sample_t *buffer, size_t length);

/**
 * Remove the DC offset from a collection of audio samples, processing data
 * in-place.
 *
 * @param buffer: buffer containing the audio samples.
 * @param length: number of samples contained in the buffer.
 */
void dsp_dcRemoval(audio_sample_t *buffer, size_t length);

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

/**
 * Class for FIR filter with configurable coefficients.
 * Adapted from the original implementation by Rob Riggs, Mobilinkd LLC.
 */
template < size_t N >
class Fir
{
public:

    /**
     * Constructor.
     *
     * @param taps: reference to a std::array of floating poing values representing
     * the FIR filter coefficients.
     */
    Fir(const std::array< float, N >& taps) : taps(taps), pos(0)
    {
        reset();
    }

    /**
     * Destructor.
     */
    ~Fir() { }

    /**
     * Perform one step of the FIR filter, computing a new output value given
     * the input value and the history of previous input values.
     *
     * @param input: FIR input value for the current time step.
     * @return FIR output as a function of the current and past input values.
     */
    float operator()(const float& input)
    {
        hist[pos++] = input;
        if(pos >= N) pos = 0;

        float  result = 0.0;
        size_t index  = pos;

        for(size_t i = 0; i < N; i++)
        {
            index   = (index != 0 ? index - 1 : N - 1);
            result += hist[index] * taps[i];
        }

        return result;
    }

    /**
     * Reset FIR history, clearing the memory of past values.
     */
    void reset()
    {
        hist.fill(0);
        pos = 0;
    }

private:

    const std::array< float, N >& taps;    ///< FIR filter coefficients.
    std::array< float, N >        hist;    ///< History of past inputs.
    size_t                        pos;     ///< Current position in history.
};

#endif // __cplusplus

#endif /* DSP_H */
