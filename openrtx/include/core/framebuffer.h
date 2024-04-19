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

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

/**
 * This source file provides an implementation for the graphics framebuffer. 
 * It is suitable for both color, grayscale and B/W display.
 * It assumes that the screen is rendered more often than it is written to.
 * Thus it stores the framebuffer in an order optimised to send it. This order 
 * may not be linear.
 *
 * Mode 0: this mode is the default framebuffer behavior, that is, the buffer
 * is stored linearly, line-by-line.
 *
 * Mode 1: this mode is optimized for SSD1309-like screens.
 * The following indices are the indices of the position in the storage
 * (for a screen of 16 lines and 4 columns)
 *  --------------------> columns
 *  |  0   8   16  24
 *  |  1   9   17  25
 *  |  2   10  18  26
 *  |  3   11  19  27
 *  |  4   12  20  28
 *  |  5   13  21  29
 *  |  6   14  22  30
 *  |  7   15  23  31
 *  |  32  40  48  56
 *  |  33  41  49  57
 *  |  34  42  50  58
 *  |  35  43  51  59
 *  |  36  44  52  60
 *  |  37  45  53  61
 *  |  38  46  54  62
 *  |  39  47  55  63
 *  V
 * lines
 */

#include <hwconfig.h>
#include <stdlib.h>
#include <graphics.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_PIX_FMT_RGB565

/* This specialization is meant for an RGB565 little endian pixel format.
 * Thus, to accomodate for the endianness, the fields in struct rgb565_t have to
 * be written in reversed order.
 *
 * For more details about endianness and bitfield structs see the following web
 * page: http://mjfrazer.org/mjfrazer/bitfields/
 */

#define PIXEL_T rgb565_t
#define FB_SIZE (CONFIG_SCREEN_HEIGHT * CONFIG_SCREEN_WIDTH)


typedef struct
{
    uint16_t b : 5;
    uint16_t g : 6;
    uint16_t r : 5;
}
rgb565_t;

static rgb565_t _true2highColor(color_t true_color)
{
    rgb565_t high_color;
    high_color.r = true_color.r >> 3;
    high_color.g = true_color.g >> 2;
    high_color.b = true_color.b >> 3;

    return high_color;
}

#elif defined CONFIG_PIX_FMT_BW

/**
 * This specialization is meant for black and white pixel format.
 * It is suitable for monochromatic displays with 1 bit per pixel,
 * it will have RGB and grayscale counterparts
 */

#define PIXEL_T uint8_t
#define FB_SIZE (((CONFIG_SCREEN_HEIGHT * CONFIG_SCREEN_WIDTH) / 8 ) + 1)

typedef enum
{
    COLOR_WHITE = 0,
    COLOR_BLACK = 1,
}
bw_t;

static bw_t _color2bw(color_t true_color)
{
    if(true_color.r == 0 &&
       true_color.g == 0 &&
       true_color.b == 0)
        return COLOR_WHITE;
    else
        return COLOR_BLACK;
}

#else
#error Please define a pixel format type into hwconfig.h or meson.build
#endif

/**
 * Initializes the framebuffer in the specified mode
 * @param store_mode: mode in which the data is stored in the framebuffer
 */
void framebuffer_init(uint8_t store_mode);

/**
 * Change the color of a single pixel.
 * @param pos: x,y coordinates of the pixel.
 * @param color: desired pixel color, in color_t format.
 */
void framebuffer_setPixel(point_t pos, color_t color);

/**
 * Returns the color of the pixel at specified position
 * @param pos: position of the pixel to read
 * @return color of the pixel located at pos.
 */
PIXEL_T framebuffer_readPixel(point_t pos);

/**
 * Returns a pointer to the framebuffer storage.
 * @return pointer to the underlying framebuffer array
 */
PIXEL_T *framebuffer_getPointer();

/**
 * Clears the framebuffer
 */
void framebuffer_clear();

/**
 * Terminates the framebuffer
 */
void framebuffer_terminate();

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_H */