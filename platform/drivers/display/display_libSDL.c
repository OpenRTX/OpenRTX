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

#include "lcd.h"
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#undef main            /* necessary to avoid conflicts with SDL_main */

/* 
 * Screen dimensions, adjust basing on the size of the screen you need to
 * emulate
 */
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 128

SDL_Window  *window;
SDL_Surface *renderSurface;
uint16_t *frameBuffer;
bool inProgress;

void lcd_init()
{
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL video init error!!\n");

    }
    else
    {

        window = SDL_CreateWindow(" ",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SCREEN_WIDTH,SCREEN_HEIGHT,
                                  SDL_WINDOW_SHOWN);

        renderSurface = SDL_GetWindowSurface(window);
        SDL_FillRect(renderSurface,NULL,0xFFFFFF);

        unsigned int scrSize = SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(uint16_t);
        frameBuffer = (uint16_t *)(malloc(scrSize));
        memset(frameBuffer, 0xFFFF, scrSize);
        inProgress = false;
    }
}

void lcd_terminate()
{
    while(inProgress) { }         /* Wait until current render finishes */
    printf("Terminating SDL display emulator, goodbye!\n");
    free(frameBuffer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

uint16_t lcd_screenWidth()
{
    return SCREEN_WIDTH;
}

uint16_t lcd_screenHeight()
{
    return SCREEN_HEIGHT;
}

void lcd_setBacklightLevel(uint8_t level)
{
    printf("Backlight level set to %d\n", level);
}

void lcd_renderRows(uint8_t startRow, uint8_t endRow)
{
    Uint32 *pixels = (Uint32*)renderSurface->pixels;
    inProgress = true;

    for(int x = 0; x < SCREEN_WIDTH; x++)
    {
        for(int y = startRow; y < endRow; y++)
        {
            /*
             * SDL pixel format is ARGB8888, while ours is RGB565, thus we need
             * to do some conversions when writing framebuffer content to the
             * window. We also set alpha value to its maximum.
             */
            uint32_t r = (frameBuffer[x + y*SCREEN_WIDTH] & 0xF800) >> 11;
            uint32_t g = (frameBuffer[x + y*SCREEN_WIDTH] & 0x07E0) >> 5;
            uint32_t b = (frameBuffer[x + y*SCREEN_WIDTH] & 0x001F) & 0x1F;

            /*
             * Here we do conversions by multiplying by some scaling factors,
             * we use ints just because the precision of floats is not really
             * needed.
             * Conversion factors:
             * - five bit to eight bit: 8.226
             * - six bit to eight bit: 4.0476
             */
            r = (r * 8) + (r * 226)/1000;
            g = (g * 4) + (g * 476)/10000;
            b = (b * 8) + (b * 226)/1000;

            pixels[x + y*SCREEN_WIDTH] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }

    inProgress = false;
    SDL_UpdateWindowSurface(window);
}

void lcd_render()
{
    lcd_renderRows(0, SCREEN_HEIGHT);
}

bool lcd_renderingInProgress()
{
    return inProgress;
}

void *lcd_getFrameBuffer()
{
    return (void *)(frameBuffer);
}
