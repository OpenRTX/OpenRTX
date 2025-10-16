/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
