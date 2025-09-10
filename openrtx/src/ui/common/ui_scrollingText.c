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

#include <ui/common/ui_scrollingText.h>
#include <interfaces/delays.h>
#include <string.h>

// Update interval for scrolling animation in milliseconds
#define SCROLL_INTERVAL_MS 150

bool ui_printScrollingBuffer(point_t start, fontSize_t size,
                             textAlign_t alignment, color_t color,
                             const char *buf, size_t *scrollPosition,
                             uint8_t maxWidth, long long *lastScrollTime)
{
    if (buf == NULL || lastScrollTime == NULL) {
        return false;
    }

    const size_t bufLen = strlen(buf);

    // Exit early if we don't actually need to scroll
    const bool needsScrolling = bufLen > maxWidth;
    if (!needsScrolling) {
        // Just display the buf directly if no scrolling is needed
        gfx_print(start, size, alignment, color, "%s", buf);
        return true;
    }
    long long currentTime = getTick();

    if ((currentTime - *lastScrollTime) >= SCROLL_INTERVAL_MS) {
        *lastScrollTime = currentTime;

        // Add a delay when reaching the end of text soas to make it more noticable
        if (*scrollPosition == bufLen - 1) {
            *lastScrollTime += SCROLL_INTERVAL_MS * 3;
        }

        *scrollPosition = (*scrollPosition + 1) % bufLen;
    }

    // Create a temporary buffer to hold the visible part of the string
    char displayBuffer[maxWidth + 1];

    for (int i = 0; i < maxWidth; i++) {
        displayBuffer[i] = buf[(*scrollPosition + i) % bufLen];
    }
    displayBuffer[maxWidth] = '\0';

    // Display the buf
    gfx_print(start, size, alignment, color, "%s", displayBuffer);

    return true;
}
