/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SH110X_MOD17_H
#define SH110X_MOD17_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the SH110x display driver.
 */
void SH110x_init();

/**
 * Shut down the SH110x display driver.
 */
void SH110x_terminate();

/**
 * Do a partial framebuffer render.
 *
 * @param startRow: first row of the partial render.
 * @param endRow: last row of the partial render.
 * @param fb: pointer to framebuffer.
 */
void SH110x_renderRows(uint8_t startRow, uint8_t endRow, void *fb);

/**
 * Render the framebuffer on the screen.
 *
 * @param fb: pointer to framebuffer.
 */
void SH110x_render(void *fb);

/**
 * Set display contrast.
 *
 * @param contrast: display contrast level, 0 to 63.
 */
void SH110x_setContrast(uint8_t contrast);

#ifdef __cplusplus
}
#endif

#endif /* SH110X_MOD17_H */
