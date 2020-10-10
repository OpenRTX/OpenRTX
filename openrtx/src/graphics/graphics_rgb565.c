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
 * This source file provides an RGB implementation for the graphics.h interface
 * It is suitable for color displays, it will have grayscale and B/W counterparts
 */

#include "display.h"
#include "graphics.h"
#include <string.h>
#include <stdio.h>

#define COLOR_WHITE = {31, 63, 31}

/* This graphics driver is meant for an RGB565 little endian pixel format.
 * Thus, to accomodate for the endianness, the fields in struct rgb565_t have to
 * be written in reversed order.
 *
 * For more details about endianness and bitfield structs see the following web
 * page: http://mjfrazer.org/mjfrazer/bitfields/
 */

typedef struct
{
    uint16_t b : 5;
    uint16_t g : 6;
    uint16_t r : 5;
} rgb565_t;

bool initialized = 0;
uint16_t screen_width = 0;
uint16_t screen_height = 0;
rgb565_t *buf;

void graphics_init()
{
    display_init();
    // Set global variables and framebuffer pointer
    screen_width = graphics_screenWidth();
    screen_height = graphics_screenHeight();
    buf = (rgb565_t *)(display_getFrameBuffer());
    initialized = 1;
}

void graphics_terminate()
{
    display_terminate();
    initialized = 0;
}

uint16_t graphics_screenWidth()
{
    return display_screenWidth();
}

uint16_t graphics_screenHeight()
{
    return display_screenHeight();
}

void graphics_setBacklightLevel(uint8_t level)
{
    display_setBacklightLevel(level);
}

void graphics_renderRows(uint8_t startRow, uint8_t endRow)
{
    display_renderRows(startRow, endRow);
}

void graphics_render()
{
    display_render();
}

bool graphics_renderingInProgress()
{
    return display_renderingInProgress();
}

rgb565_t _true2highColor(color_t true_color)
{
    rgb565_t high_color;
    high_color.r = true_color.r >> 3;
    high_color.g = true_color.g >> 2;
    high_color.b = true_color.b >> 3;

    return high_color;
}

void graphics_clearScreen()
{
    if(!initialized) return;
    // Set the whole framebuffer to 0x00 = make the screen black
    memset(buf, 0x00, sizeof(rgb565_t) * screen_width * screen_height);
}

void graphics_fillScreen(color_t color)
{
    if(!initialized) return;
    rgb565_t color_565 = _true2highColor(color);
    for(int y = 0; y < screen_height; y++)
    {
        for(int x = 0; x < screen_width; x++)
        {
            buf[x + y*screen_width] = color_565;
        }
    }
}

void graphics_drawLine(point_t start, point_t end, color_t color)
{
    if(!initialized) return;
    rgb565_t color_565 = _true2highColor(color);
    for(int y = start.y; y < end.y; y++)
    {
        for(int x = start.x; x < end.x; x++)
        {
            buf[x + y*screen_width] = color_565;
        }
    }
}

void graphics_drawRect(point_t start, uint16_t width, uint16_t height, color_t color, bool fill)
{
    if(!initialized) return;
    rgb565_t color_565 = _true2highColor(color);
    uint16_t x_max = start.x + width;
    uint16_t y_max = start.y + height;
    bool perimeter = 0;
    if(x_max > (screen_width - 1)) x_max = screen_width - 1;
    if(y_max > (screen_height - 1)) y_max = screen_height - 1;
    for(int y = start.y; y < y_max; y++)
    {
        for(int x = start.x; x < x_max; x++)
        {
            if(y == start.y || y == y_max-1 || x == start.x || x == x_max-1) perimeter = 1;
            else perimeter = 0;
            // If fill is false, draw only rectangle perimeter
            if(fill || perimeter) buf[x + y*screen_width] = color_565;
        }
    }
}

