/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * This source file provides an  implementation for the graphics.h interface
 * It is suitable for both color, grayscale and B/W display
 */

#include "interfaces/display.h"
#include "hwconfig.h"
#include "core/graphics.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>
#include "core/utils.h"

#include "fonts/adafruit/gfxfont.h"
#include "fonts/adafruit/TomThumb.h"
#include "fonts/adafruit/FreeSans6pt7b.h"
#include "fonts/adafruit/FreeSans8pt7b.h"
#include "fonts/adafruit/FreeSans9pt7b.h"
#include "fonts/adafruit/FreeSans10pt7b.h"
#include "fonts/adafruit/FreeSans12pt7b.h"
#include "fonts/adafruit/FreeSans16pt7b.h"
#include "fonts/adafruit/FreeSans18pt7b.h"
#include "fonts/adafruit/FreeSans24pt7b.h"
#include "fonts/adafruit/UbuntuRegular6pt7b.h"
#include "fonts/adafruit/UbuntuRegular8pt7b.h"
#include "fonts/adafruit/UbuntuRegular9pt7b.h"
#include "fonts/adafruit/UbuntuRegular10pt7b.h"
#include "fonts/adafruit/UbuntuRegular12pt7b.h"
#include "fonts/adafruit/UbuntuRegular16pt7b.h"
#include "fonts/adafruit/UbuntuRegular18pt7b.h"
#include "fonts/adafruit/UbuntuRegular24pt7b.h"

#include "fonts/symbols/Symbols5pt7b.h"
#include "fonts/symbols/Symbols6pt7b.h"
#include "fonts/symbols/Symbols8pt7b.h"

// Variable swap macro
#define DEG_RAD  0.017453292519943295769236907684886
#define SIN(x) sinf((x) * DEG_RAD)
#define COS(x) cosf((x) * DEG_RAD)

/**
 * Fonts, ordered by the fontSize_t enum.
 */
static const GFXfont fonts[] = { TomThumb,            // 5pt
    #if defined FONT_FREE_SANS
                                 FreeSans6pt7b,       // 6pt
                                 FreeSans8pt7b,       // 8pt
                                 FreeSans9pt7b,       // 9pt
                                 FreeSans10pt7b,      // 10pt
                                 FreeSans12pt7b,      // 12pt
                                 FreeSans16pt7b,      // 16pt
    #elif defined FONT_UBUNTU_REGULAR
                                 UbuntuRegular6pt7b,  // 6pt
                                 UbuntuRegular8pt7b,  // 8pt
                                 UbuntuRegular9pt7b,  // 9pt
                                 UbuntuRegular10pt7b, // 10pt
                                 UbuntuRegular12pt7b, // 12pt
                                 UbuntuRegular16pt7b, // 16pt
    #else
    #error Unsupported font family!
    #endif
                                 Symbols5pt7b,      // 5pt
                                 Symbols6pt7b,      // 6pt
                                 Symbols8pt7b       // 8pt
                               };

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
    WHITE = 0,
    BLACK = 1,
}
bw_t;

static bw_t _color2bw(color_t true_color)
{
    if(true_color.r == 0 &&
       true_color.g == 0 &&
       true_color.b == 0)
        return WHITE;
    else
        return BLACK;
}

#else
#error Please define a pixel format type into hwconfig.h or meson.build
#endif

#if defined(PLATFORM_LINUX)
static PIXEL_T framebuffer[FB_SIZE];
#else
static PIXEL_T __attribute__((section(".bss.fb"))) framebuffer[FB_SIZE];
#endif
static char text[32];


void gfx_init()
{
    display_init();

    // Clear text buffer
    memset(text, 0x00, 32);
}

void gfx_terminate()
{
    display_terminate();
}

void gfx_renderRows(uint8_t startRow, uint8_t endRow)
{
    display_renderRows(startRow, endRow, framebuffer);
}

void gfx_render()
{
    display_render(framebuffer);
}

void gfx_clearRows(uint8_t startRow, uint8_t endRow)
{
    if(endRow < startRow)
        return;

    uint16_t start = startRow * CONFIG_SCREEN_WIDTH * sizeof(PIXEL_T);
    uint16_t height = endRow - startRow * CONFIG_SCREEN_WIDTH * sizeof(PIXEL_T);
    // Set the specified rows to 0x00 = make the screen black
    memset(framebuffer + start, 0x00, height);
}

void gfx_clearScreen()
{
    // Set the whole framebuffer to 0x00 = make the screen black
    memset(framebuffer, 0x00, FB_SIZE * sizeof(PIXEL_T));
}

void gfx_fillScreen(color_t color)
{
    for(int16_t y = 0; y < CONFIG_SCREEN_HEIGHT; y++)
    {
        for(int16_t x = 0; x < CONFIG_SCREEN_WIDTH; x++)
        {
            point_t pos = {x, y};
            gfx_setPixel(pos, color);
        }
    }
}

inline void gfx_setPixel(point_t pos, color_t color)
{
    if (pos.x >= CONFIG_SCREEN_WIDTH || pos.y >= CONFIG_SCREEN_HEIGHT ||
        pos.x < 0 || pos.y < 0)
        return; // off the screen

#ifdef CONFIG_PIX_FMT_RGB565
    // Blend old pixel value and new one
    if (color.alpha < 255)
    {
        uint8_t alpha = color.alpha;
        rgb565_t new_pixel = _true2highColor(color);
        rgb565_t old_pixel = framebuffer[pos.x + pos.y*CONFIG_SCREEN_WIDTH];
        rgb565_t pixel;
        pixel.r = ((255-alpha)*old_pixel.r+alpha*new_pixel.r)/255;
        pixel.g = ((255-alpha)*old_pixel.g+alpha*new_pixel.g)/255;
        pixel.b = ((255-alpha)*old_pixel.b+alpha*new_pixel.b)/255;
        framebuffer[pos.x + pos.y*CONFIG_SCREEN_WIDTH] = pixel;
    }
    else
    {
        framebuffer[pos.x + pos.y*CONFIG_SCREEN_WIDTH] = _true2highColor(color);
    }
#elif defined CONFIG_PIX_FMT_BW
    // Ignore more than half transparent pixels
    if (color.alpha >= 128)
    {
        uint16_t cell = (pos.x + pos.y*CONFIG_SCREEN_WIDTH) / 8;
        uint16_t elem = (pos.x + pos.y*CONFIG_SCREEN_WIDTH) % 8;
        framebuffer[cell] &= ~(1 << elem);
        framebuffer[cell] |= (_color2bw(color) << elem);
    }
#endif
}

void gfx_drawLine(point_t start, point_t end, color_t color)
{
    int16_t steep = abs(end.y - start.y) > abs(end.x - start.x);

    if (steep)
    {
        uint16_t tmp;
        // Swap start.x and start.y
        tmp = start.x;
        start.x = start.y;
        start.y = tmp;
        // Swap end.x and end.y
        tmp = end.x;
        end.x = end.y;
        end.y = tmp;
    }

    if (start.x > end.x)
    {
        uint16_t tmp;
        // Swap start.x and end.x
        tmp = start.x;
        start.x = end.x;
        end.x = tmp;
        // Swap start.y and end.y
        tmp = start.y;
        start.y = end.y;
        end.y = tmp;
    }

    int16_t dx, dy;
    dx = end.x - start.x;
    dy = abs(end.y - start.y);

    int16_t err = dx >> 1;
    int16_t ystep;

    if (start.y < end.y)
        ystep = 1;
    else
        ystep = -1;

    for (; start.x<=end.x; start.x++)
    {
        point_t pos = {start.y, start.x};
        if (steep)
            gfx_setPixel(pos, color);
        else
            gfx_setPixel(start, color);

        err -= dy;
        if (err < 0)
        {
            start.y += ystep;
            err += dx;
        }
    }
}

void gfx_drawRect(point_t start, uint16_t width, uint16_t height, color_t color, bool fill)
{
    if(width == 0) return;
    if(height == 0) return;
    uint16_t x_max = start.x + width - 1;
    uint16_t y_max = start.y + height - 1;
    bool perimeter = 0;
    if(x_max > (CONFIG_SCREEN_WIDTH - 1)) x_max = CONFIG_SCREEN_WIDTH - 1;
    if(y_max > (CONFIG_SCREEN_HEIGHT - 1)) y_max = CONFIG_SCREEN_HEIGHT - 1;
    for(int16_t y = start.y; y <= y_max; y++)
    {
        for(int16_t x = start.x; x <= x_max; x++)
        {
            if(y == start.y || y == y_max || x == start.x || x == x_max) perimeter = 1;
            else perimeter = 0;
            // If fill is false, draw only rectangle perimeter
            if(fill || perimeter)
            {
                point_t pos = {x, y};
                gfx_setPixel(pos, color);
            }
        }
    }
}

void gfx_drawCircle(point_t start, uint16_t r, color_t color)
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    point_t pos = start;
    pos.y += r;
    gfx_setPixel(pos, color);
    pos.y -= 2 * r;
    gfx_setPixel(pos, color);
    pos.y += r;
    pos.x += r;
    gfx_setPixel(pos, color);
    pos.x -= 2 * r;
    gfx_setPixel(pos, color);

    while (x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        pos.x = start.x + x;
        pos.y = start.y + y;
        gfx_setPixel(pos, color);
        pos.x = start.x - x;
        pos.y = start.y + y;
        gfx_setPixel(pos, color);
        pos.x = start.x + x;
        pos.y = start.y - y;
        gfx_setPixel(pos, color);
        pos.x = start.x - x;
        pos.y = start.y - y;
        gfx_setPixel(pos, color);
        pos.x = start.x + y;
        pos.y = start.y + x;
        gfx_setPixel(pos, color);
        pos.x = start.x - y;
        pos.y = start.y + x;
        gfx_setPixel(pos, color);
        pos.x = start.x + y;
        pos.y = start.y - x;
        gfx_setPixel(pos, color);
        pos.x = start.x - y;
        pos.y = start.y - x;
        gfx_setPixel(pos, color);
    }
}

void gfx_drawHLine(int16_t y, uint16_t height, color_t color)
{
    point_t start = {0, y};
    gfx_drawRect(start, CONFIG_SCREEN_WIDTH, height, color, 1);
}

void gfx_drawVLine(int16_t x, uint16_t width, color_t color)
{
    point_t start = {x, 0};
    gfx_drawRect(start, width, CONFIG_SCREEN_HEIGHT, color, 1);
}

/**
 * Compute the pixel size of the first text line
 * @param f: font used as the source of glyphs
 * @param text: the input text
 * @param length: the length of the input text, used for boundary checking
 */
static inline uint16_t get_line_size(GFXfont f, const char *text, uint16_t length)
{
    uint16_t line_size = 0;
    for(unsigned i = 0; i < length && text[i] != '\n' && text[i] != '\r'; i++)
    {
        GFXglyph glyph = f.glyph[text[i] - f.first];
        if (line_size + glyph.xAdvance < CONFIG_SCREEN_WIDTH)
            line_size += glyph.xAdvance;
        else
            break;
    }

    return line_size;
}

/**
 * Compute the start x coordinate of a new line of given pixel size
 * @param alinment: enum representing the text alignment
 * @param line_size: the size of the current text line in pixels
 */
static inline uint16_t get_reset_x(textAlign_t alignment, uint16_t line_size,
                                                          uint16_t startx)
{
    switch(alignment)
    {
        case TEXT_ALIGN_LEFT:
            return startx;
        case TEXT_ALIGN_CENTER:
            return (CONFIG_SCREEN_WIDTH - line_size)/2;
        case TEXT_ALIGN_RIGHT:
            return CONFIG_SCREEN_WIDTH - line_size - startx;
    }

    return 0;
}

uint8_t gfx_getFontHeight(fontSize_t size)
{
    GFXfont f = fonts[size];
    GFXglyph glyph = f.glyph['|' - f.first];
    return glyph.height;
}

point_t gfx_printBuffer(point_t start, fontSize_t size, textAlign_t alignment,
                        color_t color, const char *buf)
{
    GFXfont f = fonts[size];

    size_t len = strlen(buf);

    // Compute size of the first row in pixels
    uint16_t line_size = get_line_size(f, buf, len);
    uint16_t reset_x = get_reset_x(alignment, line_size, start.x);
    start.x = reset_x;
    // Save initial start.y value to calculate vertical size
    uint16_t saved_start_y = start.y;
    uint16_t line_h = 0;

    /* For each char in the string */
    for(unsigned i = 0; i < len; i++)
    {
        char c = buf[i];
        GFXglyph glyph = f.glyph[c - f.first];
        uint8_t *bitmap = f.bitmap;

        uint16_t bo = glyph.bitmapOffset;
        uint8_t w = glyph.width, h = glyph.height;
        int8_t xo = glyph.xOffset,
               yo = glyph.yOffset;
        uint8_t xx, yy, bits = 0, bit = 0;
        line_h = h;

        // Handle newline and carriage return
        if (c == '\n')
        {
          if(alignment!=TEXT_ALIGN_CENTER)
          {
            start.x = reset_x;
          }
          else
          {
            line_size = get_line_size(f, &buf[i+1], len-(i+1));
            start.x = reset_x = get_reset_x(alignment, line_size, start.x);
          }
          start.y += f.yAdvance;
          continue;
        }
        else if (c == '\r')
        {
          start.x = reset_x;
          continue;
        }

        // Handle wrap around
        if (start.x + glyph.xAdvance > CONFIG_SCREEN_WIDTH)
        {
            // Compute size of the first row in pixels
            line_size = get_line_size(f, buf, len);
            start.x = reset_x = get_reset_x(alignment, line_size, start.x);
            start.y += f.yAdvance;
        }

        // Draw bitmap
        for (yy = 0; yy < h; yy++)
        {
            for (xx = 0; xx < w; xx++)
            {
                if (!(bit++ & 7))
                {
                    bits = bitmap[bo++];
                }

                if (bits & 0x80)
                {
                    if (start.y + yo + yy < CONFIG_SCREEN_HEIGHT &&
                        start.x + xo + xx < CONFIG_SCREEN_WIDTH &&
                        start.y + yo + yy > 0 &&
                        start.x + xo + xx > 0)
                    {
                        point_t pos;
                        pos.x = start.x + xo + xx;
                        pos.y = start.y + yo + yy;
                        gfx_setPixel(pos, color);

                    }
                }

                bits <<= 1;
            }
        }

        start.x += glyph.xAdvance;
    }
    // Calculate text size
    point_t text_size = {0, 0};
    text_size.x = line_size;
    text_size.y = (saved_start_y - start.y) + line_h;
    return text_size;
}

point_t gfx_print(point_t start, fontSize_t size, textAlign_t alignment,
                  color_t color, const char *fmt, ... )
{
    // Get format string and arguments from var char
    va_list ap;
    va_start(ap, fmt);
    vsniprintf(text, sizeof(text)-1, fmt, ap);
    va_end(ap);

    return gfx_printBuffer(start, size, alignment, color, text);
}

point_t gfx_printLine(uint8_t cur, uint8_t tot, int16_t startY, int16_t endY,
                      int16_t startX, fontSize_t size, textAlign_t alignment,
                      color_t color, const char* fmt, ... )
{
    // Get format string and arguments from var char
    va_list ap;
    va_start(ap, fmt);
    vsniprintf(text, sizeof(text)-1, fmt, ap);
    va_end(ap);

    // Estimate font height by reading the gliph | height
    uint8_t fontH = gfx_getFontHeight(size);

    // If endY is 0 set it to default value = CONFIG_SCREEN_HEIGHT
    if(endY == 0) endY = CONFIG_SCREEN_HEIGHT;

    // Calculate print coordinates
    int16_t height = endY - startY;
    // to print 2 lines we need 3 padding spaces
    int16_t gap = (height - (fontH * tot)) / (tot + 1);
    // We need a gap and a line height for each line
    int16_t printY = startY + (cur * (gap + fontH));

    point_t start = {startX, printY};
    return gfx_printBuffer(start, size, alignment, color, text);
}

// Print an error message to the center of the screen, surronded by a red (when possible) box
void gfx_printError(const char *text, fontSize_t size)
{
    // 3 px box padding
    uint16_t box_padding = 16;
    color_t white = {255, 255, 255, 255};
    color_t red =   {255,   0,   0, 255};
    point_t start = {0, CONFIG_SCREEN_HEIGHT/2 + 5};

    // Print the error message
    point_t text_size = gfx_print(start, size, TEXT_ALIGN_CENTER, white, text);
    text_size.x += box_padding;
    text_size.y += box_padding;
    point_t box_start = {0, 0};
    box_start.x = (CONFIG_SCREEN_WIDTH / 2) - (text_size.x / 2);
    box_start.y = (CONFIG_SCREEN_HEIGHT / 2) - (text_size.y / 2);
    // Draw the error box
    gfx_drawRect(box_start, text_size.x, text_size.y, red, false);
}

point_t gfx_drawSymbol(point_t start, symbolSize_t size, textAlign_t alignment,
                       color_t color, symbol_t symbol)
{
    /*
     * Symbol tables come immediately after fonts in the general font table.
     * But, to prevent errors where symbol size is used instead of font size and
     * vice-versa, their enums are separate. The trickery below is used to put
     * together again the two enums in a single consecutive index.
     *
     * TODO: improve this.
     */
    int symSize = size + FONT_SIZE_NUM;
    char buffer[2] = {0};

    buffer[0] = (char) symbol;
    return gfx_printBuffer(start, symSize, alignment, color, buffer);
}

/*
 * Function to draw battery of arbitrary size
 * starting coordinates are relative to the top left point.
 *
 *  ****************       |
 * *                *      |
 * *  *******       *      |
 * *  *******       **     |
 * *  *******       **     | <-- Height (px)
 * *  *******       *      |
 * *                *      |
 *  ****************       |
 *
 * __________________
 *
 * ^
 * |
 *
 * Width (px)
 *
 */
void gfx_drawBattery(point_t start, uint16_t width, uint16_t height,
                                                    uint8_t percentage)
{
    color_t white =  {255, 255, 255, 255};
    color_t black =  {0,   0,   0  , 255};

    // Cap percentage to 1
    percentage = (percentage > 100) ? 100 : percentage;

#ifdef CONFIG_PIX_FMT_RGB565
    color_t green =  {0,   255, 0  , 255};
    color_t yellow = {250, 180, 19 , 255};
    color_t red =    {255, 0,   0  , 255};

    // Select color according to percentage
    color_t bat_color = yellow;
    if (percentage < 30)
        bat_color = red;
    else if (percentage > 60)
        bat_color = green;
#elif defined CONFIG_PIX_FMT_BW
    color_t bat_color = white;
#endif

    // Draw the battery outline
    gfx_drawRect(start, width, height, white, false);

    // Draw the battery fill
    point_t fill_start;
    fill_start.x = start.x + 2;
    fill_start.y = start.y + 2;
    int fillWidth = ((width - 4) * percentage) / 100;
    gfx_drawRect(fill_start, fillWidth, height - 4, bat_color, true);

    // Round corners
    point_t top_left     = start;
    point_t top_right    = start;
    point_t bottom_left  = start;
    point_t bottom_right = start;

    top_right.x    += width - 1;
    bottom_left.y  += height - 1;
    bottom_right.x += width - 1;
    bottom_right.y += height - 1;

    gfx_setPixel(top_left, black);
    gfx_setPixel(top_right, black);
    gfx_setPixel(bottom_left, black);
    gfx_setPixel(bottom_right, black);

    // Draw the button
    point_t button_start;
    point_t button_end;

    button_start.x = start.x + width;
    button_start.y = start.y + height / 2 - (height / 8) - 1 + (height % 2);
    button_end.x   = start.x + width;
    button_end.y   = start.y + height / 2 + (height / 8);
    gfx_drawLine(button_start, button_end, white);
}

/*
 * Function to draw RSSI-meter of arbitrary size
 * starting coordinates are relative to the top left point.
 *
 * *     *     *     *     *     *     *     *      *      *      *|
 * ***********                 <-- Volume (optional)               |
 * ***************             <-- Squelch                         |
 * ***************                                                 |
 * ******************************************                      |
 * ******************************************    <- RSSI           |
 * ******************************************                      |  <-- Height (px)
 * ******************************************                      |
 * 1     2     3     4     5     6     7     8     9     +10    +20|
 * _________________________________________________________________
 *
 * ^
 * |
 *
 * Width (px)
 *
 */
void gfx_drawSmeter(point_t start, uint16_t width, uint16_t height, rssi_t rssi,
                    uint8_t squelch, uint8_t volume, bool drawVolume, color_t color)
{
    color_t white =  {255, 255, 255, 255};
    color_t yellow = {250, 180, 19 , 255};
    color_t red =    {255, 0,   0  , 255};

    fontSize_t font = FONT_SIZE_5PT;
    uint8_t font_height =  gfx_getFontHeight(font);
    uint16_t bar_height = (height - 3 - font_height);

    uint16_t bar_height_divider = drawVolume ? 7 : 6;

    // S-level marks and numbers
    for(int i = 0; i < 12; i++)
    {
        color_t color = (i % 3 == 0) ? yellow : white;
        color = (i > 9) ? red : color;
        point_t pixel_pos = start;
        pixel_pos.x += i * (width - 1) / 11;
        gfx_setPixel(pixel_pos, color);
        pixel_pos.y += height;
        if (i == 10) {
            pixel_pos.x -= 8;
            gfx_print(pixel_pos, font, TEXT_ALIGN_LEFT, color, "+%d", i);
        }
        else if(i == 11){
            pixel_pos.x -= 10;
            gfx_print(pixel_pos, font, TEXT_ALIGN_LEFT, red, "+20");
        }
        else
            gfx_print(pixel_pos, font, TEXT_ALIGN_LEFT, color, "%d", i);
        if (i == 10) {
            pixel_pos.x += 8;
        }
    }

    uint16_t volume_height = drawVolume ? bar_height / bar_height_divider : 0;
    if (drawVolume)
    {
        // Speaker Volume Bar
        uint16_t volume_width = (width * volume) / 255;
        point_t volume_pos = {start.x, (uint8_t) (start.y + 2)};
        gfx_drawRect(volume_pos, volume_width, volume_height, white, true);
    }

    // Squelch bar
    uint16_t squelch_height = bar_height * 2 / bar_height_divider ;
    uint16_t squelch_width = (width * squelch) / 15;
    point_t squelch_pos = {start.x, (uint8_t) (start.y + 2 + volume_height)};
    gfx_drawRect(squelch_pos, squelch_width, squelch_height, color, true);

    // RSSI bar
    uint16_t rssi_height = bar_height * 4 / bar_height_divider;
    uint16_t rssi_width = (rssiToSlevel(rssi) * (width - 1) / 11);
    point_t rssi_pos = { start.x, (uint8_t) (start.y + 2 + squelch_height + volume_height)};
    gfx_drawRect(rssi_pos, rssi_width, rssi_height, white, true);
}

/*
 * Function to draw RSSI-meter with level-meter of arbitrary size
 * Version without squelch bar for digital protocols
 * starting coordinates are relative to the top left point.
 *
 * ******************************************    <- volume         |
 * *             *                *               *               *|
 * ******************************************                      |
 * ******************************************    <- level          |
 * ******************************************                      |
 * ******************************************                      |
 * *             *                *               *               *|
 * ******************************************                      |  <-- Height (px)
 * ******************************************    <- RSSI           |
 * ******************************************                      |
 * ******************************************                      |
 * 1     2     3     4     5     6     7     8     9     +10    +20|
 * _________________________________________________________________
 *
 * ^
 * |
 *
 * Width (px)
 *
 */
void gfx_drawSmeterLevel(point_t start, uint16_t width, uint16_t height, rssi_t rssi,
                         uint8_t level, uint8_t volume, bool drawVolume)
{
    color_t red =    {255, 0,   0  , 255};
    color_t green =  {0,   255,   0, 255};
    color_t white =  {255, 255, 255, 255};
    color_t yellow = {250, 180, 19 , 255};

    fontSize_t font = FONT_SIZE_5PT;
    uint8_t font_height =  gfx_getFontHeight(font);
    uint16_t bar_height = (height - 6 - font_height);
    uint16_t bar_height_divider = drawVolume ? 7 : 6;

    uint16_t volume_height = drawVolume ? bar_height / bar_height_divider : 0;
    if (drawVolume)
    {
        // Speaker Volume Bar
        uint16_t volume_width = (width * volume) / 255;
        point_t volume_pos = start;
        gfx_drawRect(volume_pos, volume_width, volume_height, white, true);
    }

    // Level meter marks
    for(int i = 0; i <= 4; i++)
    {
        point_t pixel_pos =  {start.x, (uint8_t) (start.y + volume_height)};
        pixel_pos.x += i * (width - 1) / 4;
        gfx_setPixel(pixel_pos, white);
        pixel_pos.y += ((bar_height / bar_height_divider * 3) + 3);
        gfx_setPixel(pixel_pos, white);
    }
    // Level bar
    uint16_t level_height = bar_height * 3 / bar_height_divider;
    uint16_t level_width = (width * level) / 255;
    point_t level_pos = { start.x, (uint8_t) (start.y + 2 + volume_height)};
    gfx_drawRect(level_pos, level_width, level_height, green, true);

    // RSSI bar
    uint16_t rssi_height = bar_height * 3 / bar_height_divider;
    uint16_t rssi_width = (rssiToSlevel(rssi) * (width - 1) / 11);
    point_t rssi_pos = {start.x, (uint8_t) (start.y + 5 + level_height + volume_height)};
    gfx_drawRect(rssi_pos, rssi_width, rssi_height, white, true);
    // S-level marks and numbers
    for(int i = 0; i < 12; i++)
    {
        color_t color = (i % 3 == 0) ? yellow : white;
        color = (i > 9) ? red : color;
        point_t pixel_pos = start;
        pixel_pos.x += i * (width - 1) / 11;
        pixel_pos.y += height;

        if (i == 10) {
            pixel_pos.x -= 8;
            gfx_print(pixel_pos, font, TEXT_ALIGN_LEFT, color, "+%d", i);
        }
        else if(i == 11){
            pixel_pos.x -= 10;
            gfx_print(pixel_pos, font, TEXT_ALIGN_LEFT, red, "+20");
        }
        else
            gfx_print(pixel_pos, font, TEXT_ALIGN_LEFT, color, "%d", i);
        if (i == 10) {
            pixel_pos.x += 8;
        }
    }
}

/*
 * Function to draw GPS satellites snr bar graph of arbitrary size
 * starting coordinates are relative to the top left point.
 *
 * ****            |
 * ****      ****  |
 * **** **** ****  |  <-- Height (px)
 * **** **** ****  |
 * **** **** ****  |
 * **** **** ****  |
 *                 |
 *  N    N+1  N+2  |
 * __________________
 *
 * ^
 * |
 *
 * Width (px)
 *
 */
void gfx_drawGPSgraph(point_t start,
                      uint16_t width,
                      uint16_t height,
                      gpssat_t *sats,
                      uint32_t active_sats)
{
    color_t white =  {255, 255, 255, 255};
    color_t yellow = {250, 180, 19 , 255};

    // SNR Bars and satellite identifiers
    uint8_t bar_width = (width - 26) / 12;
    uint8_t bar_height = 1;
    for(int i = 0; i < 12; i++)
    {
        bar_height = (height - 8) * sats[i].snr / 100 + 1;
        point_t bar_pos = start;
        bar_pos.x += 2 + i * (bar_width + 2);
        bar_pos.y += (height - 8) - bar_height;
        color_t bar_color = (active_sats & 1 << (sats[i].id - 1)) ? yellow : white;
        gfx_drawRect(bar_pos, bar_width, bar_height, bar_color, true);
        point_t id_pos = {bar_pos.x, (uint8_t) (start.y + height)};
        gfx_print(id_pos, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
                  bar_color, "%2d ", sats[i].id);
    }
    uint8_t bars_width = 9 + 11 * (bar_width + 2);
    point_t left_line_end    = start;
    point_t right_line_start = start;
    point_t right_line_end   = start;

    left_line_end.y    += height - 9;
    right_line_start.x += bars_width;
    right_line_end.x   += bars_width;
    right_line_end.y   += height - 9;

    gfx_drawLine(start, left_line_end, white);
    gfx_drawLine(right_line_start, right_line_end, white);
}

void gfx_drawGPScompass(point_t start,
                        uint16_t radius,
                        float deg,
                        bool active)
{
    color_t white =  {255, 255, 255, 255};
    color_t black =  {  0,   0,   0, 255};
    color_t yellow = {250, 180, 19 , 255};

    // Compass circle
    point_t circle_pos = start;
    circle_pos.x += radius + 1;
    circle_pos.y += radius + 3;
    gfx_drawCircle(circle_pos, radius, white);
    point_t n_box = {(uint8_t)(start.x + radius - 5), start.y};
    gfx_drawRect(n_box, 13, 13, black, true);
    float needle_radius = radius - 4;
    if (active)
    {
        // Needle
        deg -= 90.0f;
        point_t p1 = {(uint8_t)(circle_pos.x + needle_radius * COS(deg)),
                      (uint8_t)(circle_pos.y + needle_radius * SIN(deg))};
        point_t p2 = {(uint8_t)(circle_pos.x + needle_radius * COS(deg + 145.0f)),
                      (uint8_t)(circle_pos.y + needle_radius * SIN(deg + 145.0f))};
        point_t p3 = {(uint8_t)(circle_pos.x + needle_radius / 2 * COS(deg + 180.0f)),
                      (uint8_t)(circle_pos.y + needle_radius / 2 * SIN(deg + 180.0f))};
        point_t p4 = {(uint8_t)(circle_pos.x + needle_radius * COS(deg - 145.0f)),
                      (uint8_t)(circle_pos.y + needle_radius * SIN(deg - 145.0f))};
        gfx_drawLine(p1, p2, yellow);
        gfx_drawLine(p2, p3, yellow);
        gfx_drawLine(p3, p4, yellow);
        gfx_drawLine(p4, p1, yellow);
    }
    // North indicator
    point_t n_pos = {(uint8_t)(start.x + radius - 3),
                     (uint8_t)(start.y + 7)};
    gfx_print(n_pos, FONT_SIZE_6PT, TEXT_ALIGN_LEFT, white, "N");
}

void gfx_plotData(point_t start, uint16_t width, uint16_t height,
                  const int16_t *data, size_t len)
{
    uint16_t horizontal_pos = start.x;
    color_t white = {255, 255, 255, 255};
    point_t prev_pos = {0, 0};
    point_t pos = {0, 0};
    bool first_iteration = true;
    for (size_t i = 0; i < len; i++)
    {
        horizontal_pos++;
        if (horizontal_pos > (start.x + width))
            break;
        pos.x = horizontal_pos;
        pos.y = start.y + (height / 2)
              + ((data[i] * 4) / (2 * SHRT_MAX) * height);
        if (pos.y > CONFIG_SCREEN_HEIGHT)
            pos.y = CONFIG_SCREEN_HEIGHT;
        if (!first_iteration)
            gfx_drawLine(prev_pos, pos, white);
        prev_pos = pos;
        if (first_iteration)
            first_iteration = false;
    }
}
