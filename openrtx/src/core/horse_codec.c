/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Horse voice codec: MELPe 2400, 12 bytes per 40ms. Uses melpe_horse encode/decode.
 */

#include "core/horse_codec.h"
#include "core/audio_stream.h"
#include "protocols/horse/melpe_horse.h"
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define HORSE_BUF_SIZE 4

static pathId horse_audioPath;
static uint8_t horse_initCnt = 0;
static bool horse_running;
static bool horse_reqStop;
static pthread_t horse_codecThread;
static pthread_attr_t horse_codecAttr;
static pthread_mutex_t horse_data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t horse_init_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t horse_wakeup_cond = PTHREAD_COND_INITIALIZER;

static uint8_t horse_readPos;
static uint8_t horse_writePos;
static uint8_t horse_numElements;
static uint8_t horse_dataBuffer[HORSE_BUF_SIZE][HORSE_CODEC_FRAME_BYTES];

static void *horse_encodeFunc(void *arg);
static void *horse_decodeFunc(void *arg);
static bool horse_startThread(const pathId path, void *(*func)(void *));
static void horse_stopThread(void);

void horse_codec_init(void)
{
    pthread_mutex_lock(&horse_init_mutex);
    horse_initCnt += 1;
    pthread_mutex_unlock(&horse_init_mutex);

    if (horse_initCnt > 1)
        return;

    horse_running = false;
    horse_readPos = 0;
    horse_writePos = 0;
    horse_numElements = 0;
    melpe_horse_encoder_init();
    melpe_horse_decoder_init();
}

void horse_codec_terminate(void)
{
    pthread_mutex_lock(&horse_init_mutex);
    horse_initCnt -= 1;
    pthread_mutex_unlock(&horse_init_mutex);

    if (horse_initCnt > 0)
        return;

    if (horse_running)
        horse_stopThread();
    melpe_horse_encoder_terminate();
    melpe_horse_decoder_terminate();
}

bool horse_codec_startEncode(const pathId path)
{
    return horse_startThread(path, horse_encodeFunc);
}

bool horse_codec_startDecode(const pathId path)
{
    return horse_startThread(path, horse_decodeFunc);
}

void horse_codec_stop(const pathId path)
{
    if (!horse_running || horse_audioPath != path)
        return;
    horse_stopThread();
}

bool horse_codec_running(void)
{
    return horse_running;
}

int horse_codec_popFrame(uint8_t *frame, const bool blocking)
{
    if (!horse_running)
        return -EPERM;

    pthread_mutex_lock(&horse_data_mutex);
    if (horse_numElements == 0 && !blocking)
    {
        pthread_mutex_unlock(&horse_data_mutex);
        return -EAGAIN;
    }

    while (horse_numElements == 0 && horse_running)
        pthread_cond_wait(&horse_wakeup_cond, &horse_data_mutex);

    if (!horse_running || horse_numElements == 0)
    {
        pthread_mutex_unlock(&horse_data_mutex);
        return -EPERM;
    }

    memcpy(frame, horse_dataBuffer[horse_readPos], HORSE_CODEC_FRAME_BYTES);
    horse_readPos = (horse_readPos + 1) % HORSE_BUF_SIZE;
    horse_numElements -= 1;
    pthread_mutex_unlock(&horse_data_mutex);
    return 0;
}

int horse_codec_pushFrame(const uint8_t *frame, const bool blocking)
{
    if (!horse_running)
        return -EPERM;

    pthread_mutex_lock(&horse_data_mutex);
    while (horse_numElements >= HORSE_BUF_SIZE && horse_running && blocking)
        pthread_cond_wait(&horse_wakeup_cond, &horse_data_mutex);

    if (horse_numElements >= HORSE_BUF_SIZE)
    {
        if (!blocking)
        {
            pthread_mutex_unlock(&horse_data_mutex);
            return -EAGAIN;
        }
        while (horse_numElements >= HORSE_BUF_SIZE && horse_running)
            pthread_cond_wait(&horse_wakeup_cond, &horse_data_mutex);
    }

    if (!horse_running)
    {
        pthread_mutex_unlock(&horse_data_mutex);
        return -EPERM;
    }

    memcpy(horse_dataBuffer[horse_writePos], frame, HORSE_CODEC_FRAME_BYTES);
    horse_writePos = (horse_writePos + 1) % HORSE_BUF_SIZE;
    horse_numElements += 1;
    pthread_cond_signal(&horse_wakeup_cond);
    pthread_mutex_unlock(&horse_data_mutex);
    return 0;
}

static void *horse_encodeFunc(void *arg)
{
    pathId iPath = *((pathId *)arg);
    stream_sample_t audioBuf[MELPE_HORSE_SAMPLES_20MS];
    streamId iStream = audioStream_start(iPath, audioBuf, MELPE_HORSE_SAMPLES_20MS,
                                         8000, STREAM_INPUT | BUF_CIRC_DOUBLE);
    if (iStream < 0)
    {
        horse_running = false;
        return NULL;
    }

    while (!horse_reqStop)
    {
        if (audioPath_getStatus(iPath) != PATH_OPEN)
            break;

        dataBlock_t audio = inputStream_getData(iStream);
        if (audio.data == NULL)
            break;

        uint8_t frame[HORSE_CODEC_FRAME_BYTES];
        int got;

        if (audio.len < MELPE_HORSE_SAMPLES_20MS)
            break;

        got = melpe_horse_encode_20ms(audio.data, frame);
        if (got != 1)
            continue;

        pthread_mutex_lock(&horse_data_mutex);
        if (horse_numElements >= HORSE_BUF_SIZE)
        {
            horse_readPos = (horse_readPos + 1) % HORSE_BUF_SIZE;
            horse_numElements -= 1;
        }
        memcpy(horse_dataBuffer[horse_writePos], frame, HORSE_CODEC_FRAME_BYTES);
        horse_writePos = (horse_writePos + 1) % HORSE_BUF_SIZE;
        if (horse_numElements < HORSE_BUF_SIZE)
            horse_numElements += 1;
        pthread_cond_signal(&horse_wakeup_cond);
        pthread_mutex_unlock(&horse_data_mutex);
    }

    audioStream_terminate(iStream);
    horse_running = false;
    return NULL;
}

static void *horse_decodeFunc(void *arg)
{
    pathId oPath = *((pathId *)arg);
    stream_sample_t audioBuf[MELPE_HORSE_SAMPLES_20MS];
    memset(audioBuf, 0, sizeof(audioBuf));
    streamId oStream = audioStream_start(oPath, audioBuf, MELPE_HORSE_SAMPLES_20MS,
                                         8000, STREAM_OUTPUT | BUF_CIRC_DOUBLE);
    if (oStream < 0)
    {
        horse_running = false;
        return NULL;
    }

    outputStream_sync(oStream, false);

    bool have_second = false;

    while (!horse_reqStop)
    {
        if (audioPath_getStatus(oPath) != PATH_OPEN)
            break;

        stream_sample_t *outBuf = outputStream_getIdleBuffer(oStream);
        if (outBuf == NULL)
            break;

        uint8_t frame[HORSE_CODEC_FRAME_BYTES];
        bool newData = false;

        if (have_second)
        {
            if (melpe_horse_decode_20ms_next(outBuf, MELPE_HORSE_SAMPLES_20MS) < 0)
                memset(outBuf, 0, MELPE_HORSE_SAMPLES_20MS * sizeof(stream_sample_t));
            have_second = false;
        }
        else
        {
            pthread_mutex_lock(&horse_data_mutex);
            if (horse_numElements != 0)
            {
                memcpy(frame, horse_dataBuffer[horse_readPos], HORSE_CODEC_FRAME_BYTES);
                horse_readPos = (horse_readPos + 1) % HORSE_BUF_SIZE;
                horse_numElements -= 1;
                pthread_cond_signal(&horse_wakeup_cond);
                newData = true;
            }
            pthread_mutex_unlock(&horse_data_mutex);

            if (newData)
            {
                if (melpe_horse_decode_20ms_start(frame, outBuf,
                                                  MELPE_HORSE_SAMPLES_20MS) < 0)
                    memset(outBuf, 0,
                           MELPE_HORSE_SAMPLES_20MS * sizeof(stream_sample_t));
                else
                    have_second = true;
            }
            else
            {
                memset(outBuf, 0, MELPE_HORSE_SAMPLES_20MS * sizeof(stream_sample_t));
            }
        }

        outputStream_sync(oStream, true);
    }

    audioStream_stop(oStream);
    horse_running = false;
    return NULL;
}

static bool horse_startThread(const pathId path, void *(*func)(void *))
{
    if (audioPath_getStatus(path) != PATH_OPEN)
        return false;

    pthread_mutex_lock(&horse_init_mutex);
    if (horse_running)
    {
        if (path == horse_audioPath)
        {
            pthread_mutex_unlock(&horse_init_mutex);
            return true;
        }
        horse_stopThread();
    }

    horse_running = true;
    horse_audioPath = path;
    horse_readPos = 0;
    horse_writePos = 0;
    horse_numElements = 0;
    horse_reqStop = false;
    pthread_mutex_unlock(&horse_init_mutex);

    pthread_attr_init(&horse_codecAttr);
    int ret = pthread_create(&horse_codecThread, &horse_codecAttr, func, &horse_audioPath);
    if (ret != 0)
        horse_running = false;

    return horse_running;
}

static void horse_stopThread(void)
{
    horse_reqStop = true;
    pthread_join(horse_codecThread, NULL);
    horse_running = false;
}
