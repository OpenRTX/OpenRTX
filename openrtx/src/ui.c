/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
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

/*
 * The graphical user interface (GUI) works by splitting the screen in 
 * horizontal rows, with row height depending on vertical resolution.
 * 
 * The general screen layout is composed by an upper status bar at the
 * top of the screen and a lower status bar at the bottom.
 * The central portion of the screen is filled by two big text/number rows
 * And a small row.
 *
 * Below is shown the row height for two common display densities.
 *
 *        160x128 display (MD380)            Recommended font size
 *      ┌─────────────────────────┐
 *      │  top_status_bar (16 px) │  8 pt font with 4 px vertical padding
 *      ├─────────────────────────┤
 *      │      Line 1 (32px)      │  16 pt font with 8 px vertical padding
 *      │                         │
 *      │      Line 2 (32px)      │  16 pt font with 8 px vertical padding
 *      │                         │
 *      │      Line 3 (32px)      │  16 pt font with 8 px vertical padding
 *      │                         │
 *      ├─────────────────────────┤
 *      │bottom_status_bar (16 px)│  8 pt font with 4 px vertical padding
 *      └─────────────────────────┘
 *
 *         128x64 display (GD-77)
 *      ┌─────────────────────────┐
 *      │  top_status_bar (8 px)  │  8 pt font without vertical padding
 *      ├─────────────────────────┤
 *      │      Line 1 (20px)      │  16 pt font with 2 px vertical padding
 *      │      Line 2 (20px)      │  16 pt font with 2 px vertical padding
 *      │      Line 3 (8px)       │  8 pt font without vertical padding
 *      ├─────────────────────────┤
 *      │ bottom_status_bar (8 px)│  8 pt font without vertical padding
 *      └─────────────────────────┘
 *
 *         128x48 display (RD-5R)
 *      ┌─────────────────────────┐
 *      │  top_status_bar (8 px)  │  8 pt font without vertical padding
 *      ├─────────────────────────┤
 *      │      Line 1 (20px)      │  16 pt font with 2 px vertical padding
 *      │      Line 2 (20px)      │  16 pt font with 2 px vertical padding
 *      └─────────────────────────┘
 */

#include <stdio.h>
#include "ui.h"
#include "graphics.h"
#include "rtc.h"
#include "platform.h"

typedef struct layout_t
{
    point_t top_pos;
    point_t line1_pos;
    point_t line2_pos;
    point_t line3_pos;
    point_t bottom_pos;
    fontSize_t top_font;
    fontSize_t line1_font;
    fontSize_t line2_font;
    fontSize_t line3_font;
    fontSize_t bottom_font;
} layout_t;

layout_t layout;
bool layout_ready = false;
color_t color_white = {255, 255, 255};

layout_t _ui_calculateLayout()
{
    // Variables needed to calculate positions
    uint16_t top_h = 0;
    uint16_t top_pad = 0;
    uint16_t line1_h = 0;
    uint16_t line2_h = 0;
    uint16_t line3_h = 0;
    uint16_t line_pad = 0;
    uint16_t bottom_pad = 0;
    fontSize_t top_font = 0;
    fontSize_t line1_font = 0;
    fontSize_t line2_font = 0;
    fontSize_t line3_font = 0;
    fontSize_t bottom_font = 0;

    // Calculate UI layout depending on vertical resolution
    // Tytera MD380, MD-UV380
    if (SCREEN_HEIGHT >= 128)
    {
        // Height and padding shown in diagram at beginning of file
        top_h = 16;
        top_pad = 4;
        line1_h = 32;
        line2_h = 32;
        line3_h = 32;
        line_pad = 8;
        bottom_pad = 4;
        
        // Top bar font: 8 pt
        top_font = FONT_SIZE_1;
        // Middle line fonts: 16 pt
        line1_font = FONT_SIZE_3;
        line2_font = FONT_SIZE_3;
        line3_font = FONT_SIZE_3;
        // Bottom bar font: 8 pt
        bottom_font = FONT_SIZE_1;
    }
    // Radioddity GD-77
    else if (SCREEN_HEIGHT >= 64)
    {
        // Height and padding shown in diagram at beginning of file
        top_h = 8;
        top_pad = 0;
        line1_h = 20;
        line2_h = 20;
        line3_h = 0;
        line_pad = 2;
        bottom_pad = 0;

        // Top bar font: 8 pt
        top_font = FONT_SIZE_1;
        // Middle line fonts: 16, 16, 8 pt
        line1_font = FONT_SIZE_3;
        line2_font = FONT_SIZE_3;
        line3_font = FONT_SIZE_1;
        // Bottom bar font: 8 pt
        bottom_font = FONT_SIZE_1;
    }
    else if (SCREEN_HEIGHT >= 48)
    {
        // Height and padding shown in diagram at beginning of file
        top_h = 8;
        top_pad = 0;
        line1_h = 20;
        line2_h = 20;
        line3_h = 0;
        line_pad = 2;
        bottom_pad = 0;

        // Top bar font: 8 pt
        top_font = FONT_SIZE_1;
        // Middle line fonts: 16, 16
        line1_font = FONT_SIZE_3;
        line2_font = FONT_SIZE_3;
        // Not present in this UI
        line3_font = 0;
        bottom_font = 0;
    }
    else
    {
        printf("ERROR: Unsupported vertical resolution: %d\n", SCREEN_HEIGHT);
    }

    // Calculate printing positions
    point_t top_pos = {0, top_pad};
    point_t line1_pos = {0, top_h + line_pad};
    point_t line2_pos = {0, top_h + line1_h + line_pad};
    point_t line3_pos = {0, top_h + line1_h + line2_h + line_pad};
    point_t bottom_pos = {0, top_h + line1_h + line2_h + line3_h + bottom_pad};

    layout_t new_layout = {
        top_pos,
        line1_pos,
        line2_pos,
        line3_pos,
        bottom_pos,
        top_font,
        line1_font,
        line2_font,
        line3_font,
        bottom_font,
    };
    return new_layout;
}

void ui_drawTopBar()
{
    // Print clock on top bar
    char clock_buf[6] = "";
    curTime_t time = rtc_getTime();
    snprintf(clock_buf, sizeof(clock_buf), "%2d:%2d", time.hour, time.minute);
    gfx_print(layout.top_pos, clock_buf, layout.top_font, TEXT_ALIGN_CENTER, color_white);

    // Print battery voltage on top bar, use 4 px padding
    // TODO: Replace with battery icon
    char bat_buf[6] = "";
    float v_bat = platform_getVbat();
    snprintf(bat_buf, sizeof(bat_buf), "%.1fV ", v_bat);
    gfx_print(layout.top_pos, bat_buf, layout.top_font, TEXT_ALIGN_RIGHT, color_white);
}

void ui_drawVFO(state_t state)
{
    // Print VFO frequencies
    char freq_buf[20] = "";
    snprintf(freq_buf, sizeof(freq_buf), "Rx: %09.5f", state.rx_freq);
    gfx_print(layout.line1_pos, freq_buf, layout.line1_font, TEXT_ALIGN_CENTER, color_white);
    snprintf(freq_buf, sizeof(freq_buf), "Tx: %09.5f", state.tx_freq);
    gfx_print(layout.line2_pos, freq_buf, layout.line2_font, TEXT_ALIGN_CENTER, color_white);
}

void ui_drawMainScreen(state_t state)
{
    ui_drawTopBar();
    ui_drawVFO(state);
}

void ui_init()
{
    layout = _ui_calculateLayout();
    layout_ready = true;
}

bool ui_update(state_t state, uint32_t keys)
{
    if(!layout_ready)
    {
        layout = _ui_calculateLayout();
        layout_ready = true;
    }
    gfx_clearScreen();
    ui_drawMainScreen(state);
    return true;
}

void ui_terminate()
{
}
