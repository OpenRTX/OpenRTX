/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
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

#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Scroll a string to the left by one character, moving the first character to the end.
 * When reset is true, it adds a space at the end of the string for a gap between scrolling.
 *
 * @param string The string to scroll
 * @param reset If true, adds a space at the end and resets the scroll position
 * @param max_len The maximum length of the buffer to prevent overflow
 * @return true if operation succeeded, false otherwise
 */
bool ui_scrollString(char *string, bool reset, size_t max_len);

#endif /* UI_UTILS_H */