/***************************************************************************
 *   Copyright (C) 2024 Silvano Seva IU2KWO                                *
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

#include <stm32g4xx.h>
#include "pinmap.h"

/* Screen dimensions */
#define CONFIG_SCREEN_WIDTH 128
#define CONFIG_SCREEN_HEIGHT 64

/* Screen pixel format */
#define CONFIG_PIX_FMT_BW

/* Device has no battery */
#define CONFIG_BAT_NONE

/* Device supports M17 mode */
#define CONFIG_M17

#endif
