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

#ifndef UI_METADATA_H
#define UI_METADATA_H

#include <stdbool.h>
#include <stdint.h>
#include <graphics.h>
#include <M17/m17_constants.h>

/**
 * Metadata display component state structure.
 * Encapsulates all state needed for displaying and scrolling M17 metadata text.
 */
typedef struct {
    char originalText[M17_META_TEXT_DATA_MAX_LENGTH +
                      1];     // Text that is the original, unscrolled text
    size_t scrollPosition;
    long long lastScrollTime; // Timestamp of last scroll update
} ui_metadata_t;

/**
 * Update and render metadata display in one step.
 * Handles scrolling automatically if text is too long.
 * 
 * @param position Screen position for text rendering
 * @param fontSize Font size to use for rendering
 * @param metadata Pointer to metadata structure
 * @param text Main text to display (can be NULL to clear)
 * @return true if operation was successful, false otherwise
 */
bool ui_drawMetadata(point_t position, fontSize_t fontSize,
                     ui_metadata_t *metadata, const char *text);

/**
 * Clear metadata and reset state.
 * 
 * @param metadata Pointer to metadata structure
 */
void ui_metadataClear(ui_metadata_t *metadata);

#endif /* UI_METADATA_H */
