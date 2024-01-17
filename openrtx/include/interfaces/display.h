/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Silvano Seva IU2KWO                      *
 *                            and Niccolò Izzo IU2KIN                      *
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

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Standard interface for all low-level display drivers.
 *
 *********************** HOW TO MANAGE FRAMEBUFFER *****************************
 *
 * This driver allocates the framebuffer as a block of linearly addressed memory
 * equivalent to an array of CONFIG_SCREEN_HEIGHT*CONFIG_SCREEN_WIDTH elements.
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
 * to be indexed in this way: buf[X + Y*CONFIG_SCREEN_WIDTH].
 *
 */

/**
 * This function initialises the display and allocates framebuffer on the heap.
 *
 * NOTE: framebuffer allocation is the first operation performed, if fails an
 * error message is printed on the virtual COM port and this function returns
 * prematurely, without configuring the display.
 * Thus, a dark screen can be symptom of failed allocation.
 */
void display_init();

/**
 * When called, this function terminates the display driver
 * and deallocates the framebuffer.
 */
void display_terminate();

/**
 * Copy a given section, between two given rows, of framebuffer content to the
 * display. This function blocks the caller until render is completed.
 *
 * @param startRow: first row of the framebuffer section to be copied
 * @param endRow: last row of the framebuffer section to be copied
 * @param fb: pointer to frameBuffer.
 */
void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb);

/**
 * Copy framebuffer content to the display internal buffer, to be called
 * whenever there is need to update the display.
 * This function blocks the caller until render is completed.
 *
 * @param fb: pointer to framebuffer.
 */
void display_render(void *fb);

/**
 * Set display contrast.
 * NOTE: not all the display controllers support contrast control, thus on some
 * targets this function has no effect.
 * @param contrast: display contrast, normalised value with range 0 - 255.
 */
void display_setContrast(uint8_t contrast);

/**
 * Set level of display backlight.
 * NOTE: not all the display controllers support backlight control, thus on some
 * targets this function has no effect.
 * @param level: display backlight level, normalised value with range 0 - 100.
 */
void display_setBacklightLevel(uint8_t level);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */
