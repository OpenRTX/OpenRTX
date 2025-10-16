/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Low-level driver for backlight dimming control.
 * This header file only provides the API for driver initialisation and shutdown,
 * while effective setting of backlight level is provided by target-specific
 * sources by implementating display_setBacklightLevel().
 */

/**
 * Initialise backlight driver.
 */
void backlight_init();

/**
 * Terminate backlight driver.
 */
void backlight_terminate();


#ifdef __cplusplus
}
#endif

#endif /* BACKLIGHT_H */
