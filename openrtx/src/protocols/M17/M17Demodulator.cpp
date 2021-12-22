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

#include <M17/M17Demodulator.h>
#include <M17/M17DSP.h>
#include <M17/M17Utils.h>
#include <interfaces/audio_stream.h>
#include <math.h>

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
#include <toneGenerator_MDx.h>
#elif defined(PLATFORM_LINUX)
#include <stdio.h>
#endif

namespace M17
{

M17Demodulator::M17Demodulator()
{

}

M17Demodulator::~M17Demodulator()
{
    terminate();
}

void M17Demodulator::init()
{
    /*
     * Allocate a chunk of memory to contain two complete buffers for baseband
     * audio. Split this chunk in two separate blocks for double buffering using
     * placement new.
     */

    baseband_buffer = new int16_t[M17_FRAME_SAMPLES_24K];
    baseband        = { nullptr, 0 };
    activeFrame     = new dataFrame_t;
    rawFrame        = new uint16_t[M17_FRAME_SYMBOLS];
    idleFrame       = new dataFrame_t;
}

void M17Demodulator::terminate()
{
    // Delete the buffers and deallocate memory.
    delete[] baseband_buffer;
    delete activeFrame;
    delete[] rawFrame;
    delete idleFrame;
}

void M17Demodulator::startBasebandSampling()
{

    basebandId = inputStream_start(SOURCE_RTX, PRIO_RX,
                                   baseband_buffer,
                                   M17_INPUT_BUF_SIZE,
                                   BUF_CIRC_DOUBLE,
                                   M17_RX_SAMPLE_RATE);
}

void M17Demodulator::stopBasebandSampling()
{
     inputStream_stop(basebandId);
}

void M17Demodulator::resetCorrelationStats() {
    conv_ema = 0.0f;
    conv_emvar = 10000000000.0f;
}

/**
 * Algorithms taken from
 * https://fanf2.user.srcf.net/hermes/doc/antiforgery/stats.pdf
 */
void M17Demodulator::updateCorrelationStats(int32_t value)
{
    float delta = (float) value - conv_ema;
    float incr = conv_stats_alpha * delta;
    conv_ema += incr;
    conv_emvar = (1.0f - conv_stats_alpha) * (conv_emvar + delta * incr);
}

float M17Demodulator::getCorrelationStddev()
{
    return sqrt(conv_emvar);
}

void M17Demodulator::resetQuantizationStats() {
    qnt_max = 0.0f;
}

void M17Demodulator::updateQuantizationStats(uint32_t offset)
{
    auto value = baseband.data[offset];
    if (value > qnt_max) {
        qnt_max = value;
    } else
        qnt_max *= qnt_maxmin_alpha;
    if (value < qnt_min) {
        qnt_min = value;
    } else
        qnt_min *= qnt_maxmin_alpha;
}

float M17Demodulator::getQuantizationMax()
{
    return qnt_max;
}

float M17Demodulator::getQuantizationMin()
{
    return qnt_min;
}

int32_t M17Demodulator::convolution(size_t offset,
                                    int8_t *target,
                                    size_t target_size)
{
    // Compute convolution
    int32_t conv = 0;
    for(uint32_t i = 0; i < target_size; i++)
    {
        conv += (int32_t) target[i] *
            (int32_t) this->baseband.data[offset + i * M17_SAMPLES_PER_SYMBOL];
    }
    return conv;
}

sync_t M17Demodulator::nextFrameSync(uint32_t offset)
{
    sync_t syncword = { -1, false };
    // Find peaks in the correlation between the baseband and the frame syncword
    // Leverage the fact LSF syncword is the opposite of the frame syncword
    // to detect both syncwords at once.
    for(uint32_t i = offset; syncword.index == -1 && i < baseband.len; i++)
    {
        int32_t conv = convolution(i, stream_syncword, M17_SYNCWORD_SYMBOLS);
        updateCorrelationStats(conv);
        // Positive correlation peak -> frame syncword
        if (conv > getCorrelationStddev() * conv_threshold_factor)
        {
            syncword.lsf = false;
            syncword.index = i;
        }
        // Negative correlation peak -> LSF syncword
        else if (conv < -(getCorrelationStddev() * conv_threshold_factor))
        {
            syncword.lsf = true;
            syncword.index = i;
        }
    }
    return syncword;
}

int8_t M17Demodulator::quantize(int32_t offset)
{
    if (baseband.data[offset] > getQuantizationMax() * 2 / 3)
        return +3;
    else if (baseband.data[offset] < getQuantizationMin() * 2 / 3)
        return -3;
    else if (baseband.data[offset] > 0)
        return +1;
    else
        return -1;
}

const std::array<uint8_t, 48> &M17Demodulator::getFrame()
{
    return *activeFrame;
}

bool M17Demodulator::isFrameLSF()
{
    return isLSF;
}

void M17Demodulator::update()
{
    // Read samples from the ADC
    baseband = inputStream_getData(basebandId);

    if(baseband.data != NULL)
    {
        // Apply RRC on the baseband buffer
        for(size_t i = 0; i < baseband.len; i++)
        {
            float elem = static_cast< float >(baseband.data[i]);
            baseband.data[i] = static_cast< int16_t >(M17::rrc(elem) * 0.10);
        }

        // If we locked a syncword just demodulate samples
        if (locked)
        {

        }
        else // Otherwise find next syncword
        {
            M17::sync_t syncword = { -1, false };
            uint16_t offset = 0;
            syncword = nextFrameSync(offset);
            if (syncword.index != -1)
            {
                locked = true;
                // Set a flag to mark LSF
                isLSF = syncword.lsf;
                // Next syncword does not overlap with current syncword
                offset = syncword.index + M17_SAMPLES_PER_SYMBOL;
                // Slice the input buffer to extract a frame and quantize
                for(uint16_t i = 0; i < M17_FRAME_SYMBOLS; i++)
                {
                    // Quantize
                    uint16_t symbol_index = syncword.index + 2 +
                                            M17_SAMPLES_PER_SYMBOL * i;
                    updateQuantizationStats(baseband.data[symbol_index]);
                    int8_t symbol = quantize(symbol_index);
                    setSymbol<M17_FRAME_BYTES>(*activeFrame, i, symbol);
                    // If the frame buffer is full switch active and idle frame
                    if (rawFrameIndex == M17_FRAME_SYMBOLS)
                        std::swap(activeFrame, idleFrame);
                }
                // If we have some samples left, try to decode the syncword
                // If decoding fails, signal lock is lost
            }
        }
    }
}

} /* M17 */
