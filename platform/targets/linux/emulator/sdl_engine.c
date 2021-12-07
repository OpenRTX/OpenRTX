/***************************************************************************
 *   Copyright (C) 2021 by Alessio Caiazza IU5BON                          *
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
#include "sdl_engine.h"
#include "emulator.h"

#include <stdlib.h>
#include <pthread.h>
#include <state.h>
#include <interfaces/keyboard.h>
#include <SDL2/SDL.h>

/* Shared channel to receive frame buffer updates */
chan_t fb_sync;

SDL_Window   *window;
SDL_Renderer *renderer;
SDL_Texture  *displayTexture;

/* Custom SDL Event to request a screenshot */
Uint32 SDL_Screenshot_Event;
/* Custom SDL Event to change backlight */
Uint32 SDL_Backlight_Event;

/*
 *  Mutex protected variables
 */
pthread_mutex_t mu;
bool            ready = false;  /* Signal if the main loop is ready */
keyboard_t      sdl_keys;       /* Store the keyboard status */

extern state_t state;

bool sdk_key_code_to_key(SDL_Keycode sym, keyboard_t *key)
{
    switch (sym)
    {
        case SDLK_0:
            *key = KEY_0;
            return true;

        case SDLK_1:
            *key = KEY_1;
            return true;

        case SDLK_2:
            *key = KEY_2;
            return true;

        case SDLK_3:
            *key = KEY_3;
            return true;

        case SDLK_4:
            *key = KEY_4;
            return true;

        case SDLK_5:
            *key = KEY_5;
            return true;

        case SDLK_6:
            *key = KEY_6;
            return true;

        case SDLK_7:
            *key = KEY_7;
            return true;

        case SDLK_8:
            *key = KEY_8;
            return true;

        case SDLK_9:
            *key = KEY_9;
            return true;

        case SDLK_ASTERISK:
            *key = KEY_STAR;
            return true;

        case SDLK_ESCAPE:
            *key = KEY_ESC;
            return true;

        case SDLK_LEFT:
            *key = KEY_LEFT;
            return true;

        case SDLK_RIGHT:
            *key = KEY_RIGHT;
            return true;

        case SDLK_RETURN:
            *key = KEY_ENTER;
            return true;

        case SDLK_HASH:
            *key = KEY_HASH;
            return true;

        case SDLK_n:
            *key = KEY_F1;
            return true;

        case SDLK_m:
            *key = KEY_MONI;
            return true;

        case SDLK_PAGEUP:
            *key = KNOB_LEFT;
            return true;

        case SDLK_PAGEDOWN:
            *key = KNOB_RIGHT;
            return true;

        case SDLK_UP:
            *key = KEY_UP;
            return true;

        case SDLK_DOWN:
            *key = KEY_DOWN;
            return true;

        default:
            return false;
    }
}

int screenshot_display(const char *filename)
{
    /*
     * https://stackoverflow.com/a/48176678
     * user1902824
     * modified to keep renderer and display texture references in the body
     * rather than as a parameter
     */
    SDL_Renderer *ren = renderer;
    SDL_Texture  *tex = displayTexture;
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

    if (st != 0)
    {
        SDL_Log("Failed querying texture: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }

    ren_tex = SDL_CreateTexture(ren, format, SDL_TEXTUREACCESS_TARGET, w, h);

    if (!ren_tex)
    {
        SDL_Log("Failed creating render texture: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }

    /*
     * Initialize our canvas, then copy texture to a target whose pixel data we
     * can access
     */
    st = SDL_SetRenderTarget(ren, ren_tex);

    if (st != 0)
    {
        SDL_Log("Failed setting render target: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }

    SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(ren);
    st = SDL_RenderCopy(ren, tex, NULL, NULL);

    if (st != 0)
    {
        SDL_Log("Failed copying texture data: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }

    /* Create buffer to hold texture data and load it */
    pixels = malloc(w * h * SDL_BYTESPERPIXEL(format));

    if (!pixels)
    {
        SDL_Log("Failed allocating memory\n");
        err++;
        goto cleanup;
    }

    st = SDL_RenderReadPixels(ren, NULL, format, pixels,
                              w * SDL_BYTESPERPIXEL(format));
    if (st != 0)
    {
        SDL_Log("Failed reading pixel data: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }

    /* Copy pixel data over to surface */
    surf = SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h,
                                              SDL_BITSPERPIXEL(format),
                                              w * SDL_BYTESPERPIXEL(format),
                                              format);
    if (!surf)
    {
        SDL_Log("Failed creating new surface: %s\n", SDL_GetError());
        err++;
        goto cleanup;
    }

    /* Save result to an image */
    st = SDL_SaveBMP(surf, filename);

    if (st != 0)
    {
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

bool sdl_main_loop_ready()
{
    pthread_mutex_lock(&mu);
    bool is_ready = ready;
    pthread_mutex_unlock(&mu);

    return is_ready;
}

keyboard_t sdl_getKeys()
{
    pthread_mutex_lock(&mu);
    keyboard_t keys = sdl_keys;
    pthread_mutex_unlock(&mu);

    return keys;
}

bool set_brightness(uint8_t brightness)
{
    /*
     * When this texture is rendered, during the copy operation each source
     * color channel is modulated by the appropriate color value according to
     * the following formula:
     * srcC = srcC * (color / 255)
     *
     * Color modulation is not always supported by the renderer;
     * it will return -1 if color modulation is not supported.
     */
    return SDL_SetTextureColorMod(displayTexture,
                                  brightness,
                                  brightness,
                                  brightness) == 0;
}

/*
 * SDL main loop. Due to macOS restrictions, this must run on the Main Thread.
 */
void sdl_task()
{
    pthread_mutex_lock(&mu);
    ready = true;
    pthread_mutex_unlock(&mu);

    SDL_Event ev = { 0 };

    while (!Radio_State.PowerOff)
    {
        keyboard_t key = 0;
        if (SDL_PollEvent(&ev) == 1)
        {
            switch (ev.type)
            {
                case SDL_QUIT:
                    Radio_State.PowerOff = true;
                    break;

                case SDL_KEYDOWN:
                    if (sdk_key_code_to_key(ev.key.keysym.sym, &key))
                    {
                        pthread_mutex_lock(&mu);
                        sdl_keys |= key;
                        pthread_mutex_unlock(&mu);
                    }
                    break;

                case SDL_KEYUP:
                    if (sdk_key_code_to_key(ev.key.keysym.sym, &key))
                    {
                        pthread_mutex_lock(&mu);
                        sdl_keys ^= key;
                        pthread_mutex_unlock(&mu);
                    }
                    break;
            }
            if (ev.type == SDL_Screenshot_Event)
            {
                char *filename = (char *)ev.user.data1;
                screenshot_display(filename);
                free(ev.user.data1);
            }
            else if (ev.type == SDL_Backlight_Event)
            {
                set_brightness(*((uint8_t*)ev.user.data1));
                free(ev.user.data1);
            }
        }

        // we update the window only if there is a something ready to render
        if (chan_can_send(&fb_sync))
        {
            PIXEL_SIZE *pixels;
            int pitch = 0;

            if (SDL_LockTexture(displayTexture, NULL,
                                (void **) &pixels, &pitch) < 0)
            {
                SDL_Log("SDL_lock failed: %s", SDL_GetError());
            }

            chan_send(&fb_sync, pixels);
            chan_recv(&fb_sync, NULL);

            SDL_UnlockTexture(displayTexture);
            SDL_RenderCopy(renderer, displayTexture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
}

void init_sdl()
{
    pthread_mutex_init(&mu, NULL);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
        printf("SDL video init error!!\n");
        exit(1);
    }

    // Register SDL custom events to handle screenshot requests and backlight
    SDL_Screenshot_Event = SDL_RegisterEvents(2);
    SDL_Backlight_Event = SDL_Screenshot_Event+1;

    chan_init(&fb_sync);

    window = SDL_CreateWindow("OpenRTX",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH * 3, SCREEN_HEIGHT * 3,
                              SDL_WINDOW_SHOWN );

    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    displayTexture = SDL_CreateTexture(renderer,
                                       PIXEL_FORMAT,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       SCREEN_WIDTH,
                                       SCREEN_HEIGHT);
    SDL_RenderClear(renderer);

    if(!set_brightness(state.settings.brightness))
    {
         SDL_Log("Cannot apply brightness: %s", SDL_GetError());
    }

    SDL_RenderCopy(renderer, displayTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
}
