/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#include <new>
#include <dsp.h>
#include <cstddef>
#include <experimental/array>
#include <M17/M17Modulator.h>

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
#include <toneGenerator_MDx.h>
#elif defined(PLATFORM_LINUX)
#include <stdio.h>
#endif

/*
 * Coefficients for M17 RRC filter
 */
const auto rrc_taps = std::experimental::make_array<float>
(
    -0.009265784007800534,  -0.006136551625729697,  -0.001125978562075172,
     0.004891777252042491,   0.01071805138282269,    0.01505751553351295,
     0.01679337935001369,    0.015256245142156299,   0.01042830577908502,
     0.003031522725559901,  -0.0055333532968188165, -0.013403099825723372,
     -0.018598682349642525, -0.01944761739590459,   -0.015005271935951746,
     -0.0053887880354343935, 0.008056525910253532,   0.022816244158307273,
     0.035513467692208076,   0.04244131815783876,    0.04025481153629372,
     0.02671818654865632,    0.0013810216516704976, -0.03394615682795165,
    -0.07502635967975885,   -0.11540977897637611,   -0.14703962203941534,
    -0.16119995609538576,   -0.14969512896336504,   -0.10610329539459686,
    -0.026921412469634916,   0.08757875030779196,    0.23293327870303457,
     0.4006012210123992,     0.5786324696325503,     0.7528286479934068,
     0.908262741447522,      1.0309661131633199,     1.1095611856548013,
     1.1366197723675815,     1.1095611856548013,     1.0309661131633199,
     0.908262741447522,      0.7528286479934068,     0.5786324696325503,
     0.4006012210123992,     0.23293327870303457,    0.08757875030779196,
    -0.026921412469634916,  -0.10610329539459686,   -0.14969512896336504,
    -0.16119995609538576,   -0.14703962203941534,   -0.11540977897637611,
    -0.07502635967975885,   -0.03394615682795165,    0.0013810216516704976,
     0.02671818654865632,    0.04025481153629372,    0.04244131815783876,
     0.035513467692208076,   0.022816244158307273,   0.008056525910253532,
    -0.0053887880354343935, -0.015005271935951746,  -0.01944761739590459,
    -0.018598682349642525,  -0.013403099825723372,  -0.0055333532968188165,
     0.003031522725559901,   0.01042830577908502,    0.015256245142156299,
     0.01679337935001369,    0.01505751553351295,    0.01071805138282269,
     0.004891777252042491,  -0.001125978562075172,  -0.006136551625729697,
    -0.009265784007800534
);

/*
 * FIR implementation of the RRC filter for baseband audio generation.
 */
static Fir< std::tuple_size< decltype(rrc_taps) >::value > rrc(rrc_taps);


M17Modulator::M17Modulator()
{

}

M17Modulator::~M17Modulator()
{
    terminate();
}

void M17Modulator::init()
{
    /*
     * Allocate a chunk of memory to contain two complete buffers for baseband
     * audio. Split this chunk in two separate blocks for double buffering using
     * placement new.
     */

    baseband_buffer = new int16_t[2 * M17_FRAME_SAMPLES];
    idleBuffer      = new (baseband_buffer) dataBuffer_t;
    int16_t *ptr    = baseband_buffer + activeBuffer->size();
    activeBuffer    = new (ptr) dataBuffer_t;
    txRunning       = false;
}

void M17Modulator::terminate()
{
    // Delete the buffers and deallocate memory.
    activeBuffer->~dataBuffer_t();
    idleBuffer->~dataBuffer_t();
    delete[] baseband_buffer;
}

void M17Modulator::send(const std::array< uint8_t, 2 >& sync,
                        const std::array< uint8_t, 46 >& data,
                        const bool isLast)
{
    auto sync1 = byteToSymbols(sync[0]);
    auto sync2 = byteToSymbols(sync[1]);

    auto it = std::copy(sync1.begin(), sync1.end(), symbols.begin());
         it = std::copy(sync2.begin(), sync2.end(), it);

    for(size_t i = 0; i < data.size(); i++)
    {
        auto sym = byteToSymbols(data[i]);
        it       = std::copy(sym.begin(), sym.end(), it);
    }

    // If last frame, signal stop of transmission
    if(isLast) stopTx = true;

    generateBaseband();
    emitBaseband();
}

void M17Modulator::generateBaseband()
{
    idleBuffer->fill(0);
    for(size_t i = 0; i < symbols.size(); i++)
    {
        idleBuffer->at(i * 10) = symbols[i];
    }

    for(size_t i = 0; i < idleBuffer->size(); i++)
    {
        float elem = static_cast< float >(idleBuffer->at(i));
        idleBuffer->at(i) = static_cast< int16_t >(rrc(elem) * 7168.0);
    }
}

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
void M17Modulator::emitBaseband()
{
     dsp_pwmCompensate(idleBuffer->data(), idleBuffer->size());
     dsp_invertPhase(idleBuffer->data(), idleBuffer->size());

    for(size_t i = 0; i < M17_FRAME_SAMPLES; i++)
    {
        int32_t pos_sample = idleBuffer->at(i) + 32768;
        uint16_t shifted_sample = pos_sample >> 8;
        idleBuffer->at(i) = shifted_sample;
    }

    if(txRunning == false)
    {
        // First run, start transmission
        toneGen_playAudioStream(reinterpret_cast< uint16_t *>(baseband_buffer),
                                2*M17_FRAME_SAMPLES, M17_RTX_SAMPLE_RATE, true);
        txRunning = true;
        stopTx    = false;
    }
    else
    {
        // Transmission is ongoing, syncronise with stream end before proceeding
        toneGen_waitForStreamEnd();

        // Check if transmission stop is requested
        if(stopTx == true)
        {
            toneGen_stopAudioStream();
            stopTx    = false;
            txRunning = false;
        }
    }

    std::swap(idleBuffer, activeBuffer);
}
#elif defined(PLATFORM_LINUX)
void M17Modulator::emitBaseband()
{
    std::swap(idleBuffer, activeBuffer);

    FILE *outfile = fopen("/tmp/m17_output.raw", "ab");

    for(auto s : *activeBuffer)
    {
        fwrite(&s, sizeof(s), 1, outfile);
    }

    fclose(outfile);
}
#else
void M17Modulator::emitBaseband() { }
#endif
