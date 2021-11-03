/***************************************************************************
 *   Copyright (C) 2021 by Alain Carlucci                                  *
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
#include <cstdint>
#include <cstring>
#include <experimental/array>
#include <M17/M17IntegerModulator.h>

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
#include <toneGenerator_MDx.h>
#elif defined(PLATFORM_LINUX)
#include <stdio.h>
#endif

const auto jrrc_taps = std::experimental::make_array<int32_t>(
    -2125, -1407, -258, 1122, 2458, 3453, 3851, 3499, 2392, 695, -1269, -3074,
    -4266, -4460, -3441, -1236, 1847, 5233, 8145, 9735, 9233, 6128, 316, -7786,
    -17209, -26472, -33727, -36975, -34336, -24337, -6175, 20088, 53429, 91888,
    132724, 172680, 208333, 236478, 254506, 260713, 254506, 236478, 208333,
    172680, 132724, 91888, 53429, 20088, -6175, -24337, -34336, -36975, -33727,
    -26472, -17209, -7786, 316, 6128, 9233, 9735, 8145, 5233, 1847, -1236,
    -3441, -4460, -4266, -3074, -1269, 695, 2392, 3499, 3851, 3453, 2458, 1122,
    -258, -1407, -2125);

static IntegerFir<std::tuple_size<decltype(jrrc_taps)>::value> jrrc(jrrc_taps);

M17IntegerModulator::M17IntegerModulator() {}

M17IntegerModulator::~M17IntegerModulator() { terminate(); }

void M17IntegerModulator::init()
{
    /*
     * Allocate a chunk of memory to contain two complete buffers for baseband
     * audio. Split this chunk in two separate blocks for float buffering using
     * placement new.
     */

    baseband_buffer = new int16_t[2 * M17_FRAME_SAMPLES];
    activeBuffer    = new (baseband_buffer) dataBuffer_t;
    int16_t* ptr    = baseband_buffer + activeBuffer->size();
    idleBuffer      = new (ptr) dataBuffer_t;

    memset(baseband_buffer, 0, 2 * M17_FRAME_SAMPLES);
}

void M17IntegerModulator::terminate()
{
    // Delete the buffers and deallocate memory.
    activeBuffer->~dataBuffer_t();
    idleBuffer->~dataBuffer_t();
    delete[] baseband_buffer;
}

void M17IntegerModulator::send(const std::array<uint8_t, 2>& sync,
                         const std::array<uint8_t, 46>& data)
{
    auto sync1 = byteToSymbols(sync[0]);
    auto sync2 = byteToSymbols(sync[1]);

    auto it = std::copy(sync1.begin(), sync1.end(), symbols.begin());
    it      = std::copy(sync2.begin(), sync2.end(), it);

    for (size_t i = 0; i < data.size(); i++)
    {
        auto sym = byteToSymbols(data[i]);
        it       = std::copy(sym.begin(), sym.end(), it);
    }

    generateBaseband();
    emitBaseband();
}

void M17IntegerModulator::generateBaseband()
{
    auto& buf = *idleBuffer;
    buf.fill(0);
    for (size_t i = 0; i < symbols.size(); i++)
        buf[i * 10] = symbols[i];

    for (size_t i = 0; i < buf.size(); i++)
        buf[i] = jrrc(buf[i]);
}

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
void M17IntegerModulator::emitBaseband()
{
    dsp_pwmCompensate(idleBuffer->data(), idleBuffer->size());
    dsp_invertPhase(idleBuffer->data(), idleBuffer->size());

    for (size_t i = 0; i < M17_FRAME_SAMPLES; i++)
    {
        int32_t pos_sample      = idleBuffer->at(i) + 32768;
        uint16_t shifted_sample = pos_sample >> 8;
        idleBuffer->at(i)       = shifted_sample;
    }

    toneGen_waitForStreamEnd();
    std::swap(idleBuffer, activeBuffer);

    toneGen_playAudioStream(reinterpret_cast<uint16_t*>(activeBuffer->data()),
                            activeBuffer->size(), M17_RTX_SAMPLE_RATE);
}
#elif defined(PLATFORM_LINUX)
void M17IntegerModulator::emitBaseband()
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
void M17IntegerModulator::emitBaseband() {}
#endif
