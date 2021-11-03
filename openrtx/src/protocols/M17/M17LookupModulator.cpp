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
#include <M17/M17LookupModulator.h>

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
#include <toneGenerator_MDx.h>
#elif defined(PLATFORM_LINUX)
#include <stdio.h>
#endif

static const std::array<std::array<int32_t, 79>, 5> irrc_taps = {{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // +1
    {-531,  -351,  -64,   280,   614,   863,   962,   874,   598,   173,
     -317,  -768,  -1066, -1115, -860,  -309,  461,   1308,  2036,  2433,
     2308,  1532,  79,    -1946, -4302, -6618, -8431, -9243, -8584, -6084,
     -1543, 5022,  13357, 22972, 33181, 43170, 52083, 59119, 63626, 65178,
     63626, 59119, 52083, 43170, 33181, 22972, 13357, 5022,  -1543, -6084,
     -8584, -9243, -8431, -6618, -4302, -1946, 79,    1532,  2308,  2433,
     2036,  1308,  461,   -309,  -860,  -1115, -1066, -768,  -317,  173,
     598,   874,   962,   863,   614,   280,   -64,   -351,  -531},

    // +3
    {-1593,  -1053,  -192,   840,    1842,   2589,   2886,   2622,   1794,
     519,    -951,   -2304,  -3198,  -3345,  -2580,  -927,   1383,   3924,
     6108,   7299,   6924,   4596,   237,    -5838,  -12906, -19854, -25293,
     -27729, -25752, -18252, -4629,  15066,  40071,  68916,  99543,  129510,
     156249, 177357, 190878, 195534, 190878, 177357, 156249, 129510, 99543,
     68916,  40071,  15066,  -4629,  -18252, -25752, -27729, -25293, -19854,
     -12906, -5838,  237,    4596,   6924,   7299,   6108,   3924,   1383,
     -927,   -2580,  -3345,  -3198,  -2304,  -951,   519,    1794,   2622,
     2886,   2589,   1842,   840,    -192,   -1053,  -1593},

    // -1
    {531,    351,    64,     -280,   -614,   -863,   -962,   -874,   -598,
     -173,   317,    768,    1066,   1115,   860,    309,    -461,   -1308,
     -2036,  -2433,  -2308,  -1532,  -79,    1946,   4302,   6618,   8431,
     9243,   8584,   6084,   1543,   -5022,  -13357, -22972, -33181, -43170,
     -52083, -59119, -63626, -65178, -63626, -59119, -52083, -43170, -33181,
     -22972, -13357, -5022,  1543,   6084,   8584,   9243,   8431,   6618,
     4302,   1946,   -79,    -1532,  -2308,  -2433,  -2036,  -1308,  -461,
     309,    860,    1115,   1066,   768,    317,    -173,   -598,   -874,
     -962,   -863,   -614,   -280,   64,     351,    531},

    // -3
    {1593,    1053,    192,     -840,    -1842,   -2589,   -2886,   -2622,
     -1794,   -519,    951,     2304,    3198,    3345,    2580,    927,
     -1383,   -3924,   -6108,   -7299,   -6924,   -4596,   -237,    5838,
     12906,   19854,   25293,   27729,   25752,   18252,   4629,    -15066,
     -40071,  -68916,  -99543,  -129510, -156249, -177357, -190878, -195534,
     -190878, -177357, -156249, -129510, -99543,  -68916,  -40071,  -15066,
     4629,    18252,   25752,   27729,   25293,   19854,   12906,   5838,
     -237,    -4596,   -6924,   -7299,   -6108,   -3924,   -1383,   927,
     2580,    3345,    3198,    2304,    951,     -519,    -1794,   -2622,
     -2886,   -2589,   -1842,   -840,    192,     1053,    1593},
}};

static LookupFir<79> irrc(irrc_taps);

M17LookupModulator::M17LookupModulator() {}

M17LookupModulator::~M17LookupModulator() { terminate(); }

void M17LookupModulator::init()
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

void M17LookupModulator::terminate()
{
    // Delete the buffers and deallocate memory.
    activeBuffer->~dataBuffer_t();
    idleBuffer->~dataBuffer_t();
    delete[] baseband_buffer;
}

void M17LookupModulator::send(const std::array<uint8_t, 2>& sync,
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

void M17LookupModulator::generateBaseband()
{
    auto& buf = *idleBuffer;
    buf.fill(0);
    for (size_t i = 0; i < symbols.size(); i++)
        buf[i * 10] = symbols[i];

    for (size_t i = 0; i < buf.size(); i++)
    {
        buf[i] = irrc(buf[i]);
    }
}

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
void M17LookupModulator::emitBaseband()
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
void M17LookupModulator::emitBaseband()
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
void M17LookupModulator::emitBaseband() {}
#endif
