/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Wojciech Kaczmarski SP5WWP               *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
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
#include <interfaces/gpio.h>
#include <math.h>
#include <cstring>
#include <stdio.h>
#include <interfaces/graphics.h>
#include <deque>

#ifdef PLATFORM_LINUX
#include <emulator.h>
#endif

using namespace M17;

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

    baseband_buffer = new int16_t[2 * M17_SAMPLE_BUF_SIZE];
    baseband        = { nullptr, 0 };
    activeFrame     = new frame_t;
    rawFrame        = new uint16_t[M17_FRAME_SYMBOLS];
    idleFrame       = new frame_t;
    frameIndex      = 0;
    phase           = 0;
    locked          = false;
    newFrame        = false;

    #ifdef PLATFORM_LINUX
    FILE *csv_log = fopen("demod_log.csv", "w");
    fprintf(csv_log, "Signal,Convolution,Threshold,Offset,Sample,Max,Min,Symbol,I\n");
    fclose(csv_log);
    #endif // PLATFORM_MOD17
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
                                   2 * M17_SAMPLE_BUF_SIZE,
                                   BUF_CIRC_DOUBLE,
                                   M17_RX_SAMPLE_RATE);
    // Clean start of the demodulation statistics
    resetCorrelationStats();
    // DC removal filter reset
    dsp_resetFilterState(&dsp_state);
}

void M17Demodulator::stopBasebandSampling()
{
     inputStream_stop(basebandId);
}

void M17Demodulator::resetCorrelationStats()
{
    conv_emvar = 80000000.0f;
}

/**
 * Algorithms taken from
 * https://fanf2.user.srcf.net/hermes/doc/antiforgery/stats.pdf
 */
void M17Demodulator::updateCorrelationStats(int32_t value)
{
    float incr  = CONV_STATS_ALPHA * static_cast<float>(value);
    conv_emvar  = (1.0f - CONV_STATS_ALPHA) * (conv_emvar + static_cast<float>(value) * incr);
}

float M17Demodulator::getCorrelationStddev()
{
    return sqrt(conv_emvar);
}

void M17Demodulator::resetQuantizationStats()
{
    qnt_pos_avg = 0.0f;
    qnt_neg_avg = 0.0f;
}

void M17Demodulator::updateQuantizationStats(int32_t offset)
{
    // If offset is negative use bridge buffer
    int16_t sample = 0;
    if (offset < 0) // When we are at negative offsets use bridge buffer
        sample = basebandBridge[M17_BRIDGE_SIZE + offset];
    else            // Otherwise use regular data buffer
        sample = baseband.data[offset];
    if (sample > 0)
    {
        // If the FIFO is not full just push a new sample
        if (qnt_pos_fifo.size() >= QNT_SMA_WINDOW)
        {
            qnt_pos_avg += 1 / static_cast<float>(QNT_SMA_WINDOW) *
                           (sample - qnt_pos_fifo.back());
            qnt_pos_fifo.pop_back();
        }
        qnt_pos_fifo.push_front(sample);
    }
    else
    {
        // If the FIFO is not full just push a new sample
        if (qnt_neg_fifo.size() >= QNT_SMA_WINDOW)
        {
            qnt_neg_avg += 1 / static_cast<float>(QNT_SMA_WINDOW) *
                           (sample - qnt_neg_fifo.back());
            qnt_neg_fifo.pop_back();
        }
        qnt_neg_fifo.push_front(sample);
    }
}

int32_t M17Demodulator::convolution(int32_t offset,
                                    int8_t *target,
                                    size_t target_size)
{
    // Compute convolution
    int32_t conv = 0;
    for(uint32_t i = 0; i < target_size; i++)
    {
        int32_t sample_index = offset + i * M17_SAMPLES_PER_SYMBOL;
        int16_t sample = 0;
        // When we are at negative indices use bridge buffer
        if (sample_index < 0)
            sample = basebandBridge[M17_BRIDGE_SIZE + sample_index];
        else
            sample = baseband.data[sample_index];
        conv += (int32_t) target[i] * (int32_t) sample;
    }
    return conv;
}

sync_t M17Demodulator::nextFrameSync(int32_t offset)
{
    #ifdef PLATFORM_LINUX
    FILE *csv_log = fopen("demod_log.csv", "a");
    #endif

    sync_t syncword = { -1, false };
    // Find peaks in the correlation between the baseband and the frame syncword
    // Leverage the fact LSF syncword is the opposite of the frame syncword
    // to detect both syncwords at once. Stop early because convolution needs
    // access samples ahead of the starting offset.
    int32_t maxLen = static_cast < int32_t >(baseband.len - M17_BRIDGE_SIZE);
    for(int32_t i = offset; (syncword.index == -1) && (i < maxLen); i++)
    {
        // If we are not locked search for a syncword
        int32_t conv = convolution(i, stream_syncword, M17_SYNCWORD_SYMBOLS);
        updateCorrelationStats(conv);
        updateQuantizationStats(i);

        #ifdef PLATFORM_LINUX
        int16_t sample = 0;
        if (i < 0) // When we are at negative offsets use bridge buffer
            sample = basebandBridge[M17_BRIDGE_SIZE + i];
        else            // Otherwise use regular data buffer
            sample = baseband.data[i];

        fprintf(csv_log, "%" PRId16 ",%d,%f,%d,%" PRId16 ",%f,%f,%d,%d\n",
                sample,
                conv / 10,
                CONV_THRESHOLD_FACTOR * getCorrelationStddev() / 10,
                i,
                0,0.0,0.0,0,0);
        fflush(csv_log);
        #endif

        // Positive correlation peak -> frame syncword
        if (conv > (getCorrelationStddev() * CONV_THRESHOLD_FACTOR))
        {
            syncword.lsf = false;
            syncword.index = i;
        }
        // Negative correlation peak -> LSF syncword
        else if (conv < -(getCorrelationStddev() * CONV_THRESHOLD_FACTOR))
        {
            syncword.lsf = true;
            syncword.index = i;
        }
    }

    #ifdef PLATFORM_LINUX
    fclose(csv_log);
    #endif
    return syncword;
}

int8_t M17Demodulator::quantize(int32_t offset)
{
    int16_t sample = 0;
    if (offset < 0) // When we are at negative offsets use bridge buffer
        sample = basebandBridge[M17_BRIDGE_SIZE + offset];
    else            // Otherwise use regular data buffer
        sample = baseband.data[offset];
    if (sample > static_cast< int16_t >(qnt_pos_avg * 0.067))
        return +3;
    else if (sample < static_cast< int16_t >(qnt_neg_avg * 0.067))
        return -3;
    else if (sample > 0)
        return +1;
    else
        return -1;
}

const frame_t& M17Demodulator::getFrame()
{
    // When a frame is read is not new anymore
    newFrame = false;
    return *activeFrame;
}

bool M17Demodulator::isFrameLSF()
{
    return isLSF;
}

bool M17::M17Demodulator::isLocked()
{
    return locked;
}

uint8_t M17Demodulator::hammingDistance(uint8_t x, uint8_t y)
{
    return __builtin_popcount(x ^ y);
}

bool M17Demodulator::update()
{
    M17::sync_t syncword = { 0, false };
    int32_t offset = locked ? 0 : -(int32_t) M17_BRIDGE_SIZE;
    uint16_t decoded_syms = 0;

    // Read samples from the ADC
    baseband = inputStream_getData(basebandId);

    // Apply DC removal filter
    dsp_dcRemoval(&dsp_state, baseband.data, baseband.len);

    #ifdef PLATFORM_LINUX
    FILE *csv_log = fopen("demod_log.csv", "a");
    #endif

    if(baseband.data != NULL)
    {
        // Apply RRC on the baseband buffer
        for(size_t i = 0; i < baseband.len; i++)
        {
            float elem = static_cast< float >(baseband.data[i]);
            baseband.data[i] = static_cast< int16_t >(M17::rrc(elem));
        }

        // Process the buffer
        while((syncword.index != -1) &&
              ((static_cast< int32_t >(M17_SAMPLES_PER_SYMBOL * decoded_syms) +
                offset + phase) < static_cast < int32_t >(baseband.len)))
        {

            // If we are not locked search for a syncword
            if (!locked)
            {
                syncword = nextFrameSync(offset);
                if (syncword.index != -1) // Valid syncword found
                {
                    locked = true;
                    isLSF  = syncword.lsf;
                    offset = syncword.index + 4;
                    phase = 0;
                    frameIndex = 0;
                }
            }
            // While we are locked, demodulate available samples
            else
            {
                // Slice the input buffer to extract a frame and quantize
                int32_t symbol_index = offset
                                     + phase
                                     + (M17_SAMPLES_PER_SYMBOL * decoded_syms);

                updateQuantizationStats(symbol_index);
                int8_t symbol = quantize(symbol_index);

                #ifdef PLATFORM_LINUX
                for (int i = 0; i < 4; i++)
                {
                    if ((symbol_index - 4 + i) >= 0)
                    fprintf(csv_log, "%" PRId16 ",%d,%f,%d,%" PRId16 ",%f,%f,%d,%d\n",
                            0,0,0.0,symbol_index - 4 + i,
                            baseband.data[symbol_index - 4 + i],
                            qnt_pos_avg,
                            qnt_neg_avg,
                            0,
                            frameIndex);
                }
                fprintf(csv_log, "%" PRId16 ",%d,%f,%d,%" PRId16 ",%f,%f,%d,%d\n",
                        0,0,0.0,symbol_index,
                        baseband.data[symbol_index],
                        qnt_pos_avg,
                        qnt_neg_avg,
                        symbol * 666,
                        frameIndex);
                for (int i = 0; i < 5; i++)
                {
                    if ((symbol_index + i + 1) < static_cast<int32_t> (baseband.len))
                    fprintf(csv_log, "%" PRId16 ",%d,%f,%d,%" PRId16 ",%f,%f,%d,%d\n",
                            0,0,0.0,symbol_index + i + 1,
                            baseband.data[symbol_index + i + 1],
                            qnt_pos_avg,
                            qnt_neg_avg,
                            0,
                            frameIndex);
                }
                fflush(csv_log);
                #endif

                setSymbol<M17_FRAME_BYTES>(*activeFrame, frameIndex, symbol);
                decoded_syms++;
                frameIndex++;

                // If the frame buffer is full switch active and idle frame
                if (frameIndex == M17_FRAME_SYMBOLS)
                {
                    std::swap(activeFrame, idleFrame);
                    frameIndex = 0;
                    newFrame   = true;
                }

                // If syncword is not valid, lock is lost, accept 2 bit errors
                uint8_t hammingSync = hammingDistance((*activeFrame)[0],
                                                      stream_syncword_bytes[0])
                                    + hammingDistance((*activeFrame)[1],
                                                      stream_syncword_bytes[1]);

                uint8_t hammingLsf = hammingDistance((*activeFrame)[0],
                                                     lsf_syncword_bytes[0])
                                   + hammingDistance((*activeFrame)[1],
                                                     lsf_syncword_bytes[1]);

                if (frameIndex == M17_SYNCWORD_SYMBOLS)
                {
                    // Too many errors in the syncword, lock is lost
                    if ((hammingSync > 1) && (hammingLsf > 1))
                    {
                        locked = false;
                        std::swap(activeFrame, idleFrame);
                        frameIndex = 0;
                        newFrame   = true;

                        #ifdef PLATFORM_MOD17
                        gpio_clearPin(SYNC_LED);
                        #endif // PLATFORM_MOD17
                    }
                    // Correct syncword found
                    else
                    {
                        #ifdef PLATFORM_MOD17
                        gpio_setPin(SYNC_LED);
                        #endif // PLATFORM_MOD17

                    }
                }
            }
        }

        // We are at the end of the buffer
        if (locked)
        {
            // Compute phase of next buffer
            phase = (static_cast<int32_t> (phase) + offset + baseband.len) % M17_SAMPLES_PER_SYMBOL;
        }
        else
        {
            // Copy last N samples to bridge buffer
            memcpy(basebandBridge,
                   baseband.data + (baseband.len - M17_BRIDGE_SIZE),
                   sizeof(int16_t) * M17_BRIDGE_SIZE);
        }
    }

    #ifdef PLATFORM_LINUX
    fclose(csv_log);
    #endif

    return newFrame;
}
