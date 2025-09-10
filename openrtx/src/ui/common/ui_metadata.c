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

#include <stdio.h>
#include <string.h>
#include <interfaces/delays.h>
#include <ui/common/ui_metadata.h>
#include <ui/common/ui_scrollingText.h>
#include <graphics.h>
#include <hwconfig.h>

// Update interval for scrolling animation in milliseconds
#define SCROLL_INTERVAL_MS 150

extern const color_t color_white;

void ui_metadataClear(ui_metadata_t *metadata)
{
    if (metadata == NULL) {
        return;
    }

    memset(metadata->originalText, 0, sizeof(metadata->originalText));
    metadata->scrollPosition = 0;
    metadata->lastScrollTime = 0;
}

bool ui_drawMetadata(point_t position, fontSize_t fontSize,
                     ui_metadata_t *metadata, const char *text)
{
    if (metadata == NULL) {
        return false;
    }

    if (text == NULL || text[0] == '\0') {
        return true;
    }

    long long currentTime = getTick();

    const uint8_t textLength = strlen(text);
    bool textChanged = (textLength != strlen(metadata->originalText));
    if (!textChanged && textLength > 0) {
        textChanged = (strcmp(metadata->originalText, text) != 0);
    }

    if (textChanged) {
        strncpy(metadata->originalText, text,
                sizeof(metadata->originalText) - 1);
        metadata->originalText[sizeof(metadata->originalText) - 1] = '\0';
        metadata->scrollPosition = 0;
        metadata->lastScrollTime = currentTime;
    }

    ui_printScrollingBuffer(position, fontSize, TEXT_ALIGN_CENTER, color_white,
                            text, &metadata->scrollPosition,
                            M17_META_TEXT_MAX_SCREEN_WIDTH,
                            &metadata->lastScrollTime);
    return true;
}
