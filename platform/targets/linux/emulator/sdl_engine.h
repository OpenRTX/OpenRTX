/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SDL_ENGINE_H
#define SDL_ENGINE_H

#include "interfaces/keyboard.h"
#include "SDL2/SDL.h"
#include <stdbool.h>
#include <stdint.h>
#include "core/chan.h"

/*
 * Screen dimensions, adjust basing on the size of the screen you need to
 * emulate
 */
#ifndef CONFIG_SCREEN_WIDTH
#define CONFIG_SCREEN_WIDTH 160
#endif

#ifndef CONFIG_SCREEN_HEIGHT
#define CONFIG_SCREEN_HEIGHT 128
#endif

#ifdef CONFIG_PIX_FMT_RGB565
#define PIXEL_FORMAT SDL_PIXELFORMAT_RGB565
#define PIXEL_SIZE uint16_t
#else
#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888
#define PIXEL_SIZE uint32_t
#endif

/**
 * Initialize the SDL engine. Must be called in the Main Thread.
 */
void sdlEngine_init();

/**
 * SDL main loop. Must be called in the Main Thread.
 */
void sdlEngine_run();

/**
 * Block until a previously-queued synchronous screenshot event has been
 * fully processed by the SDL thread, or the timeout elapses.
 *
 * @param timeout_ms maximum time to wait, in milliseconds.
 * @return true if the screenshot completed, false on timeout.
 */
bool sdlEngine_waitScreenshot(uint32_t timeout_ms);

/**
 * Compute an FNV-1a hash of the most recently rendered framebuffer.
 * The SDL thread caches each frame into an internal buffer protected
 * by a mutex; this call simply hashes that buffer, so it is safe to
 * call from any thread and never blocks the SDL event loop.
 *
 * @param out_hash receives the computed hash on success.
 * @param timeout_ms ignored; retained for API stability.
 * @return true on success, false if the engine is not initialized.
 */
bool sdlEngine_getFrameHash(uint64_t *out_hash, uint32_t timeout_ms);

/**
 * Thread-safe check to verify if the application entered the SDL main loop.
 *
 * @return true after sdl_task() started.
 */
bool sdlEngine_ready();

/**
 * Thread-safe function returning the keys currently being pressed.
 *
 * @return a keyboard_t bit field with the keys currently being pressed.
 */
keyboard_t sdlEngine_getKeys();

#endif /* SDL_ENGINE_H */
