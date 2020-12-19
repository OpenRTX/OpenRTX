/***************************************************************************
 *   Copyright (C) 2020 by Federico Izzo IU2NUO, Niccolò Izzo IU2KIN and   *
 *                         Silvano Seva IU2KWO                             *
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

/**
 * This driver provides an lcd screen emulator to allow UI development and
 * testing on a x86/x64 computer.
 * Graphics control is provided through SDL2 library, you need to have the SDL2
 * development library installed on your machine to compile and run code using
 * this driver.
 */

#include "display.h"
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>

#undef main            /* necessary to avoid conflicts with SDL_main */

/*
 * Screen dimensions, adjust basing on the size of the screen you need to
 * emulate
 */
#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 160
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 128
#endif

#ifdef PIX_FMT_RGB565
#define PIXEL_FORMAT SDL_PIXELFORMAT_RGB565
#define PIXEL_SIZE uint16_t
#else
#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888
#define PIXEL_SIZE uint32_t
#endif

SDL_Renderer *renderer;      /* SDL renderer           */
SDL_Window *window;          /* SDL window             */
SDL_Texture *displayTexture; /* SDL rendering surface  */
void *frameBuffer;           /* Pointer to framebuffer */
bool inProgress;             /* Flag to signal when rendering is in progress */

/**
 * @internal
 * Internal helper function which fetches pixel at position (x, y) from framebuffer
 * and returns it in SDL-compatible format, which is ARGB8888.
 */
uint32_t fetchPixelFromFb(unsigned int x, unsigned int y)
{
    uint32_t pixel = 0;

#ifdef PIX_FMT_BW
    /*
     * Black and white 1bpp format: framebuffer is an array of uint8_t, where
     * each cell contains the values of eight pixels, one per bit.
     */
    uint8_t *fb = (uint8_t *)(frameBuffer);
    unsigned int cell = (x + y*SCREEN_WIDTH) / 8;
    unsigned int elem = (x + y*SCREEN_WIDTH) % 8;
    if(fb[cell] & (1 << elem)) pixel = 0xFFFFFFFF;
#endif

#ifdef PIX_FMT_GRAYSC
    /*
     * Convert from 8bpp grayscale to ARGB8888, we have to do nothing more that
     * replicating the pixel value for the three components
     */
    uint8_t *fb = (uint8_t *)(frameBuffer);
    uint8_t px = fb[x + y*SCREEN_WIDTH];

    pixel = 0xFF000000 | (px << 16) | (px << 8) | px;
#endif
    return pixel;
}


void display_init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL video init error!!\n");

    } else
    {

        window = SDL_CreateWindow("OpenRTX",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SCREEN_WIDTH * 3, SCREEN_HEIGHT * 3,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

        renderer = SDL_CreateRenderer(window, -1, 0);
        SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
        displayTexture = SDL_CreateTexture(renderer, PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING,
                                           SCREEN_WIDTH, SCREEN_HEIGHT);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, displayTexture, NULL, NULL);
        SDL_RenderPresent(renderer);

        /*
         * Black and white pixel format: framebuffer type is uint8_t where each
         * bit represents a pixel. We have to allocate
         * (SCREEN_HEIGHT * SCREEN_WIDTH)/8 elements
         */
#ifdef PIX_FMT_BW
        unsigned int fbSize = (SCREEN_HEIGHT * SCREEN_WIDTH)/8;
        if((fbSize * 8) < (SCREEN_HEIGHT * SCREEN_WIDTH)) fbSize += 1; /* Compensate for eventual truncation error in division */
        fbSize *= sizeof(uint8_t);
#endif

        /*
         * Grayscale pixel format: framebuffer type is uint8_t where each element
         * controls one pixel
         */
#ifdef PIX_FMT_GRAYSC
        unsigned int fbSize = SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(uint8_t);
#endif

        /*
         * RGB565 pixel format: framebuffer type is uint16_t where each element
         * controls one pixel
         */
#ifdef PIX_FMT_RGB565
        unsigned int fbSize = SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(uint16_t);
#endif

        frameBuffer = malloc(fbSize);
        memset(frameBuffer, 0xFFFF, fbSize);
        inProgress = false;
    }
}

void display_terminate()
{
    while (inProgress)
    {}         /* Wait until current render finishes */
    printf("Terminating SDL display emulator, goodbye!\n");
    free(frameBuffer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void display_renderRows(uint8_t startRow, uint8_t endRow)
{
    PIXEL_SIZE *pixels;
    int pitch = 0;
    if (SDL_LockTexture(displayTexture, NULL, (void **) &pixels, &pitch) < 0)
    {
        printf("SDL_lock failed: %s\n", SDL_GetError());
    }
    inProgress = true;
#ifdef PIX_FMT_RGB565
    uint16_t *fb = (uint16_t *) (frameBuffer);
    memcpy(pixels, fb, sizeof(uint16_t) * SCREEN_HEIGHT * SCREEN_WIDTH);
#else
    for (unsigned int x = 0; x < SCREEN_WIDTH; x++) {
        for (unsigned int y = startRow; y < endRow; y++) {
            pixels[x + y * SCREEN_WIDTH] = fetchPixelFromFb(x, y);
        }
    }
#endif
    SDL_UnlockTexture(displayTexture);
    SDL_RenderCopy(renderer, displayTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
    inProgress = false;
}

void display_render()
{
    display_renderRows(0, SCREEN_HEIGHT);
}

bool display_renderingInProgress()
{
    return inProgress;
}

void *display_getFrameBuffer()
{
    return (void *) (frameBuffer);
}
