/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
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

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include <gd32f30x.h>
#include "pinmap.h"

//extern const struct spiCustomDevice nvm_spi;
extern const struct spiDevice nvm_spi0;
extern const struct spiDevice st7735s_spi1;

// Screen dimensions and pixel format
#define CONFIG_SCREEN_WIDTH  132
#define CONFIG_SCREEN_HEIGHT 128

// Battery type
#define CONFIG_BAT_LIPO_2S

#define CONFIG_PIX_FMT_RGB565
#define CONFIG_GFX_NOFRAMEBUF

#define CONFIG_SCREEN_BRIGHTNESS

#endif