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
 * FIR coefficients for compensation of the frequency response of the PWM
 * low-pass filter.
 * Tuned by looking to the spectrum of the signal after R340.
 */
/*static const std::array<float, 64> taps =
{
    -9.40491031717385e-005, -0.000848001423782242,  -0.00109696032444328,
    -0.00104009729188586,   -0.000822117903939996,  -0.000457814705725243,
    -0.000209097107096209,   1.45158986394931e-005,  6.14910127074833e-005,
     0.000143087188490481,   9.88939489392696e-005,  0.000152346137163944,
     7.61712103354242e-005,  0.000127707968067034,   2.69971997984194e-005,
     9.63336944939915e-005, -1.98038819464503e-005,  8.50942057427035e-005,
    -5.48180598514983e-005,  9.74029020847998e-005, -8.65051888156317e-005,
     0.0001371524305419,    -0.000126819029918826,   0.000216559008857725,
     0.000207354394275457,   0.000372153326183805,  -0.000415059574080058,
     0.000760696908477235,  -0.00111729560780114,    0.00235614927731939,
    -0.00580022821729115,    0.0279272947591763,     1.60023134906622,
     0.766395461102808,      0.216986452747805,     -0.273079694215707,
    -0.490419526390855,     -0.450207206012097,     -0.295047379903978,
    -0.150800551243769,     -0.0679868999770298,    -0.0316469951519529,
    -0.0126912958098696,     0.00516416837372523,    0.0203256371277265,
     0.0282234781085812,     0.0279427582235168,     0.0239288255039896,
     0.0196805882520023,     0.0165109464311636,     0.0131027879536979,
     0.008924413230651,      0.00420241867002624,    0.000241697547977826,
    -0.00252844621925366,   -0.0041180698440322,    -0.00521809743800184,
    -0.00591484004204968,   -0.0061514672437905,    -0.00549741278280048,
    -0.0040722701928889,    -0.00215228473946483,   -0.000386139756393536,
     0.000951056484550573
}; */

static const std::array<float, 64> taps =
{
    -0.000523167156726694,   0.0011010883971321,     0.00137838986287621,    0.00154512892396002,
	 0.00164637406044353,    0.00182032357388089,    0.00183742089014213,    0.0019581980575251,
	 0.00200211905193334,    0.00223640198563517,    0.00243762029193829,    0.00293708259023866,
	 0.00338509506821392,    0.00392794616065798,    0.00369014268727251,    0.00242284585262549,
	-0.000962709379976492,  -0.00563027044093947,   -0.0106836779806385,    -0.0130209630473438,
	-0.0128109914798342,    -0.011759245020922,     -0.0169113814383912,    -0.0296779098908434,
	-0.0495308061151908,    -0.0707359114441342,    -0.110771547555006,     -0.178326111879247,
	-0.254685981577775,     -0.206876574934482,      0.0606104323289596,     0.448825102293283,
	 1.79776537133947,       0.515990056880654,      0.0362534178282538,    -0.193821332018837,
	-0.243677746288118,     -0.190436093555995,     -0.108662517234115,     -0.0609557235768739,
	-0.0526066736887261,    -0.0560285946760313,    -0.0456932918793682,    -0.0218001098665789,
	 0.00089199887606847,    0.0124965610401737,     0.0138066528791882,     0.0112379235228154,
	 0.00903253903625927,    0.007957412451575,      0.00664904656540276,    0.00465156034945305,
	 0.00224743760019187,    0.000231912761000118,  -0.00142281678280983,   -0.00291263503433539,
	-0.00456669486638206,   -0.00602235828623709,   -0.00688862870804721,   -0.00674895444042032,
	-0.00583025587409893,   -0.00449830511160305,   -0.00328068437006111,   -0.00226857578272902
};

/*
 * Applies a generic FIR filter on the audio buffer passed as parameter.
 * The buffer will be processed in place to save memory.
 */
template<size_t order>
void dsp_applyFIR(audio_sample_t *src, uint16_t *dst, uint16_t length,
                  std::array<float, order> taps)
{
    for(int i = length - 1; i >= 0; i--)
    {
        float acc = 0.0f;
        for(uint16_t j = 0; j < order; j++)
        {
            if (i >= j)
            {
                float value = ((float) src[i - j]);     // Get buffer element
                value /= 2.9f;                          // Reduce amplitude to avoid saturation when filtering
                acc += value * taps[j];                 // Apply tap
            }
        }

        int16_t sample = 32768 - ((int16_t) acc);       // Convert to 8-bit 0-255 value for PWM signal generation
        dst[i] = ((uint16_t) sample) >> 8;
    }
}

/*
 * Compensate for the filtering applied by the PWM output over the modulated
 * signal. The buffer will be processed in place to save memory.
 */
void dsp_pwmCompensate(audio_sample_t *src, uint16_t *dst, uint16_t length)
{
    dsp_applyFIR(src, dst, length, taps);
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
