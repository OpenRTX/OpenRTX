/***************************************************************************
 *   Copyright (C)        2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                and the OpenRTX contributors             *
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
 *                                                                         *
 ***************************************************************************/

// This file exists to complement M17Constants.hpp with C-compatible constants

#ifndef M17_C_CONSTANTS_H
#define M17_C_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

#define M17_META_TEXT_BLOCK_SIZE 13
#define M17_META_TEXT_DATA_MAX_LENGTH M17_META_TEXT_BLOCK_SIZE * 4
#define M17_META_TEXT_PADDING_CHAR ' '
#define M17_META_GNSS_BLOCK_SIZE 14

#ifdef __cplusplus
}
#endif

#endif /* M17_C_CONSTANTS_H */