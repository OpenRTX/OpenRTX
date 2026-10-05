/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AUDIO_SDL_H
#define AUDIO_SDL_H

#include "interfaces/audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SDL2-backed audio playback and beep for the host (Linux) and WebAssembly
 * emulator targets. On native builds SDL routes to ALSA/PulseAudio/PipeWire;
 * under Emscripten it routes to the Web Audio API.
 *
 * Samples are 16 bit, signed, mono, at the stream sample rate. The playback
 * driver uses the SDL queue API (SDL_QueueAudio) so that it maps directly onto
 * the blocking data()/sync() driver contract without an audio callback thread.
 */

/**
 * Output driver: plays the audio stream on the default output device.
 */
extern const struct audioDriver sdl_play_audio_driver;

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_SDL_H */
