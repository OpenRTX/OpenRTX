/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#include <M17/M17Demodulator.hpp>
#include <M17/M17DSP.hpp>
#include <M17/M17Utils.hpp>
#include <audio_stream.h>
#include <math.h>
#include <cstring>
#include <stdio.h>

using namespace M17;

#ifdef ENABLE_DEMOD_LOG

#include <ringbuf.hpp>
#include <atomic>
#ifndef PLATFORM_LINUX
#include <usb_vcom.h>
#endif

typedef struct
{
    int16_t sample;
    int32_t conv;
    float   conv_th;
    int32_t sample_index;
    float   qnt_pos_avg;
    float   qnt_neg_avg;
    int32_t symbol;
    int32_t frame_index;
    uint8_t flags;
    uint8_t _empty;
}
__attribute__((packed)) log_entry_t;

#ifdef PLATFORM_LINUX
#define LOG_QUEUE 160000
#else
#define LOG_QUEUE 1024
#endif

static RingBuffer< log_entry_t, LOG_QUEUE > logBuf;
static std::atomic_bool dumpData;
static bool      logRunning;
static bool      trigEnable;
static bool      triggered;
static uint32_t  trigCnt;
static pthread_t logThread;

static void *logFunc(void *arg)
{
    (void) arg;

    #ifdef PLATFORM_LINUX
    FILE *csv_log = fopen("demod_log.csv", "w");
    fprintf(csv_log, "Sample,Convolution,Threshold,Index,Max,Min,Symbol,I,Flags\n");
    #endif

    uint8_t emptyCtr = 0;

    while(logRunning)
    {
        if(dumpData)
        {
            // Log up to four entries filled with zeroes before terminating
            // the dump.
            log_entry_t entry;
            memset(&entry, 0x00, sizeof(log_entry_t));
            if(logBuf.pop(entry, false) == false) emptyCtr++;

            if(emptyCtr >= 100)
            {
                dumpData = false;
                emptyCtr = 0;
                #ifdef PLATFORM_LINUX
                logRunning = false;
                #endif
            }

            #ifdef PLATFORM_LINUX
            fprintf(csv_log, "%d,%d,%f,%d,%f,%f,%d,%d,%d\n",
                    entry.sample,
                    entry.conv,
                    entry.conv_th,
                    entry.sample_index,
                    entry.qnt_pos_avg,
                    entry.qnt_neg_avg,
                    entry.symbol,
                    entry.frame_index,
                    entry.flags);
            fflush(csv_log);
            #else
            vcom_writeBlock(&entry, sizeof(log_entry_t));
            #endif
        }
    }

    #ifdef PLATFORM_LINUX
    fclose(csv_log);
    exit(-1);
    #endif

    return NULL;
}

static inline void pushLog(const log_entry_t& e)
{
    /*
     * 1) do not push data to log while dump is in progress
     * 2) if triggered, increase the counter
     * 3) fill half of the buffer with entries after the trigger, then start dump
     * 4) if buffer is full, erase the oldest element
     * 5) push data without blocking
     */

    if(dumpData) return;
    if(triggered) trigCnt++;
    if(trigCnt >= LOG_QUEUE/2)
    {
        dumpData  = true;
        triggered = false;
        trigCnt   = 0;
    }
    if(logBuf.full()) logBuf.eraseElement();
    logBuf.push(e, false);
}

#endif


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

    baseband_buffer = std::make_unique< int16_t[] >(2 * M17_SAMPLE_BUF_SIZE);
    demodFrame      = std::make_unique< frame_t >();
    readyFrame      = std::make_unique< frame_t >();
    baseband        = { nullptr, 0 };
    frame_index     = 0;
    phase           = 0;
    syncDetected    = false;
    locked          = false;
    newFrame        = false;

    #ifdef ENABLE_DEMOD_LOG
    logRunning = true;
    triggered  = false;
    dumpData   = false;
    trigEnable = false;
    trigCnt    = 0;
    pthread_create(&logThread, NULL, logFunc, NULL);
    #endif
}

void M17Demodulator::terminate()
{
    // Ensure proper termination of baseband sampling
    audioPath_release(basebandPath);
    audioStream_terminate(basebandId);

    // Delete the buffers and deallocate memory.
    baseband_buffer.reset();
    demodFrame.reset();
    readyFrame.reset();

    #ifdef ENABLE_DEMOD_LOG
    logRunning = false;
    #endif
}

void M17Demodulator::startBasebandSampling()
{
    basebandPath = audioPath_request(SOURCE_RTX, SINK_MCU, PRIO_RX);
    basebandId = audioStream_start(basebandPath, baseband_buffer.get(),
                                   2 * M17_SAMPLE_BUF_SIZE, M17_RX_SAMPLE_RATE,
                                   STREAM_INPUT | BUF_CIRC_DOUBLE);

    // Clean start of the demodulation statistics
    resetCorrelationStats();
    resetQuantizationStats();
    // DC removal filter reset
    dsp_resetFilterState(&dsp_state);
}

void M17Demodulator::stopBasebandSampling()
{
    audioStream_terminate(basebandId);
    audioPath_release(basebandPath);
    phase = 0;
    syncDetected = false;
    locked = false;
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
    int16_t sample = 0;
    // When we are at negative indices use bridge buffer
    if (symbol_index < 0)
        sample = basebandBridge[M17_BRIDGE_SIZE + symbol_index];
    else
        sample = baseband.data[symbol_index];
    if (sample > 0)
    {
        qnt_pos_acc += sample;
        qnt_pos_cnt++;
    }
    else
    {
        qnt_neg_acc += sample;
        qnt_neg_cnt++;
    }
    // If we reached end of the syncword, compute average and reset queue
    if(frame_index == M17_SYNCWORD_SYMBOLS - 1)
    {
        qnt_pos_avg = qnt_pos_acc / static_cast<float>(qnt_pos_cnt);
        qnt_neg_avg = qnt_neg_acc / static_cast<float>(qnt_neg_cnt);
        qnt_pos_acc = 0;
        qnt_neg_acc = 0;
        qnt_pos_cnt = 0;
        qnt_neg_cnt = 0;
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
    int32_t maxLen = static_cast < int32_t >(baseband.len - M17_SYNCWORD_SAMPLES);
    for(int32_t i = offset; (syncword.index == -1) && (i < maxLen); i++)
    {
        int32_t conv = convolution(i, stream_syncword, M17_SYNCWORD_SYMBOLS);
        updateCorrelationStats(conv);

        #ifdef ENABLE_DEMOD_LOG
        log_entry_t log;
        log.sample       = (i < 0) ? basebandBridge[M17_BRIDGE_SIZE + i] : baseband.data[i];
        log.conv         = conv;
        log.conv_th      = CONV_THRESHOLD_FACTOR * getCorrelationStddev();
        log.sample_index = i;
        log.qnt_pos_avg  = 0.0;
        log.qnt_neg_avg  = 0.0;
        log.symbol       = 0;
        log.frame_index  = 0;
        log.flags        = 1;

        pushLog(log);
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
    if (sample > static_cast< int16_t >(qnt_pos_avg / 1.5f))
        return +3;
    else if (sample < static_cast< int16_t >(qnt_neg_avg / 1.5f))
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
    return *readyFrame;
}

bool M17Demodulator::isLocked()
{
    return locked;
}

int32_t M17Demodulator::syncwordSweep(int32_t offset)
{
    int32_t max_conv = 0, max_index = 0;
    // Start from 5 samples behind, end 5 samples after
    for(int i = -SYNC_SWEEP_WIDTH; i <= SYNC_SWEEP_WIDTH; i++)
    {
        // TODO: Extend for LSF and BER syncwords
        int32_t conv = convolution(offset + i,
                                   stream_syncword,
                                   M17_SYNCWORD_SYMBOLS);
        #ifdef ENABLE_DEMOD_LOG
        int16_t sample;
        if (offset + i < 0)
            sample = basebandBridge[M17_BRIDGE_SIZE + offset + i];
        else
            sample = baseband.data[offset + i];

        log_entry_t log;
        log.sample       = sample;
        log.conv         = conv;
        log.conv_th      = 0.0;
        log.sample_index = offset + i;
        log.qnt_pos_avg  = 0.0;
        log.qnt_neg_avg  = 0.0;
        log.symbol       = 0;
        log.frame_index  = 0;
        log.flags        = 2;

        pushLog(log);
        #endif

        if (conv > max_conv)
        {
            max_conv = conv;
            max_index = i;
        }
    }

    return max_index;
}

bool M17Demodulator::update()
{
    sync_t syncword = { 0, false };
    phase = (syncDetected) ? phase % M17_SAMPLES_PER_SYMBOL : -M17_BRIDGE_SIZE;
    uint16_t decoded_syms = 0;

    // Read samples from the ADC
    if(audioPath_getStatus(basebandPath) != PATH_OPEN) return false;
    baseband = inputStream_getData(basebandId);

    if(baseband.data != NULL)
    {
        // Apply DC removal filter
        dsp_dcRemoval(&dsp_state, baseband.data, baseband.len);

        // Apply RRC on the baseband buffer
        for(size_t i = 0; i < baseband.len; i++)
        {
            float elem = static_cast< float >(baseband.data[i]);
            if(invPhase) elem = 0.0f - elem;
            baseband.data[i]  = static_cast< int16_t >(M17::rrc_24k(elem));
        }

        // Process the buffer
        while(syncword.index != -1)
        {

            // If we are not demodulating a syncword, search for one
            if (syncDetected == false)
            {
                syncword = nextFrameSync(phase);

                if (syncword.index != -1) // Valid syncword found
                {
                    phase = syncword.index + 1;
                    syncDetected = true;
                    frame_index  = 0;
                    decoded_syms = 0;
                }
            }
            // While we detected a syncword, demodulate available samples
            else
            {
                // Slice the input buffer to extract a frame and quantize
                int32_t symbol_index = phase
                    + (M17_SAMPLES_PER_SYMBOL * decoded_syms);
                if (symbol_index >= static_cast<int32_t>(baseband.len))
                    break;
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
                        log_entry_t log;
                        log.sample       = baseband.data[symbol_index + i];
                        log.conv         = phase;
                        log.conv_th      = 0.0;
                        log.sample_index = symbol_index + i;
                        log.qnt_pos_avg  = qnt_pos_avg / 1.5f;
                        log.qnt_neg_avg  = qnt_neg_avg / 1.5f;
                        log.symbol       = symbol;
                        log.frame_index  = frame_index;
                        log.flags        = 3;
                        if(i == 0) log.flags += 8;

                        pushLog(log);
                    }
                }
                #endif

                setSymbol(*demodFrame, frame_index, symbol);
                decoded_syms++;
                frame_index++;

                if (frame_index == M17_SYNCWORD_SYMBOLS)
                {
                    /*
                     * Check for valid syncword using hamming distance.
                     * The demodulator switches to locked state only if there
                     * is an exact syncword match, this avoids continuous false
                     * detections in absence of an M17 signal.
                     */
                    uint8_t maxHamming = 2;
                    if(locked == false) maxHamming = 0;

                    uint8_t hammingSync = hammingDistance((*demodFrame)[0],
                                                          STREAM_SYNC_WORD[0])
                                        + hammingDistance((*demodFrame)[1],
                                                          STREAM_SYNC_WORD[1]);

                    uint8_t hammingLsf = hammingDistance((*demodFrame)[0],
                                                         LSF_SYNC_WORD[0])
                                       + hammingDistance((*demodFrame)[1],
                                                         LSF_SYNC_WORD[1]);

                    if ((hammingSync > maxHamming) && (hammingLsf > maxHamming))
                    {
                        // Lock lost, reset demodulator alignment (phase) only
                        // if we were locked on a valid signal.
                        // This to avoid, in case of absence of carrier, to fall
                        // in a loop where the demodulator continues to search
                        // for the syncword in the same block of samples, causing
                        // the update function to take more than 20ms to complete.
                        if(locked) phase = 0;
                        syncDetected = false;
                        locked       = false;

                        #ifdef ENABLE_DEMOD_LOG
                        // Pre-arm the log trigger.
                        trigEnable = true;
                        #endif
                    }
                    else
                    {
                        // Correct syncword found
                        locked = true;

                        #ifdef ENABLE_DEMOD_LOG
                        // Trigger a data dump when lock is re-acquired.
                        if((dumpData == false) && (trigEnable == true))
                        {
                            trigEnable = false;
                            triggered  = true;
                        }
                        #endif
                    }
                }

                // Locate syncword to correct clock skew between Tx and Rx
                if (frame_index == M17_SYNCWORD_SYMBOLS + SYNC_SWEEP_OFFSET)
                {
                    // Find index (possibly negative) of the syncword
                    int32_t expected_sync =
                        phase +
                        M17_SAMPLES_PER_SYMBOL * decoded_syms -
                        M17_SYNCWORD_SAMPLES -
                        SYNC_SWEEP_OFFSET * M17_SAMPLES_PER_SYMBOL;
                    int32_t sync_skew = syncwordSweep(expected_sync);
                    phase += sync_skew;
                }

                // If the frame buffer is full switch demod and ready frame
                if (frame_index == M17_FRAME_SYMBOLS)
                {
                    demodFrame.swap(readyFrame);
                    frame_index = 0;
                    newFrame    = true;
                }
            }
        }

        // Copy last N samples to bridge buffer
        memcpy(basebandBridge,
               baseband.data + (baseband.len - M17_BRIDGE_SIZE),
               sizeof(int16_t) * M17_BRIDGE_SIZE);
    }

    #if defined(PLATFORM_LINUX) && defined(ENABLE_DEMOD_LOG)
    if (baseband.data == NULL)
        dumpData = true;
    #endif

    return newFrame;
}

void M17Demodulator::invertPhase(const bool status)
{
    invPhase = status;
}
