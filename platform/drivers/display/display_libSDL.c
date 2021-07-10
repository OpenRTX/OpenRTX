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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
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

#include <interfaces/display.h>
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


int screenshot_display(const char *filename)
{
    //https://stackoverflow.com/a/48176678
    //user1902824
    //modified to keep renderer and display texture references in the body rather than as a parameter
    SDL_Renderer * ren = renderer;
    SDL_Texture * tex = displayTexture;
    int err = 0;


    SDL_Texture *ren_tex;
    SDL_Surface *surf;
    int st;
    int w;
    int h;
    int format;
    void *pixels;

    pixels  = NULL;
    surf    = NULL;
    ren_tex = NULL;
    format  = SDL_PIXELFORMAT_RGBA32;

    /* Get information about texture we want to save */
    st = SDL_QueryTexture(tex, NULL, NULL, &w, &h);
    if (st != 0) {
        SDL_Log("Failed querying texture: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }

    ren_tex = SDL_CreateTexture(ren, format, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!ren_tex) {
        SDL_Log("Failed creating render texture: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }

    /*
     * Initialize our canvas, then copy texture to a target whose pixel data we
     * can access
     */
    st = SDL_SetRenderTarget(ren, ren_tex);
    if (st != 0) {
        SDL_Log("Failed setting render target: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }
    SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(ren);
    st = SDL_RenderCopy(ren, tex, NULL, NULL);
    if (st != 0) {
        SDL_Log("Failed copying texture data: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }
    /* Create buffer to hold texture data and load it */
    pixels = malloc(w * h * SDL_BYTESPERPIXEL(format));
    if (!pixels) {
        SDL_Log("Failed allocating memory\n");
        err++;
        goto cleanup;
    }
    st = SDL_RenderReadPixels(ren, NULL, format, pixels, w * SDL_BYTESPERPIXEL(format));
    if (st != 0) {
        SDL_Log("Failed reading pixel data: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }
    /* Copy pixel data over to surface */
    surf = SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h, SDL_BITSPERPIXEL(format), w * SDL_BYTESPERPIXEL(format), format);
    if (!surf) {
        SDL_Log("Failed creating new surface: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }
    /* Save result to an image */
    st = SDL_SaveBMP(surf, filename);
    if (st != 0) {
        SDL_Log("Failed saving image: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }
    SDL_Log("Saved texture as BMP to \"%s\"\n", filename);

cleanup:
    SDL_FreeSurface(surf);
    free(pixels);
    SDL_DestroyTexture(ren_tex);
    return err;
}


/**
 * @internal
 * Internal helper function which fetches pixel at position (x, y) from framebuffer
 * and returns it in SDL-compatible format, which is ARGB8888.
 */
uint32_t fetchPixelFromFb(__attribute__((unused)) unsigned int x,
                          __attribute__((unused)) unsigned int y)
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
                                  SDL_WINDOW_SHOWN ); 
        //removed RESIZABLE flag so automatic screen recording is a little easier

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

void display_renderRows(__attribute__((unused)) uint8_t startRow,
                        __attribute__((unused)) uint8_t endRow)
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

void display_setContrast(uint8_t contrast)
{
    printf("Setting display contrast to %d\n", contrast);
}

