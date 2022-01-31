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
#include <M17/M17DSP.h>

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
#include <toneGenerator_MDx.h>
#elif defined(PLATFORM_LINUX)
#include <stdio.h>
#endif

namespace M17
{

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

    baseband_buffer = new int16_t[2 * M17_FRAME_SAMPLES_48K];
    idleBuffer      = new (baseband_buffer) dataBuffer_t;
    int16_t *ptr    = baseband_buffer + M17_FRAME_SAMPLES_48K;
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
        idleBuffer->at(i) = static_cast< int16_t >(M17::rrc(elem) * 7168.0);
    }
}

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
void M17Modulator::emitBaseband()
{
     dsp_pwmCompensate(idleBuffer->data(), idleBuffer->size());
     dsp_invertPhase(idleBuffer->data(), idleBuffer->size());

    for(size_t i = 0; i < M17_FRAME_SAMPLES_48K; i++)
    {
        int32_t pos_sample = idleBuffer->at(i) + 32768;
        uint16_t shifted_sample = pos_sample >> 8;
        idleBuffer->at(i) = shifted_sample;
    }

    if(txRunning == false)
    {
        // First run, start transmission
        toneGen_playAudioStream(reinterpret_cast< uint16_t *>(baseband_buffer),
                                2*M17_FRAME_SAMPLES_48K, M17_TX_SAMPLE_RATE, true);
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

} /* M17 */
