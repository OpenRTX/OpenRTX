/***************************************************************************
 *   Copyright (C) 2020 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Silvano Seva IU2KWO,                     *
 *                                Morgan Diepart ON4MOD                    *
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

#include "framebuffer.h"

#include <stdlib.h>
#include <graphics.h>
#include <hwconfig.h>
#include <stddef.h>
#include <stdint.h>

static PIXEL_T __attribute__((section(".bss.fb"))) framebuffer[FB_SIZE];
static uint8_t mode = 0;

#ifdef CONFIG_BIT_BANDING
static uint8_t *bit_banded_framebuffer;
#endif

void framebuffer_init(uint8_t store_mode)
{
    mode = store_mode;
#ifdef CONFIG_BIT_BANDING
    bit_banded_framebuffer = (uint8_t *)(0x22000000UL + 32UL*((uint32_t)framebuffer - 0x20000000UL));
#endif
}

static inline size_t framebuffer_index(point_t pos){
    switch(mode){
        case 0:
        default:
            /* Linear adressing */
            return (pos.x + pos.y*CONFIG_SCREEN_WIDTH);
        case 1:
            /* Adressing per page */
            return (pos.x*8 + pos.y%8 + (pos.y/8)*(CONFIG_SCREEN_WIDTH*8));
    }
}

void framebuffer_setPixel(point_t pos, color_t color)
{
    if (pos.x >= CONFIG_SCREEN_WIDTH || pos.y >= CONFIG_SCREEN_HEIGHT ||
        pos.x < 0 || pos.y < 0)
        return; // off the screen

    // Computes the position in the buffer according to the mode selected
    size_t linear_index = framebuffer_index(pos);
    
#ifdef CONFIG_PIX_FMT_RGB565
        // Blend old pixel value and new one
        rgb565_t pixel = _true2highColor(color);
        if (color.alpha < 255)
        {
            rgb565_t old_pixel = framebuffer[linear_index];
            pixel.r = ((255-color.alpha)*old_pixel.r+color.alpha*pixel.r)/255;
            pixel.g = ((255-color.alpha)*old_pixel.g+color.alpha*pixel.g)/255;
            pixel.b = ((255-color.alpha)*old_pixel.b+color.alpha*pixel.b)/255;
        }
        framebuffer[linear_index] = pixel;
#elif defined CONFIG_PIX_FMT_BW
        // Ignore more than half transparent pixels
        if (color.alpha >= 128)
        {
#ifdef CONFIG_BIT_BANDING
            bit_banded_framebuffer[linear_index*4] = _color2bw(color);
#else
            uint_fast16_t cell = linear_index / 8;
            uint_fast16_t elem = linear_index % 8;
            framebuffer[cell] &= ~(1 << elem);
            framebuffer[cell] |= (_color2bw(color) << elem);
#endif // CONFIG_BIT_BANDING
        }
#endif // CONFIG_PIXEL_FMT
}

PIXEL_T framebuffer_readPixel(point_t pos)
{
    size_t linear_index = framebuffer_index(pos);
#ifdef CONFIG_PIX_FMT_RGB565
    return framebuffer[linear_index];
#elif defined CONFIG_PIX_FMT_BW
    #ifdef CONFIG_BIT_BANDING
        return bit_banded_framebuffer[linear_index*4];
    #else
        uint_fast16_t cell = linear_index / 8;
        uint_fast16_t elem = linear_index % 8;
        return (framebuffer[cell] >> elem) & 0x01;
    #endif // CONFIG_BIT_BANDING
#endif // CONFIG_PIXEL_FMT    
}

PIXEL_T *framebuffer_getPointer()
{
    return framebuffer;
}

void framebuffer_clear()
{
    // Set the whole framebuffer to 0x00 = make the screen black
    memset(framebuffer, 0x00, FB_SIZE * sizeof(PIXEL_T));
}

void framebuffer_terminate(){}