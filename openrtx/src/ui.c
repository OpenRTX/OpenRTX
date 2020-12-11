/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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
 *      │      Line 1 (15px)      │  16 pt font with 2 px vertical padding
 *      │      Line 2 (15px)      │  16 pt font with 2 px vertical padding
 *      └─────────────────────────┘
 */

#include <stdio.h>
#include <stdint.h>
#include <ui.h>
#include <delays.h>
#include <graphics.h>
#include <keyboard.h>
#include <platform.h>
#include <hwconfig.h>
#include <string.h>
#include <battery.h>

const char *menuItems[MENU_NUM] =
{
    "Zone",
    "Channels",
    "Contacts",
    "Messages",
    "GPS",
    "Settings"
};


typedef struct layout_t
{
    uint16_t top_h;
    uint16_t line1_h;
    uint16_t bottom_h;
    uint16_t vertical_pad;
    uint16_t horizontal_pad;
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

const color_t color_white = {255, 255, 255};
const color_t color_grey = {60, 60, 60};
const color_t yellow_fab413 = {250, 180, 19};

layout_t layout;
bool layout_ready = false;
bool redraw_needed = true;

layout_t _ui_calculateLayout()
{
    // Calculate UI layout depending on vertical resolution
    // Tytera MD380, MD-UV380
    #if SCREEN_HEIGHT > 127

    // Height and padding shown in diagram at beginning of file
    const uint16_t top_h = 16;
    const uint16_t bottom_h = top_h;
    const uint16_t line1_h = 32;
    const uint16_t line2_h = 32;
    const uint16_t line3_h = 32;
    const uint16_t line_pad = 8;
    const uint16_t vertical_pad = 4;
    const uint16_t horizontal_pad = 4;

    // Top bar font: 8 pt
    const fontSize_t top_font = FONT_SIZE_8PT;
    // Middle line fonts: 16 pt
    const fontSize_t line1_font = FONT_SIZE_12PT;
    const fontSize_t line2_font = FONT_SIZE_12PT;
    const fontSize_t line3_font = FONT_SIZE_12PT;
    // Bottom bar font: 8 pt
    const fontSize_t bottom_font = FONT_SIZE_8PT;

    // Radioddity GD-77
    #elif SCREEN_HEIGHT > 63

    // Height and padding shown in diagram at beginning of file
    const uint16_t top_h = 13;
    const uint16_t bottom_h = 18;
    const uint16_t line1_h = 0;
    const uint16_t line2_h = 15;
    const uint16_t line3_h = 15;
    const uint16_t line_pad = 2;
    const uint16_t vertical_pad = 4;
    const uint16_t horizontal_pad = 4;

    // Top bar font: 8 pt
    const fontSize_t top_font = FONT_SIZE_6PT;
    // Middle line fonts: 16, 16, 8 pt
    const fontSize_t line1_font = FONT_SIZE_8PT;
    const fontSize_t line2_font = FONT_SIZE_8PT;
    const fontSize_t line3_font = FONT_SIZE_5PT;
    // Bottom bar font: 8 pt
    const fontSize_t bottom_font = FONT_SIZE_6PT;
    
    #elif SCREEN_HEIGHT > 47

    // Height and padding shown in diagram at beginning of file
    const uint16_t top_h = 8;
    const uint16_t bottom_h = 0;
    const uint16_t line1_h = 20;
    const uint16_t line2_h = 20;
    const uint16_t line3_h = 0;
    const uint16_t line_pad = 2;
    const uint16_t vertical_pad = 0;
    const uint16_t horizontal_pad = 0;

    // Top bar font: 8 pt
    const fontSize_t top_font = FONT_SIZE_1;
    // Middle line fonts: 16, 16
    const fontSize_t line1_font = FONT_SIZE_3;
    const fontSize_t line2_font = FONT_SIZE_3;
    // Not present in this UI
    const fontSize_t line3_font = 0;
    const fontSize_t bottom_font = 0;

    #else
    #error Unsupported vertical resolution!
    #endif

    // Calculate printing positions
    point_t top_pos    = {0, top_h - vertical_pad};
    point_t line1_pos  = {0, top_h + line1_h - line_pad};
    point_t line2_pos  = {0, top_h + line1_h + line2_h - line_pad};
    point_t line3_pos  = {0, top_h + line1_h + line2_h + line3_h - line_pad};
    point_t bottom_pos = {0, top_h + line1_h + line2_h + line3_h + bottom_h - vertical_pad};

    layout_t new_layout =
    {
        top_h,
        line1_h,
        bottom_h,
        vertical_pad,
        horizontal_pad,
        top_pos,
        line1_pos,
        line2_pos,
        line3_pos,
        bottom_pos,
        top_font,
        line1_font,
        line2_font,
        line3_font,
        bottom_font
    };
    return new_layout;
}

void _ui_drawBackground()
{
    // Print top bar line of 1 pixel height
    gfx_drawHLine(layout.top_h, 1, color_grey);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(SCREEN_HEIGHT - layout.bottom_h - 1, 1, color_grey);
}

void _ui_drawTopBar(state_t* last_state)
{
    // Print clock on top bar
    char clock_buf[9] = "";
    snprintf(clock_buf, sizeof(clock_buf), "%02d:%02d:%02d", last_state->time.hour, 
             last_state->time.minute, last_state->time.second);
    gfx_print(layout.top_pos, clock_buf, layout.top_font, TEXT_ALIGN_CENTER,
              color_white);

    // Print battery icon on top bar, use 4 px padding
    float charge = battery_getCharge(last_state->v_bat);
    uint16_t bat_width = SCREEN_WIDTH / 9;
    uint16_t bat_height = layout.top_h - layout.vertical_pad;
    point_t bat_pos = {SCREEN_WIDTH - bat_width - layout.horizontal_pad,
                       layout.vertical_pad / 2};
    gfx_drawBattery(bat_pos, bat_width, bat_height, charge);

    // Print radio mode on top bar
    char mode[4] = "";
    switch(last_state->radio_mode)
    {
        case MODE_FM:
        strcpy(mode, "FM");
        break;
        case MODE_NFM:
        strcpy(mode, "NFM");
        break;
        case MODE_DMR:
        strcpy(mode, "DMR");
        break;
    }
    gfx_print(layout.top_pos, mode, layout.top_font, TEXT_ALIGN_LEFT,
              color_white);
}

void _ui_drawMiddleVFO(state_t* last_state)
{
    // Print VFO frequencies
    char freq_buf[20] = "";
    snprintf(freq_buf, sizeof(freq_buf), "Rx: %03u.%05u",
             last_state->channel.rx_frequency/1000000,
             last_state->channel.rx_frequency%1000000/10);

    gfx_print(layout.line2_pos, freq_buf, layout.line1_font, TEXT_ALIGN_CENTER,
              color_white);

    snprintf(freq_buf, sizeof(freq_buf), "Tx: %03u.%05u",
             last_state->channel.tx_frequency/1000000,
             last_state->channel.tx_frequency%1000000/10);

    gfx_print(layout.line3_pos, freq_buf, layout.line2_font, TEXT_ALIGN_CENTER,
              color_white);
}

void _ui_drawBottomBar()
{
    gfx_print(layout.bottom_pos, "OpenRTX", layout.bottom_font,
              TEXT_ALIGN_CENTER, color_white);
}

bool _ui_drawMainVFO(state_t* last_state)
{
    bool screen_update = false;
    // Total GUI redraw
    if(redraw_needed)
    {
        gfx_clearScreen();
        _ui_drawBackground();
        _ui_drawTopBar(last_state);
        _ui_drawMiddleVFO(last_state);
        _ui_drawBottomBar();
        screen_update = true;
    }
    // Partial GUI page redraw
    // TODO: until gfx_clearRows() is implemented, we need to redraw everything
    else
    {
        gfx_clearScreen();
        _ui_drawBackground();
        _ui_drawTopBar(last_state);
        _ui_drawMiddleVFO(last_state);
        _ui_drawBottomBar();
        screen_update = true;
    }
    return screen_update;
}

bool _ui_drawMenuTop()
{
    bool screen_update = false;
    // Total GUI page redraw
    if(redraw_needed)
    {
        // Print "menu" on top bar
        gfx_clearScreen();
        gfx_print(layout.top_pos, "Menu", layout.top_font,
                  TEXT_ALIGN_CENTER, color_white);
        // Print menu entries
        point_t pos = {layout.horizontal_pad, layout.line1_h};
        char entry_buf[MENU_LEN] = "";
        for(int item=0; (item < MENU_NUM) && (pos.y < SCREEN_HEIGHT); item++)
        {
            snprintf(entry_buf, sizeof(entry_buf), "%s", menuItems[item]);
            gfx_print(pos, entry_buf, layout.top_font, 
                      TEXT_ALIGN_LEFT, color_white);
            pos.y += layout.top_h;
        }
        screen_update = true;
    }
    return screen_update;
}

void ui_init()
{
    redraw_needed = true;
    layout = _ui_calculateLayout();
    layout_ready = true;
}

void ui_drawSplashScreen()
{
    gfx_clearScreen();

    #ifdef OLD_SPLASH
    point_t splash_origin = {0, SCREEN_HEIGHT / 2 + 6};
    gfx_print(splash_origin, "OpenRTX", FONT_SIZE_12PT, TEXT_ALIGN_CENTER,
              yellow_fab413);
    #else
    point_t splash_origin = {0, SCREEN_HEIGHT / 2 - 6};
    gfx_print(splash_origin, "O P N\nR T X", FONT_SIZE_12PT, TEXT_ALIGN_CENTER,
              yellow_fab413);
    #endif
}

void ui_updateFSM(event_t event)
{
    // Process pressed keys
    if(event.type == EVENT_KBD)
    {
        keyboard_t keys = event.payload;
        switch(state.ui_screen)
        {
            // VFO screen
            case MAIN_VFO:
                // Temporary VFO controls
                if(keys & KEY_UP)
                {
                    // Advance TX and RX frequency of 12.5KHz
                    state.channel.rx_frequency += 12500;
                    state.channel.tx_frequency += 12500;
                }
                else if(keys & KEY_DOWN)
                {
                    // Advance TX and RX frequency of 12.5KHz
                    state.channel.rx_frequency -= 12500;
                    state.channel.tx_frequency -= 12500;
                }
                else if(keys & KEY_ENTER)
                    // Open Menu
                    state.ui_screen = MENU_TOP;
                break;
            // Top menu screen
            case MENU_TOP:
                if(keys & KEY_UP)
                {
                }
                else if(keys & KEY_DOWN)
                {
                }
                else if(keys & KEY_ESC)
                    // Close Menu
                    state.ui_screen = MAIN_VFO;
                break;
        }
    }
}

bool ui_updateGUI(state_t last_state)
{
    if(!layout_ready)
    {
        layout = _ui_calculateLayout();
        layout_ready = true;
    }
    bool screen_update = false;
    // Draw current GUI page
    switch(last_state.ui_screen)
    {
        // VFO screen
        case MAIN_VFO:
            screen_update = _ui_drawMainVFO(&last_state);
            break;
        // Top menu screen
        case MENU_TOP:
            screen_update = _ui_drawMenuTop();
            break;
    }
    return screen_update;
}

void ui_terminate()
{
}
