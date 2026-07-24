/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TONE_GENERATOR_SDL_H
#define TONE_GENERATOR_SDL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SDL2-backed beep tone generator for the Linux emulator and WebAssembly
 * targets. Drives a dedicated audio device on a background thread that
 * synthesizes a phase-continuous sine wave.
 */

/**
 * Start a continuous beep tone at the given frequency on a dedicated output
 * device. Calling it again while a beep is active retunes the tone.
 *
 * @param freq: beep frequency, in Hz.
 */
void toneGen_sdl_start(uint16_t freq);

/**
 * Stop an ongoing beep tone.
 */
void toneGen_sdl_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* TONE_GENERATOR_SDL_H */
