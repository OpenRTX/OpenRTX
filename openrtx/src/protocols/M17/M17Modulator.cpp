/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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
#include <cstddef>
#include <cstring>
#include <experimental/array>
#include <M17/M17Modulator.hpp>
#include <M17/M17Utils.hpp>
#include <M17/M17DSP.hpp>

#if defined(PLATFORM_LINUX)
#include <stdio.h>
#endif

using namespace M17;


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

    baseband_buffer = std::make_unique< int16_t[] >(2 * M17_FRAME_SAMPLES);
    idleBuffer      = baseband_buffer.get();
    txRunning       = false;
    #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    dsp_resetFilterState(&pwmFilterState);
    #endif
}

void M17Modulator::stop() {
    if(txRunning)
    {
        // Send EOT
        std::array<uint8_t, 46> eot_bytes;
        
        // Alternatively fill eot_bytes with 0x55, 0x5D as per
        // M17 spec. There's probably a more efficient way to do
        // this, as I'm relying on the compiler to unroll these.
        for (uint i=0; i<eot_bytes.size(); i+=2) {
            eot_bytes[i] = 0x55;
        }

        for (uint i=1; i<eot_bytes.size(); i+=2) {
            eot_bytes[i] = 0x5D;
        }

        send(EOT_SYNC_WORD, eot_bytes);

        outputStream_stop(outStream);
        outputStream_sync(outStream, false);
        txRunning  = false;
        idleBuffer = baseband_buffer.get();
        #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
        dsp_resetFilterState(&pwmFilterState);
        #endif
    }
}

void M17Modulator::terminate()
{
    // Terminate an ongoing stream, if present
    if(txRunning)
    {
        outputStream_terminate(outStream);
        txRunning = false;
    }

    // Deallocate memory.
    baseband_buffer.reset();
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

    generateBaseband();
    emitBaseband();

    // If last frame, signal stop of transmission
    if(isLast) stop();
}

void M17Modulator::generateBaseband()
{
    memset(idleBuffer, 0x00, M17_FRAME_SAMPLES * sizeof(stream_sample_t));

    for(size_t i = 0; i < symbols.size(); i++)
    {
        idleBuffer[i * 10] = symbols[i];
    }

    for(size_t i = 0; i < M17_FRAME_SAMPLES; i++)
    {
        float elem    = static_cast< float >(idleBuffer[i]);
        elem          = M17::rrc_48k(elem * M17_RRC_GAIN) - M17_RRC_OFFSET;
        idleBuffer[i] = static_cast< int16_t >(elem);
    }
}

#ifndef PLATFORM_LINUX
void M17Modulator::emitBaseband()
{
    #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    dsp_pwmCompensate(&pwmFilterState, idleBuffer, M17_FRAME_SAMPLES);
    dsp_invertPhase(idleBuffer, M17_FRAME_SAMPLES);
    #endif

    if(txRunning == false)
    {
        // First run, start transmission
        outStream = outputStream_start(SINK_RTX,
                                       PRIO_TX,
                                       baseband_buffer.get(),
                                       2*M17_FRAME_SAMPLES,
                                       BUF_CIRC_DOUBLE,
                                       M17_TX_SAMPLE_RATE);
        txRunning = true;
    }
    else
    {
        // Transmission is ongoing, syncronise with stream end before proceeding
        outputStream_sync(outStream, true);
    }

    idleBuffer = outputStream_getIdleBuffer(outStream);
}
#else
void M17Modulator::emitBaseband()
{
    FILE *outfile = fopen("/tmp/m17_output.raw", "ab");

    for(size_t i = 0; i < M17_FRAME_SAMPLES; i++)
    {
        auto s = idleBuffer[i];
        fwrite(&s, sizeof(s), 1, outfile);
    }

    fclose(outfile);
}
#endif
