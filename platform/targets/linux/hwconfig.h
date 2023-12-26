/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Frederik Saraci IU2NRO                   *
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

#ifdef __cplusplus
extern "C" {
#endif

/* Screen has adjustable brightness */
#define CONFIG_SCREEN_BRIGHTNESS

/* Battery type */
#define CONFIG_BAT_LIPO_2S

/* Device supports M17 mode */
#define CONFIG_M17

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
