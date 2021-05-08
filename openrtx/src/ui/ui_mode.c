/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#include <stdio.h>
#include <stdint.h>
#include <ui.h>
#include <string.h>

/* UI main screen helper functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainTop();
extern void _ui_drawMainBottom();

void _ui_drawModeVFOMiddle()
{
    // Print VFO RX Frequency on line 1 of 4
    gfx_printLine(1, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, layout.horizontal_pad, layout.line1_font, 
                  TEXT_ALIGN_CENTER, color_white, "%03lu.%05lu",
                  (unsigned long)last_state.channel.rx_frequency/1000000,
                  (unsigned long)last_state.channel.rx_frequency%1000000/10);
    // Print Module / Talkgroup on line 2 of 4
    gfx_printLine(2, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, layout.horizontal_pad, layout.line1_font, 
                  TEXT_ALIGN_LEFT, color_white, "mo:");
    // Print User ID on line 3 of 4
    gfx_printLine(3, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, layout.horizontal_pad, layout.line1_font, 
                  TEXT_ALIGN_LEFT, color_white, "id:");
}

void _ui_drawModeMEMMiddle()
{
    // Print Channel name on line 1 of 4
    gfx_printLine(1, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, layout.horizontal_pad, layout.line1_font, 
                  TEXT_ALIGN_CENTER, color_white, "%03d: %.12s", 
                  last_state.channel_index, last_state.channel.name);
    // Print Module Frequency on line 2 of 4
    gfx_printLine(2, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, layout.horizontal_pad, layout.line1_font, 
                  TEXT_ALIGN_LEFT, color_white, "mo:");
    // Print User ID on line 3 of 4
    gfx_printLine(3, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, layout.horizontal_pad, layout.line1_font, 
                  TEXT_ALIGN_LEFT, color_white, "id:");
}

void _ui_drawModeVFO()
{
    gfx_clearScreen();
    _ui_drawMainTop();
    _ui_drawModeVFOMiddle();
    _ui_drawMainBottom();
}

void _ui_drawModeMEM()
{
    gfx_clearScreen();
    _ui_drawMainTop();
    _ui_drawModeMEMMiddle();
    _ui_drawMainBottom();
}
