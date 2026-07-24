/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>
#include "SDL2/SDL.h"
#include "audio_sdl.h"

/*
 * The playback driver uses the SDL queue API rather than an audio callback: the
 * callback model would fire on an SDL-owned thread (and, under Emscripten's
 * PROXY_TO_PTHREAD, on the wrong side of the worker/main-thread split), whereas
 * SDL_QueueAudio maps cleanly onto the blocking data()/sync() contract expected
 * by the stream manager.
 */

// Number of extra queued chunks tolerated before a playback sync() blocks. Keeps
// output latency bounded to a few audio periods.
#define PLAY_QUEUE_SLACK 2

// Upper bound on the playback drain loops, in ms. Comfortably above any real
// drain time, but ensures a suspended device (e.g. an Emscripten audio context
// awaiting a user gesture) cannot block the producing thread forever.
#define PLAY_DRAIN_TIMEOUT_MS 2000

/*
 * SDL's audio subsystem is initialised exactly once for the whole process:
 * either the playback or the beep path can be the first to touch audio, and they
 * run on different threads, so the init must be race-free. pthread_once gives us
 * that; the subsystem is intentionally never de-initialised, matching the
 * process-lifetime nature of the audio devices.
 */
static pthread_once_t sdlAudioOnce = PTHREAD_ONCE_INIT;
static int sdlAudioReady = 0;

static void sdlAudioInit(void)
{
    sdlAudioReady = (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) ? 1 : 0;
}

struct sdlPlayState {
    SDL_AudioDeviceID dev;
    int idleHalf;
};

/**
 * \internal
 * Open the SDL playback device in the stream's format. allowed_changes is 0 so
 * SDL hands us exactly the requested format and performs any rate/format
 * conversion to the underlying hardware internally.
 */
static SDL_AudioDeviceID openDevice(const struct streamCtx *ctx)
{
    pthread_once(&sdlAudioOnce, sdlAudioInit);
    if (sdlAudioReady == 0)
        return 0;

    SDL_AudioSpec want;
    SDL_AudioSpec have;
    SDL_zero(want);
    want.freq = (int)ctx->sampleRate;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 256;

    return SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
}

static int sdlPlay_start(const uint8_t instance, const void *config,
                         struct streamCtx *ctx)
{
    (void)instance;
    (void)config;

    if (ctx == NULL)
        return -EINVAL;

    if (ctx->running != 0)
        return -EBUSY;

    struct sdlPlayState *state =
        static_cast<struct sdlPlayState *>(malloc(sizeof(struct sdlPlayState)));
    if (state == NULL)
        return -ENOMEM;

    state->dev = openDevice(ctx);
    if (state->dev == 0) {
        free(state);
        return -EIO;
    }

    state->idleHalf = 0;
    ctx->priv = state;
    ctx->running = 1;
    SDL_PauseAudioDevice(state->dev, 0);

    return 0;
}

static int sdlPlay_data(struct streamCtx *ctx, stream_sample_t **buf)
{
    if (ctx->running == 0)
        return -1;

    struct sdlPlayState *state = (struct sdlPlayState *)ctx->priv;
    size_t size = ctx->bufSize;
    if (ctx->bufMode == BUF_CIRC_DOUBLE)
        size /= 2;

    *buf = ctx->buffer + (state->idleHalf * size);

    return (int)size;
}

static int sdlPlay_sync(struct streamCtx *ctx, uint8_t dirty)
{
    if (ctx->running == 0)
        return -1;

    struct sdlPlayState *state = (struct sdlPlayState *)ctx->priv;
    size_t size = ctx->bufSize;
    if (ctx->bufMode == BUF_CIRC_DOUBLE)
        size /= 2;

    Uint32 bytes = (Uint32)(size * sizeof(stream_sample_t));

    if (dirty != 0) {
        stream_sample_t *half = ctx->buffer + (state->idleHalf * size);
        SDL_QueueAudio(state->dev, half, bytes);

        // Only double-buffered mode alternates halves; in linear mode the idle
        // half is always the (single) buffer, so leaving idleHalf at 0 keeps
        // data() from indexing one buffer past the end.
        if (ctx->bufMode == BUF_CIRC_DOUBLE)
            state->idleHalf ^= 1;
    }

    // Block until the queued audio drains back below the slack threshold. This
    // paces the producing thread to the audio device, mirroring how a hardware
    // driver blocks on its DMA syncpoint. Bounded so a stalled device cannot
    // wedge the producer indefinitely.
    int guard = 0;
    while ((SDL_GetQueuedAudioSize(state->dev) > (PLAY_QUEUE_SLACK * bytes))
           && (guard < PLAY_DRAIN_TIMEOUT_MS)) {
        SDL_Delay(1);
        guard += 1;
    }

    return 0;
}

static void sdlPlay_stop(struct streamCtx *ctx)
{
    if (ctx->running == 0)
        return;

    struct sdlPlayState *state = (struct sdlPlayState *)ctx->priv;

    // Graceful stop: let the already-queued tail play out before closing, but
    // bound the wait so a suspended device does not hang the caller. On timeout
    // we fall through and close anyway, dropping whatever remains queued.
    int guard = 0;
    while ((SDL_GetQueuedAudioSize(state->dev) > 0)
           && (guard < PLAY_DRAIN_TIMEOUT_MS)) {
        SDL_Delay(1);
        guard += 1;
    }

    SDL_CloseAudioDevice(state->dev);
    free(state);
    ctx->priv = NULL;
    ctx->running = 0;
}

static void sdlPlay_halt(struct streamCtx *ctx)
{
    if (ctx->running == 0)
        return;

    struct sdlPlayState *state = (struct sdlPlayState *)ctx->priv;

    SDL_ClearQueuedAudio(state->dev);
    SDL_CloseAudioDevice(state->dev);
    free(state);
    ctx->priv = NULL;
    ctx->running = 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wc++20-extensions"
const struct audioDriver sdl_play_audio_driver = {
    .start = sdlPlay_start,
    .data = sdlPlay_data,
    .sync = sdlPlay_sync,
    .stop = sdlPlay_stop,
    .terminate = sdlPlay_halt,
};
#pragma GCC diagnostic pop

/*
 * Beep tone generator. Beeps are a path separate from the SINK_SPK stream
 * (platform_beepStart/Stop), so they get their own persistent device driven by
 * a dedicated thread that synthesizes a phase-continuous sine wave.
 */

#define BEEP_RATE 48000
#define BEEP_CHUNK (BEEP_RATE / 50) // 20 ms of samples
#define BEEP_AMPLITUDE 8000

static pthread_t beepThread;
static pthread_once_t beepOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t beepMutex = PTHREAD_MUTEX_INITIALIZER;
static int beepActive = 0;
static uint16_t beepFreq = 0;

static void *beepThreadFunc(void *arg)
{
    (void)arg;

    pthread_once(&sdlAudioOnce, sdlAudioInit);
    if (sdlAudioReady == 0)
        return NULL;

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

void sdlBeep_start(uint16_t freq)
{
    pthread_once(&beepOnce, beepThreadStart);

    pthread_mutex_lock(&beepMutex);
    beepFreq = freq;
    beepActive = 1;
    pthread_mutex_unlock(&beepMutex);
}

void sdlBeep_stop(void)
{
    pthread_mutex_lock(&beepMutex);
    beepActive = 0;
    pthread_mutex_unlock(&beepMutex);
}
