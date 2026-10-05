/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/HorseModulator.hpp"
#include "protocols/horse/HorseUtils.hpp"
#include "protocols/M17/DSP.hpp"
#include <cstring>

#if defined(PLATFORM_LINUX)
#include <stdio.h>
#endif

namespace horse
{

HorseModulator::HorseModulator() : idleBuffer(nullptr), txRunning(false), invPhase(false)
{
}

HorseModulator::~HorseModulator()
{
    terminate();
}

void HorseModulator::init()
{
    baseband_buffer = std::make_unique<int16_t[]>(2 * FRAME_SAMPLES);
    idleBuffer      = baseband_buffer.get();
    txRunning       = false;
#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    pwmComp.reset();
#endif
}

void HorseModulator::terminate()
{
    if (txRunning)
    {
        audioStream_terminate(outStream);
        txRunning = false;
    }
    audioPath_release(outPath);
    baseband_buffer.reset();
}

bool HorseModulator::start()
{
    if (txRunning)
        return true;
#ifndef PLATFORM_LINUX
    outPath = audioPath_request(SOURCE_MCU, SINK_RTX, PRIO_TX);
    if (outPath < 0)
        return false;
    outStream = audioStream_start(outPath, baseband_buffer.get(),
                                  2 * FRAME_SAMPLES, TX_SAMPLE_RATE,
                                  STREAM_OUTPUT | BUF_CIRC_DOUBLE);
    if (outStream < 0)
        return false;
    idleBuffer = outputStream_getIdleBuffer(outStream);
#endif
    txRunning = true;
    return true;
}

void HorseModulator::sendPreamble()
{
    for (size_t i = 0; i < symbols.size(); i += 2)
    {
        symbols[i]     = +3;
        symbols[i + 1] = -3;
    }
    symbolsToBaseband();
    sendBaseband();
    symbolsToBaseband();
    sendBaseband();
}

void HorseModulator::sendFrame(const frame_t& frame)
{
    auto it = symbols.begin();
    for (size_t i = 0; i < frame.size(); i++)
    {
        auto sym = byteToSymbols(frame[i]);
        it       = std::copy(sym.begin(), sym.end(), it);
    }
    symbolsToBaseband();
    sendBaseband();
}

void HorseModulator::stop()
{
    if (!txRunning)
        return;
    audioStream_stop(outStream);
    txRunning  = false;
    idleBuffer = baseband_buffer.get();
    audioPath_release(outPath);
#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    pwmComp.reset();
#endif
}

void HorseModulator::invertPhase(bool status)
{
    invPhase = status;
}

void HorseModulator::symbolsToBaseband()
{
    std::memset(idleBuffer, 0x00, FRAME_SAMPLES * sizeof(stream_sample_t));
    for (size_t i = 0; i < symbols.size(); i++)
        idleBuffer[i * 10] = symbols[i];
    for (size_t i = 0; i < FRAME_SAMPLES; i++)
    {
        float elem = static_cast<float>(idleBuffer[i]);
        elem       = M17::rrc_48k(elem * RRC_GAIN) - RRC_OFFSET;
#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
        elem = pwmComp(elem);
#endif
        if (invPhase) elem = 0.0f - elem;
        idleBuffer[i] = static_cast<int16_t>(elem);
    }
}

#ifndef PLATFORM_LINUX
void HorseModulator::sendBaseband()
{
    if (!txRunning) return;
    if (audioPath_getStatus(outPath) != PATH_OPEN) return;
    outputStream_sync(outStream, true);
    idleBuffer = outputStream_getIdleBuffer(outStream);
}
#else
void HorseModulator::sendBaseband()
{
    FILE* outfile = fopen("/tmp/horse_output.raw", "ab");
    if (outfile)
    {
        for (size_t i = 0; i < FRAME_SAMPLES; i++)
            fwrite(&idleBuffer[i], sizeof(idleBuffer[i]), 1, outfile);
        fclose(outfile);
    }
}
#endif

}  // namespace horse
