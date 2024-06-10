/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Federico Amedeo Izzo IU2NUO,             *
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

    baseband_buffer = std::make_unique< int16_t[] >(2 * SAMPLE_BUF_SIZE);
    demodFrame      = std::make_unique< frame_t >();
    readyFrame      = std::make_unique< frame_t >();

    reset();

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
                                   2 * SAMPLE_BUF_SIZE, RX_SAMPLE_RATE,
                                   STREAM_INPUT | BUF_CIRC_DOUBLE);

    reset();
}

void M17Demodulator::stopBasebandSampling()
{
    audioStream_terminate(basebandId);
    audioPath_release(basebandPath);
    locked = false;
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

bool M17Demodulator::update(const bool invertPhase)
{
    // Audio path closed, nothing to do
    if(audioPath_getStatus(basebandPath) != PATH_OPEN)
        return false;

    // Read samples from the ADC
    dataBlock_t baseband = inputStream_getData(basebandId);
    if(baseband.data != NULL)
    {
        // Apply DC removal filter
        dsp_dcRemoval(&dcrState, baseband.data, baseband.len);

        // Process samples
        for(size_t i = 0; i < baseband.len; i++)
        {
            // Apply RRC on the baseband sample
            float           elem   = static_cast< float >(baseband.data[i]);
            if(invertPhase) elem   = 0.0f - elem;
            int16_t         sample = static_cast< int16_t >(M17::rrc_24k(elem));

            // Update correlator and sample filter for correlation thresholds
            correlator.sample(sample);
            corrThreshold = sampleFilter(std::abs(sample));

            switch(demodState)
            {
                case DemodState::INIT:
                {
                    initCount -= 1;
                    if(initCount == 0)
                        demodState = DemodState::UNLOCKED;
                }
                    break;

                case DemodState::UNLOCKED:
                {
                    int32_t syncThresh = static_cast< int32_t >(corrThreshold * 33.0f);
                    int8_t  syncStatus = streamSync.update(correlator, syncThresh, -syncThresh);

                    if(syncStatus != 0)
                        demodState = DemodState::SYNCED;
                }
                    break;

                case DemodState::SYNCED:
                {
                    // Set sampling point and deviation, zero frame symbol count
                    samplingPoint  = streamSync.samplingIndex();
                    outerDeviation = correlator.maxDeviation(samplingPoint);
                    frameIndex     = 0;

                    // Quantize the syncword taking data from the correlator
                    // memory.
                    for(size_t i = 0; i < SYNCWORD_SAMPLES; i++)
                    {
                        size_t  pos = (correlator.index() + i) % SYNCWORD_SAMPLES;
                        int16_t val = correlator.data()[pos];

                        if((pos % SAMPLES_PER_SYMBOL) == samplingPoint)
                            updateFrame(val);
                    }

                    uint8_t hd  = hammingDistance((*demodFrame)[0], STREAM_SYNC_WORD[0]);
                            hd += hammingDistance((*demodFrame)[1], STREAM_SYNC_WORD[1]);

                    if(hd == 0)
                    {
                        locked     = true;
                        demodState = DemodState::LOCKED;
                    }
                    else
                    {
                        demodState = DemodState::UNLOCKED;
                    }
                }
                    break;

                case DemodState::LOCKED:
                {
                    // Quantize and update frame at each sampling point
                    if(sampleIndex == samplingPoint)
                    {
                        updateFrame(sample);

                        // When we have reached almost the end of a frame, switch
                        // to syncpoint update.
                        if(frameIndex == (M17_FRAME_SYMBOLS - M17_SYNCWORD_SYMBOLS/2))
                        {
                            demodState = DemodState::SYNC_UPDATE;
                            syncCount  = SYNCWORD_SAMPLES * 2;
                        }
                    }
                }
                    break;

                case DemodState::SYNC_UPDATE:
                {
                    // Keep filling the ongoing frame!
                    if(sampleIndex == samplingPoint)
                        updateFrame(sample);

                    // Find the new correlation peak
                    int32_t syncThresh = static_cast< int32_t >(corrThreshold * 33.0f);
                    int8_t  syncStatus = streamSync.update(correlator, syncThresh, -syncThresh);

                    if(syncStatus != 0)
                    {
                        // Correlation has to coincide with a syncword!
                        if(frameIndex == M17_SYNCWORD_SYMBOLS)
                        {
                            uint8_t hd  = hammingDistance((*demodFrame)[0], STREAM_SYNC_WORD[0]);
                                    hd += hammingDistance((*demodFrame)[1], STREAM_SYNC_WORD[1]);

                            // Valid sync found: update deviation and sample
                            // point, then go back to locked state
                            if(hd <= 1)
                            {
                                outerDeviation = correlator.maxDeviation(samplingPoint);
                                samplingPoint  = streamSync.samplingIndex();
                                missedSyncs    = 0;
                                demodState     = DemodState::LOCKED;
                                break;
                            }
                        }
                    }

                    // No syncword found within the window, increase the count
                    // of missed syncs and choose where to go. The lock is lost
                    // after four consecutive sync misses.
                    if(syncCount == 0)
                    {
                        if(missedSyncs >= 4)
                        {
                            demodState = DemodState::UNLOCKED;
                            locked     = false;
                        }
                        else
                        {
                            demodState = DemodState::LOCKED;
                        }

                        missedSyncs += 1;
                    }

                    syncCount -= 1;
                }
                    break;
            }

            sampleCount += 1;
            sampleIndex  = (sampleIndex + 1) % SAMPLES_PER_SYMBOL;
        }
    }

    return newFrame;
}

int8_t M17Demodulator::updateFrame(stream_sample_t sample)
{
    int8_t symbol;

    if(sample > (2 * outerDeviation.first)/3)
    {
        symbol = +3;
    }
    else if(sample < (2 * outerDeviation.second)/3)
    {
        symbol = -3;
    }
    else if(sample > 0)
    {
        symbol = +1;
    }
    else
    {
        symbol = -1;
    }

    setSymbol(*demodFrame, frameIndex, symbol);
    frameIndex += 1;

    if(frameIndex >= M17_FRAME_SYMBOLS)
    {
        std::swap(readyFrame, demodFrame);
        frameIndex = 0;
        newFrame   = true;
    }

    return symbol;
}

void M17Demodulator::reset()
{
    sampleIndex = 0;
    frameIndex  = 0;
    sampleCount = 0;
    newFrame    = false;
    locked      = false;
    demodState  = DemodState::INIT;
    initCount   = RX_SAMPLE_RATE / 50;  // 50ms of init time

    dsp_resetFilterState(&dcrState);
}


constexpr std::array < float, 3 > M17Demodulator::sfNum;
constexpr std::array < float, 3 > M17Demodulator::sfDen;
