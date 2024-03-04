/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Silvano Seva IU2KWO                      *
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

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <symbols.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <gps.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Standard high-level graphic interface for all display types.
 * This interface is based on the lower level interface display.h
 * Many methods of graphics.h are basically calls to methods in display.h.
 *
 * On top of basic framebuffer control, graphics.h implements
 * graphics primitives for drawing fonts and shapes
 *
 *********************** HOW TO MANAGE FRAMEBUFFER *****************************
 *
 * This driver allocates the framebuffer as a block of linearly addressed memory
 * equivalent to an array of SCREEN_HEIGHT*SCREEN_WIDTH elements.
 * With respect to it, screen is indexed in this way:
 *
 *   (0,0)
 *     +-------> x
 *     |
 *     |    o (X,Y)
 *     |
 *     v
 *     y
 *
 * then to set the value of the pixel having coordinates (X,Y), framebuffer has
 * to be indexed in this way: buf[X + Y*SCREEN_WIDTH].
 *
 */

/**
 * Structure that represents the X,Y coordinates of a single point
 */
typedef int16_t Pos_t ;

typedef struct Pos_st
{
    Pos_t x ;
    Pos_t y ;
}Pos_st;

/**
 * Structure that represents a single color in the RGB 8 bit per channel format
 */
typedef struct Color_st
{
    uint8_t red ;
    uint8_t green ;
    uint8_t blue ;
    uint8_t alpha ;
}Color_st;

typedef enum
{
    FONT_SIZE_5PT  ,
    FONT_SIZE_6PT  ,
    FONT_SIZE_8PT  ,
    FONT_SIZE_9PT  ,
    FONT_SIZE_10PT ,
    FONT_SIZE_12PT ,
    FONT_SIZE_16PT ,
    FONT_SIZE_18PT ,
    FONT_SIZE_24PT
}FontSize_t;

typedef enum
{
    SYMBOLS_SIZE_5PT ,
    SYMBOLS_SIZE_6PT ,
    SYMBOLS_SIZE_8PT
}SymbolSize_t;

typedef enum
{
    ALIGN_LEFT   ,
    ALIGN_CENTER ,
    ALIGN_RIGHT
}Align_t;

/**
 * This function calls the correspondent method of the low level interface display.h
 * It initializes the display.
 */
void gfx_init();

/**
 * This function calls the correspondent method of the low level interface display.h
 * It terminates the display driver and deallocates the framebuffer.
 */
void gfx_terminate();

/**
 * This function calls the correspondent method of the low level interface display.h
 * Copy a given section, between two given rows, of framebuffer content to the
 * display.
 * @param startRow: first row of the framebuffer section to be copied
 * @param endRow: last row of the framebuffer section to be copied
 */
void gfx_renderRows(uint8_t startRow, uint8_t endRow);

/**
 * This function calls the correspondent method of the low level interface display.h
 * Copy framebuffer content to the display internal buffer. To be called
 * whenever there is need to update the display.
 */
void gfx_render();

/**
 * This function calls the correspondent method of the low level interface display.h
 * Check if framebuffer is being copied to the screen or not, in which case it
 * can be modified without problems.
 * @return false if rendering is not in progress.
 */
bool gfx_renderingInProgress();

/**
 * Clears a portion of the screen content
 * This results in a black screen on color displays
 * And a white screen on B/W displays
 * @param startRow: first row of the framebuffer section to be cleared
 * @param endRow: last row of the framebuffer section to be cleared
 */
void gfx_clearRows(uint8_t startRow, uint8_t endRow);

/**
 * Clears the content of the screen
 * This results in a black screen on color displays
 * And a white screen on B/W displays
 */
void gfx_clearScreen();

/**
 * Fills screen with the specified color.
 * @param color: border and fill color, in Color_st format.
 */
void gfx_fillScreen(Color_st color);

/**
 * Clear the color of a single pixel.
 * @param pos: x,y coordinates of the pixel.
 */
void gfx_clearPixel( Pos_st pos );

/**
 * Change the color of a single pixel.
 * @param pos: x,y coordinates of the pixel.
 * @param color: desired pixel color, in Color_st format.
 */
void gfx_setPixel(Pos_st pos, Color_st color);

/**
 * Draw a line from start to end coordinates, ends included.
 * @param start: line start point, in pixel coordinates.
 * @param end: line end point, in pixel coordinates.
 * @param color: line color, in Color_st format.
 */
void gfx_drawLine(Pos_st start, Pos_st end, Color_st color);


/**
 * Draw a horizontal line with specified vertical position and width.
 * @param y: vertical position, in pixel coordinates.
 * @param height: line height, in pixel coordinates.
 * @param color: line color, in Color_st format.
 */
void gfx_drawHLine(int16_t y, uint16_t height, Color_st color);

/**
 * Draw a vertical line with specified horizontal position and width.
 * @param x: horizontal position, in pixel coordinates.
 * @param width: line width, in pixel coordinates.
 * @param color: line color, in Color_st format.
 */
void gfx_drawVLine(int16_t x, uint16_t width, Color_st color);

/**
 * Clear a rectangle of specified width, height and color.
 * @param start: screen position of the rectangle, in pixels
 * @param width: rectangle width, in pixels, borders included.
 * @param height: rectangle height, in pixels, borders included.
 */
void gfx_clearRectangle( Pos_st start, uint16_t width, uint16_t height );

/**
 * Draw a rectangle of specified width, height and color.
 * @param start: screen position of the rectangle, in pixels
 * @param width: rectangle width, in pixels, borders included.
 * @param height: rectangle height, in pixels, borders included.
 * @param color: border and fill color, in Color_st format.
 * @param fill: if true the rectangle will be solid, otherwise it will be empty with a 1-pixel border
 */
void gfx_drawRect(Pos_st start, uint16_t width, uint16_t height, Color_st color,
                  bool fill);

/**
 * Draw the outline of a circle of specified radius and color.
 * @param start: screen position of the center of the circle, in pixels
 * @param r: circle radius, in pixels, border included.
 * @param color: border color, in Color_st format.
 */
void gfx_drawCircle(Pos_st start, uint16_t r, Color_st color);

/**
 * Estimates the maximum font height by reading the gliph | height
 * @param size: text font size, defined as enum.
 * @return font height
 */
uint8_t gfx_getFontHeight(FontSize_t size);

/**
 * Prints text on the screen at the specified coordinates.
 * Reads text from a given char buffer
 * @param start: text line start point, in pixel coordinates.
 * @param size: text font size, defined as enum.
 * @param alignment: text alignment type, defined as enum.
 * @param color: text color, in Color_st format.
 * @param buf: char buffer
 * @return text width and height as Pos_st coordinates
 */
Pos_st gfx_printBuffer(Pos_st start, FontSize_t size, Align_t alignment,
                        Color_st color, const char *buf);

/**
 * Prints text on the screen at the specified coordinates.
 * @param start: text line start point, in pixel coordinates.
 * @param size: text font size, defined as enum.
 * @param alignment: text alignment type, defined as enum.
 * @param color: text color, in Color_st format.
 * @param fmt: printf style format string
 * @return text width and height as Pos_st coordinates
 */
Pos_st gfx_print(Pos_st start, FontSize_t size, Align_t alignment,
                  Color_st color, const char* fmt, ... );

/**
 * Prints text on the screen, calculating the print position.
 * The print position is calculated to fit the desired number of lines in the vertical space
 * @param cur: current line number over total (1-based)
 * @param tot: number of lines to fit in screen
 * @param startY: starting Y coordinate to leave space at the top, use 0 to leave no space
 * @param endY: ending Y coordinate to leave space at the bottom, use 0 to leave no space
 * @param startX: starting X coordinate to leave space on the screen sides
 * @param size: text font size, defined as enum.
 * @param alignment: text alignment type, defined as enum.
 * @param color: text color, in Color_st format.
 * @param fmt: printf style format string
 * @return text width and height as Pos_st coordinates
 */
Pos_st gfx_printLine(uint8_t cur, uint8_t tot, int16_t startY, int16_t endY,
                      int16_t startX, FontSize_t size, Align_t alignment,
                      Color_st color, const char* fmt, ... );

/**
 * Prints an error message surrounded by a red box on the screen.
 * @param text: text to print.
 * @param size: text font size, defined as enum.
 */
void gfx_printError(const char *text, FontSize_t size);

/**
 * Prints text on the screen at the specified coordinates.
 * @param start: text line start point, in pixel coordinates.
 * @param size: icon font size, defined as enum.
 * @param alignment: text alignment type, defined as enum. DEPRECATED: in the
 *                   future this will be always LEFT.
 * @param color: text color, in Color_st format.
 * @param symbol: symbol to be printed.
 * @return text width and height as Pos_st coordinates
 */
Pos_st gfx_drawSymbol(Pos_st start, SymbolSize_t size, Align_t alignment,
                       Color_st color, symbol_t symbol);

/**
 * Function to draw battery of arbitrary size.
 * Starting coordinates are relative to the top left point.
 * @param start: battery icon start point, in pixel coordinates.
 * @param width: battery icon width
 * @param height: battery icon height
 * @param percentage: battery charge percentage
 */
void gfx_drawBattery(Pos_st start, uint16_t width, uint16_t height,
                     uint16_t percentage);

/**
 * Function to draw Smeter of arbitrary size.
 * Starting coordinates are relative to the top left point.
 * @param start: Smeter start point, in pixel coordinates.
 * @param width: Smeter width
 * @param height: Smeter height
 * @param rssi: rssi level in dBm
 * @param squelch: squelch level in percentage
 * @param color: color of the squelch bar
 */
void gfx_drawSmeter(Pos_st start, uint16_t width, uint16_t height, float rssi,
                    float squelch, Color_st color);

/**
 * Function to draw Smeter + level meter of arbitrary size.
 * Version without squelch bar for digital protocols
 * Starting coordinates are relative to the top left point.
 * @param start: Smeter start point, in pixel coordinates.
 * @param width: Smeter width
 * @param height: Smeter height
 * @param rssi: rssi level in dBm
 * @param level: level in range {0, 255}
 */
void gfx_drawSmeterLevel(Pos_st start, uint16_t width, uint16_t height,
                         float rssi, uint8_t level);

/**
 * Function to draw GPS SNR bar graph of arbitrary size.
 * Starting coordinates are relative to the top left point.
 * @param start: Bar graph start point, in pixel coordinates.
 * @param width: Bar graph width
 * @param height: Bar graph height
 * @param sats: pointer to the array of satellites data
 * @param active_sats: bitset representing which sats are part of the fix
 */
void gfx_drawGPSgraph(Pos_st start, uint16_t width, uint16_t height,
                      gpssat_t *sats, uint32_t active_sats);

/**
 * Function to draw a compass of arbitrary size.
 * Starting coordinates are relative to the top left point.
 * @param start: Compass start point, in pixel coordinates.
 * @param width: Compass radius
 * @param deg: degrees marked by the compass needle
 * @param active: whether the needle is to be drawn or not
 */
void gfx_drawGPScompass(Pos_st start, uint16_t radius, float deg, bool active);

/**
 * Function to plot a collection of data on the screen.
 * Starting coordinates are relative to the top left point.
 * @param start: Plot start point, in pixel coordinates.
 * @param width: Plot width
 * @param height: Plot height
 * @param data: pointer to data buffer
 * @param len: data length, in elements
 */
void gfx_plotData(Pos_st start, uint16_t width, uint16_t height,
                  const int16_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* GRAPHICS_H */
