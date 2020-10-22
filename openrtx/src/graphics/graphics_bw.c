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
 * This source file provides a black and white implementation for the graphics.h interface
 * It is suitable for monochromatic displays with 1 bit per pixel, 
 * it will have RGB and grayscale counterparts
 */

#include <string.h>
#include <stdio.h>
#include "display.h"
#include "graphics.h"
#include "font_OpenGD77.h"

typedef enum
{
    WHITE = 0,
    BLACK = 1,
} bw_t;

bool initialized = 0;
uint16_t screen_width = 0;
uint16_t screen_height = 0;
uint8_t *buf;
uint16_t fbSize;

void gfx_init()
{
    display_init();
    // Set global variables and framebuffer pointer
    screen_width = gfx_screenWidth();
    screen_height = gfx_screenHeight();
    buf = (uint8_t *)(display_getFrameBuffer());
    // Calculate framebuffer size
    fbSize = (screen_height * screen_width) / 8;
    /* Compensate for eventual truncation error in division */
    if((fbSize * 8) < (screen_height * screen_width)) fbSize += 1; 
    fbSize *= sizeof(uint8_t);
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

bw_t _color2bw(color_t true_color)
{
    if(true_color.r == 0 && 
       true_color.g == 0 && 
       true_color.b == 0)
        return WHITE;
    else
        return BLACK;
}

void gfx_clearScreen()
{
    if(!initialized) return;
    // Set the whole framebuffer to 0x00 = make the screen white
    memset(buf, 0x00, fbSize);
}

void gfx_fillScreen(color_t color)
{
    if(!initialized) return;
    bw_t bw = _color2bw(color);
    if(bw == WHITE)
        memset(buf, 0x00, fbSize);
    else if(bw == BLACK)
        memset(buf, 0xFF, fbSize);
}

void _bw_setPixel(point_t pos, bw_t bw)
{
    /*
     * Black and white 1bpp format: framebuffer is an array of uint8_t, where
     * each cell contains the values of eight pixels, one per bit.
     */
    uint16_t cell = (pos.x + pos.y*screen_width) / 8;
    uint16_t elem = (pos.x + pos.y*screen_width) % 8;
    buf[cell] &= ~(1 << elem);
    buf[cell] |= (bw << elem);
}

void gfx_setPixel(point_t pos, color_t color)
{
    if (pos.x >= screen_width || pos.y >= screen_height)
        return; // off the screen
    bw_t bw = _color2bw(color);
    _bw_setPixel(pos, bw); 
}

void gfx_drawLine(point_t start, point_t end, color_t color)
{
    if(!initialized) return;
    bw_t bw = _color2bw(color);
    for(int y = start.y; y < end.y; y++)
    {
        for(int x = start.x; x < end.x; x++)
        {
            point_t pos = {x, y};
            _bw_setPixel(pos, bw);
        }
    }
}

void gfx_drawRect(point_t start, uint16_t width, uint16_t height, color_t color, bool fill)
{
    if(!initialized) return;
    bw_t bw = _color2bw(color);
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
            if(fill || perimeter)
            {
                point_t pos = {x, y};
                _bw_setPixel(pos, bw);
            }
        }
    }
}

void gfx_print(point_t start, const char *text, fontSize_t size, textAlign_t alignment, color_t color)
{
    uint8_t *currentCharData;
    uint8_t *currentFont;
    uint16_t *writePos;
    uint8_t *readPos;

    bw_t bw = _color2bw(color);

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
                {
                    point_t pos = {start.x + hscan + i * charWidthPixels, 
                                   start.y + vscan};
                    _bw_setPixel(pos, bw);
                }
            }
        }
    }
}
