/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Izzo IU2NUO,                    *
 *                                Niccolò Izzo IU2KIN,                     *
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

/**
 * This driver provides an lcd screen emulator to allow UI development and
 * testing on a x86/x64 computer.
 * Graphics control is provided through SDL2 library, you need to have the SDL2
 * development library installed on your machine to compile and run code using
 * this driver.
 */

#include <SDL2/SDL.h>
#include <emulator/sdl_engine.h>
#include <stdio.h>
#include <string.h>

#include "core/chan.h"
#include "interfaces/display.h"

void *frameBuffer = NULL;    /* Pointer to framebuffer */
bool inProgress;             /* Flag to signal when rendering is in progress */

/*
 * SDL main loop syncronization
 */
bool sdl_ready = false;      /* Flag to signal the sdl main loop is running */
extern chan_t fb_sync;       /* Shared channel to send a frame buffer update */

/**
 * @internal
 * Internal helper function which fetches pixel at position (x, y) from framebuffer
 * and returns it in SDL-compatible format, which is ARGB8888.
 */
uint32_t fetchPixelFromFb(unsigned int x, unsigned int y)
{
    (void) x;
    (void) y;
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
    /*
     * Black and white pixel format: framebuffer type is uint8_t where each
     * bit represents a pixel. We have to allocate
     * (SCREEN_HEIGHT * SCREEN_WIDTH)/8 elements
     */
    #ifdef PIX_FMT_BW
    unsigned int fbSize = (SCREEN_HEIGHT * SCREEN_WIDTH)/8;
    /* Compensate for eventual truncation error in division */
    if((fbSize * 8) < (SCREEN_HEIGHT * SCREEN_WIDTH)) fbSize += 1;
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

void display_terminate()
{
    while (inProgress){ }         /* Wait until current render finishes */
    chan_close(&fb_sync);
    chan_terminate(&fb_sync);
    if(frameBuffer != NULL) free(frameBuffer);
    frameBuffer = NULL;
}

void display_renderRows(uint8_t startRow, uint8_t endRow)
{
    (void) startRow;
    (void) endRow;
    inProgress = true;
    if(!sdl_ready)
    {
        sdl_ready = sdlEngine_ready();
    }

    if(sdl_ready)
    {
        // receive a texture pixel map
        void *fb;
        chan_recv(&fb_sync, &fb);
        #ifdef PIX_FMT_RGB565
        memcpy(fb, frameBuffer, sizeof(PIXEL_SIZE) * SCREEN_HEIGHT * SCREEN_WIDTH);
        #else
        for (unsigned int x = 0; x < SCREEN_WIDTH; x++)
        {
            for (unsigned int y = startRow; y < endRow; y++)
            {
                pixels[x + y * SCREEN_WIDTH] = fetchPixelFromFb(x, y);
            }
        }
        #endif
        // signal the SDL main loop to proceed with rendering
        void *done = {0};
        chan_send(&fb_sync, done);
    }

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

void display_setContrast(uint8_t contrast)
{
    printf("Setting display contrast to %d\n", contrast);
}
