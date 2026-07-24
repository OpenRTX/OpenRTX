/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <math.h>
#include <pthread.h>
#include "SDL2/SDL.h"
#include "toneGenerator_sdl.h"
#include "interfaces/audio.h"

#define BEEP_RATE 48000
#define BEEP_CHUNK (BEEP_RATE / 50) // 20 ms of samples
#define BEEP_AMPLITUDE 8000

// Number of extra queued chunks tolerated before a playback sync() blocks.
#define PLAY_QUEUE_SLACK 2

static pthread_t beepThread;
static pthread_once_t beepOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t beepMutex = PTHREAD_MUTEX_INITIALIZER;
static int beepActive = 0;
static uint16_t beepFreq = 0;

static void *beepThreadFunc(void *arg)
{
    (void)arg;

    SDL_AudioSpec want;
    SDL_AudioSpec have;
    SDL_zero(want);
    want.freq = BEEP_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0)
        return NULL;

    stream_sample_t chunk[BEEP_CHUNK];
    double phase = 0.0;
    int playing = 0;

    for (;;) {
        pthread_mutex_lock(&beepMutex);
        int active = beepActive;
        uint16_t freq = beepFreq;
        pthread_mutex_unlock(&beepMutex);

        if (active != 0) {
            if (playing == 0) {
                SDL_ClearQueuedAudio(dev);
                SDL_PauseAudioDevice(dev, 0);
                playing = 1;
            }

            double step = (2.0 * M_PI * (double)freq) / (double)BEEP_RATE;
            for (size_t i = 0; i < BEEP_CHUNK; i++) {
                chunk[i] = (stream_sample_t)(BEEP_AMPLITUDE * sin(phase));
                phase += step;
                if (phase >= (2.0 * M_PI))
                    phase -= (2.0 * M_PI);
            }

            Uint32 chunkBytes = (Uint32)sizeof(chunk);
            while (SDL_GetQueuedAudioSize(dev)
                   > (PLAY_QUEUE_SLACK * chunkBytes))
                SDL_Delay(1);

            SDL_QueueAudio(dev, chunk, chunkBytes);
        } else {
            if (playing != 0) {
                SDL_ClearQueuedAudio(dev);
                SDL_PauseAudioDevice(dev, 1);
                playing = 0;
                phase = 0.0;
            }

            SDL_Delay(5);
        }
    }

    return NULL;
}

static void beepThreadStart(void)
{
    pthread_create(&beepThread, NULL, beepThreadFunc, NULL);
}

void toneGen_sdl_start(uint16_t freq)
{
    pthread_once(&beepOnce, beepThreadStart);

    pthread_mutex_lock(&beepMutex);
    beepFreq = freq;
    beepActive = 1;
    pthread_mutex_unlock(&beepMutex);
}

void toneGen_sdl_stop(void)
{
    pthread_mutex_lock(&beepMutex);
    beepActive = 0;
    pthread_mutex_unlock(&beepMutex);
}
