/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                Mathis Schmieder DB9MAT                  *
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

#ifndef SSD1306_MOD17_H
#define SSD1306_MOD17_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the SSD1306 display driver.
 */
void SSD1306_init();

/**
 * Shut down the SSD1306 display driver.
 */
void SSD1306_terminate();

/**
 * Do a partial framebuffer render.
 *
 * @param startRow: first row of the partial render.
 * @param endRow: last row of the partial render.
 * @param fb: pointer to framebuffer.
 */
void SSD1306_renderRows(uint8_t startRow, uint8_t endRow, void *fb);

/**
 * Render the framebuffer on the screen.
 *
 * @param fb: pointer to framebuffer.
 */
void SSD1306_render(void *fb);

/**
 * Set display contrast.
 *
 * @param contrast: display contrast level, 0 to 63.
 */
void SSD1306_setContrast(uint8_t contrast);

#ifdef __cplusplus
}
#endif

#endif /* SSD1306_MOD17_H */
