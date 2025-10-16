/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <new>
#include <cstddef>
#include <cstring>
#include "protocols/M17/M17Modulator.hpp"
#include "protocols/M17/M17Utils.hpp"
#include "protocols/M17/M17DSP.hpp"

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
     * audio.
     */

    baseband_buffer = std::make_unique< int16_t[] >(2 * M17_FRAME_SAMPLES);
    idleBuffer      = baseband_buffer.get();
    txRunning       = false;
    #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    pwmComp.reset();
    #endif
}

void M17Modulator::terminate()
{
    // Terminate an ongoing stream, if present
    if(txRunning)
    {
        audioStream_terminate(outStream);
        txRunning = false;
    }

    // Always ensure that outgoing audio path is closed
    audioPath_release(outPath);

    // Deallocate memory.
    baseband_buffer.reset();
}

bool M17Modulator::start()
{
    if(txRunning)
        return true;

    #ifndef PLATFORM_LINUX
    outPath = audioPath_request(SOURCE_MCU, SINK_RTX, PRIO_TX);
    if(outPath < 0)
        return false;

    outStream = audioStream_start(outPath, baseband_buffer.get(),
                                  2*M17_FRAME_SAMPLES, M17_TX_SAMPLE_RATE,
                                  STREAM_OUTPUT | BUF_CIRC_DOUBLE);

    if(outStream < 0)
        return false;

    idleBuffer = outputStream_getIdleBuffer(outStream);
    #endif

    txRunning = true;

    return true;
}

void M17Modulator::sendPreamble()
{
    // Fill symbol buffer with preamble, made of alternated +3 and -3 symbols
    for(size_t i = 0; i < symbols.size(); i += 2)
    {
        symbols[i]     = +3;
        symbols[i + 1] = -3;
    }

    // Generate baseband signal and then start transmission
    symbolsToBaseband();
    sendBaseband();

    // Repeat baseband generation and transmission, this makes the preamble to
    // be long 80ms (two frames)
    symbolsToBaseband();
    sendBaseband();
}

void M17Modulator::sendFrame(const frame_t& frame)
{
    auto it = symbols.begin();
    for(size_t i = 0; i < frame.size(); i++)
    {
        auto sym = byteToSymbols(frame[i]);
        it       = std::copy(sym.begin(), sym.end(), it);
    }

    symbolsToBaseband();
    sendBaseband();
}

void M17Modulator::stop()
{
    if(txRunning == false)
        return;

    audioStream_stop(outStream);
    txRunning  = false;
    idleBuffer = baseband_buffer.get();
    audioPath_release(outPath);

    #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    pwmComp.reset();
    #endif
}

void M17Modulator::invertPhase(const bool status)
{
    invPhase = status;
}


void M17Modulator::symbolsToBaseband()
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
        #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
        elem          = pwmComp(elem);
        #endif
        if(invPhase) elem = 0.0f - elem;    // Invert signal phase
        idleBuffer[i] = static_cast< int16_t >(elem);
    }
}

#ifndef PLATFORM_LINUX
void M17Modulator::sendBaseband()
{
    if(txRunning == false) return;
    if(audioPath_getStatus(outPath) != PATH_OPEN) return;

    // Transmission is ongoing, syncronise with stream end before proceeding
    outputStream_sync(outStream, true);
    idleBuffer = outputStream_getIdleBuffer(outStream);
}
#else
void M17Modulator::sendBaseband()
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
