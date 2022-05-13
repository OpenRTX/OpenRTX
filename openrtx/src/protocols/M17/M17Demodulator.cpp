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
#include <math.h>
#include <cstring>
#include <stdio.h>
#include <deque>
#include <ringbuf.h>
#include <usb_vcom.h>

// #define ENABLE_DEMOD_LOG

using namespace M17;

#ifdef ENABLE_DEMOD_LOG
typedef struct
{
    int16_t sample;
    int32_t conv;
    float conv_th;
    int32_t sample_index;
    float qnt_pos_avg;
    float qnt_neg_avg;
    int32_t symbol;
    int32_t frame_index;
}
log_entry_t;

static RingBuffer< log_entry_t, 128 > logBuf;
static bool      logRunning;
static pthread_t logThread;

void *logFunc(void *arg)
{
    (void) arg;

    #ifdef PLATFORM_LINUX
    FILE *csv_log = fopen("demod_log.csv", "w");
    fprintf(csv_log, "Sample,Convolution,Threshold,Index,Max,Min,Symbol,I\n");
    #endif

    while(logRunning)
    {
        log_entry_t entry;
        logBuf.pop(entry, true);

        #ifdef PLATFORM_LINUX
        fprintf(csv_log, "%" PRId16 ",%d,%f,%d,%f,%f,%d,%d\n",
                entry.sample,
                entry.conv,
                entry.conv_th,
                entry.sample_index,
                entry.qnt_pos_avg,
                entry.qnt_neg_avg,
                entry.symbol,
                entry.frame_index);
        fflush(csv_log);
        #else
        vcom_writeBlock(&entry, sizeof(log_entry_t));
        #endif
    }

    #ifdef PLATFORM_LINUX
    fclose(csv_log);
    #endif

    return NULL;
}
#endif


M17Demodulator::M17Demodulator()
{

}

M17Demodulator::~M17Demodulator()
{
    // TODO
    // terminate();
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
    frame_index     = 0;
    phase           = 0;
    syncDetected    = false;
    locked          = false;
    newFrame        = false;

    #ifdef ENABLE_DEMOD_LOG
    logRunning = true;
    pthread_create(&logThread, NULL, logFunc, NULL);
    #endif
}

void M17Demodulator::terminate()
{
    // Delete the buffers and deallocate memory.
    delete[] baseband_buffer;
    delete activeFrame;
    delete[] rawFrame;
    delete idleFrame;

    #ifdef ENABLE_DEMOD_LOG
    logRunning = false;
    #endif
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
    resetQuantizationStats();
    // DC removal filter reset
    dsp_resetFilterState(&dsp_state);
}

void M17Demodulator::stopBasebandSampling()
{
     inputStream_stop(basebandId);
}

void M17Demodulator::resetCorrelationStats()
{
    conv_emvar = 40000000.0f;
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

void M17Demodulator::updateQuantizationStats(int32_t frame_index,
                                             int32_t symbol_index)
{
    int16_t sample = baseband.data[symbol_index];
    if (sample > 0)
        qnt_pos_fifo.push_front(sample);
    else
        qnt_neg_fifo.push_front(sample);
    // If we reached end of the syncword, compute average and reset queue
    if(frame_index == M17_SYNCWORD_SYMBOLS - 1)
    {
            int32_t acc = 0;
            for(auto e : qnt_pos_fifo)
                acc += e;
            qnt_pos_avg = acc / static_cast<float>(qnt_pos_fifo.size());
            acc = 0;
            for(auto e : qnt_neg_fifo)
                acc += e;
            qnt_neg_avg = acc / static_cast<float>(qnt_neg_fifo.size());
            qnt_pos_fifo.clear();
            qnt_neg_fifo.clear();
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

    sync_t syncword = { -1, false };
    // Find peaks in the correlation between the baseband and the frame syncword
    // Leverage the fact LSF syncword is the opposite of the frame syncword
    // to detect both syncwords at once. Stop early because convolution needs
    // access samples ahead of the starting offset.
    int32_t maxLen = static_cast < int32_t >(baseband.len - M17_BRIDGE_SIZE);
    for(int32_t i = offset; (syncword.index == -1) && (i < maxLen); i++)
    {
        int32_t conv = convolution(i, stream_syncword, M17_SYNCWORD_SYMBOLS);
        updateCorrelationStats(conv);

        #ifdef ENABLE_DEMOD_LOG
        // Log syncword search
        log_entry_t log =
        {
            (i < 0) ? basebandBridge[M17_BRIDGE_SIZE + i] : baseband.data[i],
            conv,
            CONV_THRESHOLD_FACTOR * getCorrelationStddev(),
            i,
            0.0,0.0,0,0
        };

        logBuf.push(log, false);
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

    return syncword;
}

int8_t M17Demodulator::quantize(int32_t offset)
{
    int16_t sample = 0;
    if (offset < 0) // When we are at negative offsets use bridge buffer
        sample = basebandBridge[M17_BRIDGE_SIZE + offset];
    else            // Otherwise use regular data buffer
        sample = baseband.data[offset];
    if (sample > static_cast< int16_t >(qnt_pos_avg / 2.0))
        return +3;
    else if (sample < static_cast< int16_t >(qnt_neg_avg / 2.0))
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
    int32_t offset = syncDetected ? 0 : -(int32_t) M17_BRIDGE_SIZE;
    uint16_t decoded_syms = 0;

    // Read samples from the ADC
    baseband = inputStream_getData(basebandId);

    // Apply DC removal filter
    dsp_dcRemoval(&dsp_state, baseband.data, baseband.len);

    if(baseband.data != NULL)
    {
        // Apply RRC on the baseband buffer
        for(size_t i = 0; i < baseband.len; i++)
        {
            float elem = static_cast< float >(baseband.data[i]);
            baseband.data[i] = static_cast< int16_t >(M17::rrc_24k(elem));
        }

        // Process the buffer
        while((syncword.index != -1) &&
              ((static_cast< int32_t >(M17_SAMPLES_PER_SYMBOL * decoded_syms) +
                offset + phase) < static_cast < int32_t >(baseband.len)))
        {

            // If we are not demodulating a syncword, search for one
            if (!syncDetected)
            {
                syncword = nextFrameSync(offset);
                if (syncword.index != -1) // Valid syncword found
                {
                    syncDetected = true;
                    offset = syncword.index + 1;
                    phase = 0;
                    frame_index = 0;
                }
            }
            // While we detected a syncword, demodulate available samples
            else
            {
                // Slice the input buffer to extract a frame and quantize
                int32_t symbol_index = offset
                                     + phase
                                     + (M17_SAMPLES_PER_SYMBOL * decoded_syms);
                // Update quantization stats only on syncwords
                if (frame_index < M17_SYNCWORD_SYMBOLS)
                    updateQuantizationStats(frame_index, symbol_index);
                int8_t symbol = quantize(symbol_index);

                #ifdef ENABLE_DEMOD_LOG
                // Log quantization
                for (int i = -2; i <= 2; i++)
                {
                    if ((symbol_index + i) >= 0 &&
                        (symbol_index + i) < static_cast<int32_t> (baseband.len))
                    {
                        log_entry_t log =
                        {
                            baseband.data[symbol_index + i],
                            0,0.0,symbol_index + i,
                            qnt_pos_avg / 2.0f,
                            qnt_neg_avg / 2.0f,
                            symbol,
                            frame_index
                        };

                        logBuf.push(log, false);
                    }
                }
                #endif

                setSymbol<M17_FRAME_BYTES>(*activeFrame, frame_index, symbol);
                decoded_syms++;
                frame_index++;

                // If the frame buffer is full switch active and idle frame
                if (frame_index == M17_FRAME_SYMBOLS)
                {
                    std::swap(activeFrame, idleFrame);
                    frame_index = 0;
                    newFrame   = true;
                }

                if (frame_index == M17_SYNCWORD_SYMBOLS)
                {

                    // If syncword is not valid, lock is lost, accept 2 bit errors
                    uint8_t hammingSync = hammingDistance((*activeFrame)[0],
                                                          stream_syncword_bytes[0])
                                        + hammingDistance((*activeFrame)[1],
                                                          stream_syncword_bytes[1]);

                    uint8_t hammingLsf = hammingDistance((*activeFrame)[0],
                                                         lsf_syncword_bytes[0])
                                       + hammingDistance((*activeFrame)[1],
                                                         lsf_syncword_bytes[1]);

                    // Too many errors in the syncword, lock is lost
                    if ((hammingSync > 4) && (hammingLsf > 4))
                    {
                        syncDetected = false;
                        locked = false;
                        std::swap(activeFrame, idleFrame);
                        frame_index = 0;
                        newFrame   = true;
                    }
                    // Correct syncword found
                    else
                        locked = true;
                }
            }
        }

        // We are at the end of the buffer
        if (syncDetected)
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

    return newFrame;
}
