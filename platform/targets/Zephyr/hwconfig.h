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

#ifndef HWCONFIG_H
#define HWCONFIG_H

/*
 * This header file is generic for all the Zephyr-based platforms,
 * further specialization is performed in the devicetree.
 */

/*
 * In Zephyr platforms display properties are encoded in the devicetree
 */
#if defined(PLATFORM_ZEPHYR)
#include <zephyr/device.h>
#define DISPLAY DT_CHOSEN(zephyr_display)
#define SCREEN_WIDTH DT_PROP(DISPLAY, width)
#define SCREEN_HEIGHT DT_PROP(DISPLAY, height)
#endif

/*
 * Pixel format is computed at compile-time
 */
#if defined(CONFIG_SSD1306)
#define PIX_FMT_BW
#endif

// TODO: add battery type in devicetree
#define BAT_NONE

#endif /* HWCONFIG_H */
