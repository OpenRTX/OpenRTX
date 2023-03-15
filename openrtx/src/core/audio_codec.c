/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#include <audio_stream.h>
#include <audio_codec.h>
#include <pthread.h>
#include <codec2.h>
#include <stdlib.h>
#include <string.h>
#include <dsp.h>

#define BUF_SIZE 4

static struct CODEC2   *codec2;
static stream_sample_t *audioBuf;
static streamId         audioStream;

static uint8_t          initCnt = 0;
static bool             running;

static bool             stopThread;
static pthread_t        codecThread;
static pthread_mutex_t  mutex;
static pthread_cond_t   not_empty;
static pthread_cond_t   not_full;

static uint8_t          readPos;
static uint8_t          writePos;
static uint8_t          numElements;
static uint64_t         dataBuffer[BUF_SIZE];

#ifdef PLATFORM_MOD17
static const uint8_t micGainPre  = 4;
static const uint8_t micGainPost = 3;
#else
static const uint8_t micGainPre  = 8;
static const uint8_t micGainPost = 4;
#endif

static void *encodeFunc(void *arg);
static void *decodeFunc(void *arg);
static void startThread(void *(*func) (void *));


void codec_init()
{
    if(initCnt > 0)
    {
        pthread_mutex_lock(&mutex);
        initCnt += 1;
        pthread_mutex_unlock(&mutex);
        return;
    }
    else
    {
        initCnt = 1;
    }

    running     = false;
    readPos     = 0;
    writePos    = 0;
    numElements = 0;
    memset(dataBuffer, 0x00, BUF_SIZE * sizeof(uint64_t));

    audioBuf  = ((stream_sample_t *) malloc(320 * sizeof(stream_sample_t)));

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&not_empty, NULL);
    pthread_cond_init(&not_full, NULL);
}

void codec_terminate()
{
    pthread_mutex_lock(&mutex);
    initCnt -= 1;
    pthread_mutex_unlock(&mutex);

    if(initCnt > 0) return;
    if(running) codec_stop();

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);

    if(audioBuf != NULL)
    {
        free(audioBuf);
        audioBuf = NULL;
    }
}

bool codec_startEncode(const enum AudioSource source)
{
    if(running) return false;
    if(audioBuf == NULL) return false;

    running = true;

    audioStream = inputStream_start(source, PRIO_TX, audioBuf, 320,
                                    BUF_CIRC_DOUBLE, 8000);

    if(audioStream == -1)
    {
        running = false;
        return false;
    }

    readPos     = 0;
    writePos    = 0;
    numElements = 0;
    stopThread  = false;
    startThread(encodeFunc);

    return true;
}

bool codec_startDecode(const enum AudioSink destination)
{
    if(running) return false;
    if(audioBuf == NULL) return false;

    running = true;

    memset(audioBuf, 0x00, 320 * sizeof(stream_sample_t));
    audioStream = outputStream_start(destination, PRIO_RX, audioBuf, 320,
                                     BUF_CIRC_DOUBLE, 8000);

    if(audioStream == -1)
    {
        running = false;
        return false;
    }

    readPos     = 0;
    writePos    = 0;
    numElements = 0;
    stopThread  = false;
    startThread(decodeFunc);

    return true;
}

void codec_stop()
{
    if(running == false) return;

    stopThread = true;
    pthread_join(codecThread, NULL);

    running = false;
}

bool codec_popFrame(uint8_t *frame, const bool blocking)
{
    if(running == false) return false;

    uint64_t element;

    // No data available and non-blocking call: just return false.
    if((numElements == 0) && (blocking == false))
        return false;

    // Blocking call: wait until some data is pushed
    pthread_mutex_lock(&mutex);
    while(numElements == 0)
    {
        pthread_cond_wait(&not_empty, &mutex);
    }

    element      = dataBuffer[readPos];
    readPos      = (readPos + 1) % BUF_SIZE;
    numElements -= 1;
    pthread_mutex_unlock(&mutex);

    // Do memcpy after mutex unlock to reduce execution time spent inside the
    // critical section
    memcpy(frame, &element, 8);

    return true;
}

bool codec_pushFrame(const uint8_t *frame, const bool blocking)
{
    if(running == false) return false;

    // Copy data to a temporary variable before mutex lock to reduce execution
    // time spent inside the critical section
    uint64_t element;
    memcpy(&element, frame, 8);


    // No space available and non-blocking call: return
    if((numElements >= BUF_SIZE) && (blocking == false))
        return false;

    // Blocking call: wait until there is some free space
    pthread_mutex_lock(&mutex);
    while(numElements >= BUF_SIZE)
    {
        pthread_cond_wait(&not_full, &mutex);
    }

    // There is free space, push data into the queue
    dataBuffer[writePos] = element;
    writePos = (writePos + 1) % BUF_SIZE;

    // Signal that the queue is not empty
    if(numElements == 0) pthread_cond_signal(&not_empty);
    numElements += 1;

    pthread_mutex_unlock(&mutex);
    return true;
}




static void *encodeFunc(void *arg)
{
    (void) arg;

    filter_state_t dcrState;
    dsp_resetFilterState(&dcrState);

    codec2 = codec2_create(CODEC2_MODE_3200);

    while(stopThread == false)
    {
        dataBlock_t audio = inputStream_getData(audioStream);

        if(audio.data != NULL)
        {
            #ifndef PLATFORM_LINUX
            // Pre-amplification stage
            for(size_t i = 0; i < audio.len; i++) audio.data[i] *= micGainPre;

            // DC removal
            dsp_dcRemoval(&dcrState, audio.data, audio.len);

            // Post-amplification stage
            for(size_t i = 0; i < audio.len; i++) audio.data[i] *= micGainPost;
            #endif

            // CODEC2 encodes 160ms of speech into 8 bytes: here we write the
            // new encoded data into a buffer of 16 bytes writing the first
            // half and then the second one, sequentially.
            // Data ready flag is rised once all the 16 bytes contain new data.
            uint64_t frame = 0;
            codec2_encode(codec2, ((uint8_t*) &frame), audio.data);

            pthread_mutex_lock(&mutex);

            // If buffer is full erase the oldest frame
            if(numElements >= BUF_SIZE)
            {
                readPos = (readPos + 1) % BUF_SIZE;
            }

            dataBuffer[writePos] = frame;
            writePos = (writePos + 1) % BUF_SIZE;

            if(numElements == 0) pthread_cond_signal(&not_empty);
            if(numElements < BUF_SIZE) numElements += 1;

            pthread_mutex_unlock(&mutex);
        }
    }

    inputStream_stop(audioStream);
    codec2_destroy(codec2);

    return NULL;
}

static void *decodeFunc(void *arg)
{
    (void) arg;

    codec2 = codec2_create(CODEC2_MODE_3200);

    // Ensure that thread start is correctly synchronized with the output
    // stream to avoid having the decode function writing in a memory area
    // being read at the same time by the output stream system causing cracking
    // noises at speaker output. Behaviour observed on both Module17 and MD-UV380
    outputStream_sync(audioStream, false);

    while(stopThread == false)
    {
        // Try popping data from the queue
        uint64_t frame   = 0;
        bool     newData = false;

        pthread_mutex_lock(&mutex);

        if(numElements != 0)
        {
            frame        = dataBuffer[readPos];
            readPos      = (readPos + 1) % BUF_SIZE;
            if(numElements >= BUF_SIZE) pthread_cond_signal(&not_full);
            numElements -= 1;
            newData      = true;
        }

        pthread_mutex_unlock(&mutex);

        stream_sample_t *audioBuf = outputStream_getIdleBuffer(audioStream);

        if(newData)
        {
            codec2_decode(codec2, audioBuf, ((uint8_t *) &frame));

            #ifdef PLATFORM_MD3x0
            // Bump up volume a little bit, as on MD3x0 is quite low
            for(size_t i = 0; i < 160; i++) audioBuf[i] *= 2;
            #endif

        }
        else
        {
            memset(audioBuf, 0x00, 160 * sizeof(stream_sample_t));
        }

        outputStream_sync(audioStream, true);
    }

    // Stop stream and wait until its effective termination
    outputStream_stop(audioStream);
    outputStream_sync(audioStream, false);
    codec2_destroy(codec2);

    return NULL;
}

static void startThread(void *(*func) (void *))
{
    #ifdef _MIOSIX
    // Set stack size of CODEC2 thread to 16kB.
    pthread_attr_t codecAttr;
    pthread_attr_init(&codecAttr);
    pthread_attr_setstacksize(&codecAttr, 16384);

    // Set priority of CODEC2 thread to the maximum one, the same of RTX thread.
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(0);
    pthread_attr_setschedparam(&codecAttr, &param);

    // Start thread
    pthread_create(&codecThread, &codecAttr, func, NULL);
    #else
    pthread_create(&codecThread, NULL, func, NULL);
    #endif

}
