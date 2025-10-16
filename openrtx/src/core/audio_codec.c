/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/audio_stream.h"
#include "core/audio_codec.h"
#include <pthread.h>
#include "core/threads.h"
// codec2 system library has a weird include prefix
#if defined(PLATFORM_LINUX)
#include <codec2/codec2.h>
#else
#include "codec2.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "core/dsp.h"

#define BUF_SIZE 4

static pathId           audioPath;

static uint8_t          initCnt = 0;
static bool             running;

static bool             reqStop;
static pthread_t        codecThread;
static pthread_attr_t   codecAttr;
static pthread_mutex_t  data_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  init_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   wakeup_cond = PTHREAD_COND_INITIALIZER;

static uint8_t          readPos;
static uint8_t          writePos;
static uint8_t          numElements;
static uint64_t         dataBuffer[BUF_SIZE];

#ifdef PLATFORM_MOD17
static const uint8_t micGain = 12;
#else
#ifndef PLATFORM_LINUX
static const uint8_t micGain = 32;
#endif
#endif

static void *encodeFunc(void *arg);
static void *decodeFunc(void *arg);
static bool startThread(const pathId path, void *(*func) (void *));
static void stopThread();


void codec_init()
{
    pthread_mutex_lock(&init_mutex);
    initCnt += 1;
    pthread_mutex_unlock(&init_mutex);

    if(initCnt > 0)
        return;

    running     = false;
    readPos     = 0;
    writePos    = 0;
    numElements = 0;
}

void codec_terminate()
{
    pthread_mutex_lock(&init_mutex);
    initCnt -= 1;
    pthread_mutex_unlock(&init_mutex);

    if(initCnt > 0)
        return;

    if(running)
        stopThread();
}

bool codec_startEncode(const pathId path)
{
    return startThread(path, encodeFunc);
}

bool codec_startDecode(const pathId path)
{
    return startThread(path, decodeFunc);
}

void codec_stop(const pathId path)
{
    if(running == false)
        return;

    if(audioPath != path)
        return;

    stopThread();
}

bool codec_running()
{
    return running;
}

int codec_popFrame(uint8_t *frame, const bool blocking)
{
    if(running == false)
        return -EPERM;

    uint64_t element;

    // No data available and non-blocking call: just return false.
    if((numElements == 0) && (blocking == false))
        return -EAGAIN;

    // Blocking call: wait until some data is pushed
    pthread_mutex_lock(&data_mutex);
    while(numElements == 0)
    {
        pthread_cond_wait(&wakeup_cond, &data_mutex);
    }

    element      = dataBuffer[readPos];
    readPos      = (readPos + 1) % BUF_SIZE;
    numElements -= 1;
    pthread_mutex_unlock(&data_mutex);

    // Do memcpy after mutex unlock to reduce execution time spent inside the
    // critical section
    memcpy(frame, &element, 8);

    return 0;
}

int codec_pushFrame(const uint8_t *frame, const bool blocking)
{
    if(running == false)
        return -EPERM;

    // Copy data to a temporary variable before mutex lock to reduce execution
    // time spent inside the critical section
    uint64_t element;
    memcpy(&element, frame, 8);


    // No space available and non-blocking call: return
    if((numElements >= BUF_SIZE) && (blocking == false))
        return -EAGAIN;

    // Blocking call: wait until there is some free space
    pthread_mutex_lock(&data_mutex);
    while(numElements >= BUF_SIZE)
    {
        pthread_cond_wait(&wakeup_cond, &data_mutex);
    }

    // There is free space, push data into the queue
    dataBuffer[writePos] = element;
    writePos = (writePos + 1) % BUF_SIZE;
    numElements += 1;

    pthread_mutex_unlock(&data_mutex);
    return 0;
}




static void *encodeFunc(void *arg)
{

    streamId        iStream;
    pathId          iPath = *((pathId*) arg);
    stream_sample_t audioBuf[320];
    struct CODEC2   *codec2;
    struct dcBlock  dcBlock;

    iStream = audioStream_start(iPath, audioBuf, 320, 8000,
                                STREAM_INPUT | BUF_CIRC_DOUBLE);
    if(iStream < 0)
    {
        pthread_detach(pthread_self());
        running = false;
        return NULL;
    }

    dsp_resetState(dcBlock);
    codec2 = codec2_create(CODEC2_MODE_3200);

    while(reqStop == false)
    {
        // Invalid path, quit
        if(audioPath_getStatus(iPath) != PATH_OPEN)
            break;

        dataBlock_t audio = inputStream_getData(iStream);
        if(audio.data == NULL)
            break;

        #ifndef PLATFORM_LINUX
        for(size_t i = 0; i < audio.len; i++) {
            int16_t sample;

            sample = dsp_dcBlockFilter(&dcBlock, audio.data[i]);
            audio.data[i] = sample * micGain;
        }
        #endif

        // CODEC2 encodes 160ms of speech into 8 bytes: here we write the
        // new encoded data into a buffer of 16 bytes writing the first
        // half and then the second one, sequentially.
        // Data ready flag is rised once all the 16 bytes contain new data.
        uint64_t frame = 0;
        codec2_encode(codec2, ((uint8_t*) &frame), audio.data);

        pthread_mutex_lock(&data_mutex);

        // If buffer is full erase the oldest frame
        if(numElements >= BUF_SIZE)
        {
            readPos = (readPos + 1) % BUF_SIZE;
        }

        dataBuffer[writePos] = frame;
        writePos = (writePos + 1) % BUF_SIZE;

        if(numElements == 0)
            pthread_cond_signal(&wakeup_cond);

        if(numElements < BUF_SIZE)
            numElements += 1;

        pthread_mutex_unlock(&data_mutex);
    }

    audioStream_terminate(iStream);
    codec2_destroy(codec2);

    // In case thread terminates due to invalid path or stream error, detach it
    // to ensure that its memory gets freed by the OS.
    if(reqStop == false)
        pthread_detach(pthread_self());

    running = false;
    return NULL;
}

static void *decodeFunc(void *arg)
{
    streamId        oStream;
    pathId          oPath = *((pathId*) arg);
    stream_sample_t audioBuf[320];
    struct CODEC2   *codec2;

    // Open output stream
    memset(audioBuf, 0x00, 320 * sizeof(stream_sample_t));
    oStream = audioStream_start(oPath, audioBuf, 320, 8000,
                                STREAM_OUTPUT | BUF_CIRC_DOUBLE);
    if(oStream < 0)
    {
        pthread_detach(pthread_self());
        running = false;
        return NULL;
    }

    codec2 = codec2_create(CODEC2_MODE_3200);

    // Ensure that thread start is correctly synchronized with the output
    // stream to avoid having the decode function writing in a memory area
    // being read at the same time by the output stream system causing cracking
    // noises at speaker output. Behaviour observed on both Module17 and MD-UV380
    outputStream_sync(oStream, false);

    while(reqStop == false)
    {
        // Invalid path, quit
        if(audioPath_getStatus(oPath) != PATH_OPEN)
            break;

        // Try popping data from the queue
        uint64_t frame   = 0;
        bool     newData = false;

        pthread_mutex_lock(&data_mutex);

        if(numElements != 0)
        {
            frame        = dataBuffer[readPos];
            readPos      = (readPos + 1) % BUF_SIZE;
            if(numElements >= BUF_SIZE)
                pthread_cond_signal(&wakeup_cond);

            numElements -= 1;
            newData      = true;
        }

        pthread_mutex_unlock(&data_mutex);

        stream_sample_t *audioBuf = outputStream_getIdleBuffer(oStream);
        if(audioBuf == NULL)
            break;

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

        outputStream_sync(oStream, true);
    }

    // Stop stream and wait until its effective termination
    audioStream_stop(oStream);
    codec2_destroy(codec2);

    // In case thread terminates due to invalid path or stream error, detach it
    // to ensure that its memory gets freed by the OS.
    if(reqStop == false)
        pthread_detach(pthread_self());

    running = false;
    return NULL;
}

static bool startThread(const pathId path, void *(*func) (void *))
{
    // Bad incoming path
    if(audioPath_getStatus(path) != PATH_OPEN)
        return false;

    // Handle access contention when starting the codec thread to ensure that
    // only one call at a time can effectively start the thread.
    pthread_mutex_lock(&init_mutex);
    if(running)
    {
        // Same path as before, path open, codec already running: all good.
        if(path == audioPath)
        {
            pthread_mutex_unlock(&init_mutex);
            return true;
        }

        // New path takes over the current one only if it has an higher priority
        // or the current one is closed/suspended.
        pathInfo_t newPath = audioPath_getInfo(path);
        pathInfo_t curPath = audioPath_getInfo(audioPath);
        if((curPath.status == PATH_OPEN) && (curPath.prio >= newPath.prio))
        {
            pthread_mutex_unlock(&init_mutex);
            return false;
        }
        else
        {
            stopThread();
        }
    }

    running   = true;
    audioPath = path;
    pthread_mutex_unlock(&init_mutex);

    readPos     = 0;
    writePos    = 0;
    numElements = 0;
    reqStop     = false;

    pthread_attr_init(&codecAttr);

    #if defined(_MIOSIX)
    // Set stack size of CODEC2 thread to 16kB.
    pthread_attr_setstacksize(&codecAttr, CODEC2_THREAD_STKSIZE);

    // Set priority of CODEC2 thread to the maximum one, the same of RTX thread.
    struct sched_param param;
    param.sched_priority = THREAD_PRIO_HIGH;
    pthread_attr_setschedparam(&codecAttr, &param);
    #elif defined(__ZEPHYR__)
    // Allocate and set the stack for CODEC2 thread
    void *codec_thread_stack = malloc(CODEC2_THREAD_STKSIZE * sizeof(uint8_t));
    pthread_attr_setstack(&codecAttr, codec_thread_stack, CODEC2_THREAD_STKSIZE);
    #endif

    // Start thread
    int ret = pthread_create(&codecThread, &codecAttr, func, &audioPath);
    if(ret < 0)
        running = false;

    return running;
}

static void stopThread()
{
    reqStop = true;
    pthread_join(codecThread, NULL);
    running = false;

    #ifdef __ZEPHYR__
    void  *addr;
    size_t size;

    pthread_attr_getstack(&codecAttr, &addr, &size);
    free(addr);
    #endif
}
