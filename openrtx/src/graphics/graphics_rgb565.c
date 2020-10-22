/***************************************************************************
 *   Copyright (C) 2020 by Federico Izzo IU2NUO,                           *
 *                         Niccolò Izzo IU2KIN,                            *
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

#include <string.h>
#include <stdio.h>
#include "display.h"
#include "graphics.h"
#include "font_OpenGD77.h"

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

void gfx_init()
{
    display_init();
    // Set global variables and framebuffer pointer
    screen_width = gfx_screenWidth();
    screen_height = gfx_screenHeight();
    buf = (rgb565_t *)(display_getFrameBuffer());
    initialized = 1;
}

void gfx_terminate()
{
    display_terminate();
    initialized = 0;
}

uint16_t gfx_screenWidth()
{
    return display_screenWidth();
}

uint16_t gfx_screenHeight()
{
    return display_screenHeight();
}

void gfx_renderRows(uint8_t startRow, uint8_t endRow)
{
    display_renderRows(startRow, endRow);
}

void gfx_render()
{
    display_render();
}

bool gfx_renderingInProgress()
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

void gfx_clearScreen()
{
    if(!initialized) return;
    // Set the whole framebuffer to 0x00 = make the screen black
    memset(buf, 0x00, sizeof(rgb565_t) * screen_width * screen_height);
}

void gfx_fillScreen(color_t color)
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

void gfx_setPixel(point_t pos, color_t color)
{
    if (pos.x > screen_width || pos.y > screen_height)
        return; // off the screen

    buf[pos.x + pos.y*screen_width] = _true2highColor(color);
}

void gfx_drawLine(point_t start, point_t end, color_t color)
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

void gfx_drawRect(point_t start, uint16_t width, uint16_t height, color_t color, bool fill)
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

void gfx_print(point_t start, const char *text, fontSize_t size, textAlign_t alignment, color_t color)
{
    uint8_t *currentCharData;
    uint8_t *currentFont;
    uint16_t *writePos;
    uint8_t *readPos;

    rgb565_t color_565 = _true2highColor(color);

    switch(size)
    {
        case FONT_SIZE_1:
            currentFont = (uint8_t *) font_6x8;
            break;
        case FONT_SIZE_1_BOLD:
            currentFont = (uint8_t *) font_6x8_bold;
            break;
        case FONT_SIZE_2:
            currentFont = (uint8_t *) font_8x8;
            break;
        case FONT_SIZE_3:
            currentFont = (uint8_t *) font_8x16;
            break;
        case FONT_SIZE_4:
            currentFont = (uint8_t *) font_16x32;
            break;
        default:
            return;// Invalid font selected
            break;
    }

    int16_t startCode           = currentFont[2];  // get first defined character
    int16_t endCode             = currentFont[3];  // get last defined character
    int16_t charWidthPixels     = currentFont[4];  // width in pixel of one char
    int16_t charHeightPixels    = currentFont[5];  // page count per char
    int16_t bytesPerChar        = currentFont[7];  // bytes per char

    int16_t sLen = strlen(text);
    // Compute amount of letters that fit till the end of the screen
    if ((charWidthPixels*sLen) + start.x > screen_width)
    {
        sLen = (screen_width-start.x)/charWidthPixels;
    }

    if (sLen < 0) return;

    switch(alignment)
    {
        case TEXT_ALIGN_LEFT:
            // left aligned, do nothing.
            break;
        case TEXT_ALIGN_CENTER:
            start.x = (screen_width - (charWidthPixels * sLen))/2;
            break;
        case TEXT_ALIGN_RIGHT:
            start.x = screen_width - (charWidthPixels * sLen);
            break;
    }

    for (int16_t i=0; i<sLen; i++)
    {
        uint32_t charOffset = (text[i] - startCode);

        // End boundary checking.
        if (charOffset > endCode)
        {
            charOffset = ('?' - startCode); // Substitute unsupported ASCII code by a question mark
        }

        currentCharData = (uint8_t *)&currentFont[8 + (charOffset * bytesPerChar)];

        // We print the character from up-left to bottom right
        for(int16_t vscan=0; vscan < charHeightPixels; vscan++) {
            for(int16_t hscan=0; hscan < charWidthPixels; hscan++) {
                int16_t charChunk = vscan / 8;
                int16_t bitIndex = (hscan + charChunk * charWidthPixels) * 8 +
                                    vscan % 8;
                int16_t byte = bitIndex >> 3;
                int16_t bitMask = 1 << (bitIndex & 7);
                if (currentCharData[byte] & bitMask)
                    buf[(start.y + vscan) * screen_width +
                         start.x + hscan + i * charWidthPixels] = color_565;
            }
        }
    }
}
