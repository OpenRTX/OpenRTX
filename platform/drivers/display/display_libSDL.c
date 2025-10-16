/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * This driver provides an lcd screen emulator to allow UI development and
 * testing on a x86/x64 computer.
 * Graphics control is provided through SDL2 library, you need to have the SDL2
 * development library installed on your machine to compile and run code using
 * this driver.
 */

#include "interfaces/display.h"
#include "emulator/sdl_engine.h"
#include "core/chan.h"
#include <stdio.h>
#include <string.h>
#include "SDL2/SDL.h"

static bool inProgress;             /* Flag to signal when rendering is in progress */

/*
 * SDL main loop syncronization
 */
bool sdl_ready = false;      /* Flag to signal the sdl main loop is running */
extern chan_t fb_sync;       /* Shared channel to send a frame buffer update */

/* Custom SDL Event to adjust backlight */
extern Uint32 SDL_Backlight_Event;

#ifndef CONFIG_PIX_FMT_RGB565
/**
 * @internal
 * Internal helper function which fetches pixel at position (x, y) from framebuffer
 * and returns it in SDL-compatible format, which is ARGB8888.
 */
static uint32_t fetchPixelFromFb(unsigned int x, unsigned int y, void *fb)
{
    (void) x;
    (void) y;
    (void) fb;
    uint32_t pixel = 0;

    #ifdef CONFIG_PIX_FMT_BW
    /*
     * Black and white 1bpp format: framebuffer is an array of uint8_t, where
     * each cell contains the values of eight pixels, one per bit.
     */
    uint8_t *buf = (uint8_t *)(fb);
    unsigned int cell = (x + y*CONFIG_SCREEN_WIDTH) / 8;
    unsigned int elem = (x + y*CONFIG_SCREEN_WIDTH) % 8;
    if(buf[cell] & (1 << elem)) pixel = 0xFFFFFFFF;
    #endif

    #ifdef PIX_FMT_GRAYSC
    /*
     * Convert from 8bpp grayscale to ARGB8888, we have to do nothing more that
     * replicating the pixel value for the three components
     */
    uint8_t *buf = (uint8_t *)(fb);
    uint8_t px = buf[x + y*CONFIG_SCREEN_WIDTH];

    pixel = 0xFF000000 | (px << 16) | (px << 8) | px;
    #endif

    return pixel;
}
#endif

void display_init()
{
    inProgress = false;
}

void display_terminate()
{
    while (inProgress){ }         /* Wait until current render finishes */
    chan_close(&fb_sync);
    chan_terminate(&fb_sync);
}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
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
        void *pixelMap;
        chan_recv(&fb_sync, &pixelMap);
        #ifdef CONFIG_PIX_FMT_RGB565
        memcpy(pixelMap, fb, sizeof(PIXEL_SIZE) * CONFIG_SCREEN_HEIGHT * CONFIG_SCREEN_WIDTH);
        #else
        uint32_t *pixels = (uint32_t *) pixelMap;
        for (unsigned int x = 0; x < CONFIG_SCREEN_WIDTH; x++)
        {
            for (unsigned int y = startRow; y < endRow; y++)
            {
                pixels[x + y * CONFIG_SCREEN_WIDTH] = fetchPixelFromFb(x, y, fb);
            }
        }
        #endif
        // signal the SDL main loop to proceed with rendering
        void *done = {0};
        chan_send(&fb_sync, done);
    }

    inProgress = false;
}

void display_render(void *fb)
{
    display_renderRows(0, CONFIG_SCREEN_HEIGHT, fb);
}

void display_setContrast(uint8_t contrast)
{
    printf("Setting display contrast to %d\n", contrast);
}

void display_setBacklightLevel(uint8_t level)
{
    // Saturate level to 100 and convert value to 0 - 255
    if(level > 100) level = 100;
    uint16_t value = (2 * level) + (level * 55)/100;

    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_Backlight_Event;
    e.user.data1 = malloc(sizeof(uint8_t));
    uint8_t *data = (uint8_t *)e.user.data1;
    *data = ((uint8_t) value);

    SDL_PushEvent(&e);
}
