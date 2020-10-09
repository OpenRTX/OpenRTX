/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
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

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>
#include <display.h>

/**
 * Standard high-level graphic interface for all display types.
 * This interface is based on the lower level interface display.h
 * Many methods of graphics.h are basically calls to methods in display.h.
 *
 * On top of basic framebuffer and backlight control, graphics.h implements
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
typedef struct point_t
{
    uint16_t x;
    uint16_t y;
} point_t;

/**
 * Structure that represents a single color in the RGB 8 bit per channel format
 */
typedef struct color_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

/**
 * This function calls the correspondent method of the low level interface display.h
 * It initializes the display and sets the backlight to zero.
 */
void graphics_init();

/**
 * This function calls the correspondent method of the low level interface display.h
 * It turns off backlight, shuts down backlight control and deallocates the framebuffer.
 */
void graphics_terminate();

/**
 * This function calls the correspondent method of the low level interface display.h
 * Get screen width in pixels.
 * @return screen width, in pixels.
 */
uint16_t graphics_screenWidth();

/**
 * This function calls the correspondent method of the low level interface display.h
 * Get screen height in pixels.
 * @return screen height, in pixels.
 */
uint16_t graphics_screenHeight();

/**
 * This function calls the correspondent method of the low level interface display.h
 * Set screen backlight to a given level.
 * @param level: backlight level, from 0 (backlight off) to 255 (backlight at
 * full brightness).
 */
void graphics_setBacklightLevel(uint8_t level);

/**
 * This function calls the correspondent method of the low level interface display.h
 * Copy a given section, between two given rows, of framebuffer content to the
 * display.
 * @param startRow: first row of the framebuffer section to be copied
 * @param endRow: last row of the framebuffer section to be copied
 */
void graphics_renderRows(uint8_t startRow, uint8_t endRow);

/**
 * This function calls the correspondent method of the low level interface display.h
 * Copy framebuffer content to the display internal buffer. To be called
 * whenever there is need to update the display.
 */
void graphics_render();

/**
 * This function calls the correspondent method of the low level interface display.h
 * Check if framebuffer is being copied to the screen or not, in which case it
 * can be modified without problems.
 * @return false if rendering is not in progress.
 */
bool graphics_renderingInProgress();

/**
 * Makes the screen black.
 */
void graphics_clearScreen();

/**
 * Fills screen with the specified color.
 * @param color: border and fill color, in color_t format.
 */
void graphics_fillScreen(color_t color);

/**
 * Draw a line from start to end coordinates, ends included.
 * @param start: line start point, in pixel coordinates.
 * @param end: line end point, in pixel coordinates.
 * @param color: line color, in color_t format.
 */
void graphics_drawLine(point_t start, point_t end, color_t color);

/**
 * Draw a rectangle of specified width, height and color.
 * @param width: rectangle width, in pixels, borders included.
 * @param height: rectangle height, in pixels, borders included.
 * @param color: border and fill color, in color_t format.
 * @param fill: if true the rectangle will be solid, otherwise it will be empty with a 1-pixel border
 */
void graphics_drawRect(point_t start, uint16_t width, uint16_t height, color_t color, bool fill);

#endif /* GRAPHICS_H */
