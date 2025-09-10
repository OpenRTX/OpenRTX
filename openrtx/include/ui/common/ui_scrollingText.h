/***************************************************************************
 *   Copyright (C) 2025        by Federico Amedeo Izzo IU2NUO,             *
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
 ***************************************************************************/

#ifndef UI_SCROLLINGTEXT_H
#define UI_SCROLLINGTEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <graphics.h>

/**
 * Print a string that scrolls if it is too long to fit in the available space.
 * @param position Screen position for text rendering
 * @param fontSize Font size to use for rendering
 * @param state Pointer to ui_scrolling_text_state_t structure for maintaining state
 * @param text Main text to display (can be NULL to clear)
 * @param maxWidth  Maximum display width in characters
 * @return true if operation was successful, false otherwise
 */
bool ui_printScrollingBuffer(point_t start, fontSize_t size,
                             textAlign_t alignment, color_t color,
                             const char *buf, size_t *scrollPosition,
                             uint8_t maxWidth, long long *lastScrollTime);

#endif /* UI_SCROLLINGTEXT_H */