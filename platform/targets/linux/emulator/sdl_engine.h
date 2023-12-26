/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Alessio Caiazza IU5BON                   *
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
#ifndef SDL_ENGINE_H
#define SDL_ENGINE_H

#include <interfaces/keyboard.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <chan.h>

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
