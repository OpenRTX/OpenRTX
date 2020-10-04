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
 * This function calls the correspondent method of the low level interface display.h
 * Get pointer to framebuffer. Being this a standard interface for all the
 * low-level display drivers, this function returns a pointer to void: it's up
 * to the caller performing the correct cast to one of the standard types used
 * for color coding.
 * Changes to the framebuffer will not be reflected on the display until
 * graphics_render() or graphics_renderRows() are called.
 *
 * 
 * WARNING: no bound check is performed! Do not call free() on the pointer
 * returned, doing so will destroy the framebuffer!
 * @return pointer to framebuffer.
 */
void *graphics_getFrameBuffer();

#endif /* GRAPHICS_H */
