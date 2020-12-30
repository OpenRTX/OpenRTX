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
#include <interfaces/rtx.h>
#include <interfaces/delays.h>
#include <interfaces/graphics.h>
#include <interfaces/keyboard.h>
#include <interfaces/platform.h>
#include <hwconfig.h>
#include <string.h>
#include <battery.h>

// Maximum menu entry length
#define MAX_ENTRY_LEN 12

const char *menu_items[6] =
{
    "Zone",
    "Channels",
    "Contacts",
    "Messages",
    "GPS",
    "Settings"
};
// Calculate number of main menu entries
const uint8_t menu_num = sizeof(menu_items)/sizeof(menu_items[0]);

const char *settings_items[1] =
{
    "Time & Date"
};
// Calculate number of settings menu entries
const uint8_t settings_num = sizeof(settings_items)/sizeof(settings_items[0]);

typedef struct layout_t
{
    uint16_t top_h;
    uint16_t line1_h;
    uint16_t line2_h;
    uint16_t line3_h;
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

const color_t color_black = {0, 0, 0, 255};
const color_t color_grey = {60, 60, 60, 255};
const color_t color_white = {255, 255, 255, 255};
const color_t yellow_fab413 = {250, 180, 19, 255};

layout_t layout;
bool layout_ready = false;
bool redraw_needed = true;
uint8_t menu_selected = 0;

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
    const uint16_t line1_h = 15;
    const uint16_t line2_h = 15;
    const uint16_t line3_h = 0;
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

    // Radioddity RD-5R
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
    point_t top_pos    = {horizontal_pad, top_h - vertical_pad};
    point_t line1_pos  = {horizontal_pad, top_h + line1_h - line_pad};
    point_t line2_pos  = {horizontal_pad, top_h + line1_h + line2_h - line_pad};
    point_t line3_pos  = {horizontal_pad, top_h + line1_h + line2_h + line3_h - line_pad};
    point_t bottom_pos = {horizontal_pad, top_h + line1_h + line2_h + line3_h + bottom_h - vertical_pad};

    layout_t new_layout =
    {
        top_h,
        line1_h,
        line2_h,
        line3_h,
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

void _ui_drawVFOBackground()
{
    // Print top bar line of 1 pixel height
    gfx_drawHLine(layout.top_h, 1, color_grey);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(SCREEN_HEIGHT - layout.bottom_h - 1, 1, color_grey);
}

void _ui_drawVFOTop(state_t* last_state)
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
    switch(last_state->channel.mode)
    {
        case FM:
        strcpy(mode, "FM");
        break;
        case DMR:
        strcpy(mode, "DMR");
        break;
    }
    gfx_print(layout.top_pos, mode, layout.top_font, TEXT_ALIGN_LEFT,
              color_white);
}

void _ui_drawVFOMiddle(state_t* last_state)
{
    // Print VFO frequencies
    char freq_buf[20] = "";
    point_t freq1_pos = {0,0};
    point_t freq2_pos = {0,0};
    // On radios with 2 rows display use line 1 and 2
    if(layout.line3_h == 0)
    {
        freq1_pos = layout.line1_pos;
        freq2_pos = layout.line2_pos;
    }
    // On radios with 3 rows display use line 2 and 3
    else
    {
        freq1_pos = layout.line2_pos;
        freq2_pos = layout.line3_pos;
    }
    snprintf(freq_buf, sizeof(freq_buf), "Rx: %03lu.%05lu",
             last_state->channel.rx_frequency/1000000,
             last_state->channel.rx_frequency%1000000/10);
    gfx_print(freq1_pos, freq_buf, layout.line1_font, TEXT_ALIGN_CENTER,
              color_white);
    snprintf(freq_buf, sizeof(freq_buf), "Tx: %03lu.%05lu",
             last_state->channel.tx_frequency/1000000,
             last_state->channel.tx_frequency%1000000/10);
    gfx_print(freq2_pos, freq_buf, layout.line2_font, TEXT_ALIGN_CENTER,
              color_white);
}

void _ui_drawVFOBottom()
{
    gfx_print(layout.bottom_pos, "OpenRTX", layout.bottom_font,
              TEXT_ALIGN_CENTER, color_white);
}

void _ui_drawMenuList(point_t pos, const char *entries[], 
                      uint8_t num_entries, uint8_t selected)
{
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen = ((SCREEN_HEIGHT - pos.y) / layout.top_h) + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    for(int item=0; (item < num_entries) && (pos.y < SCREEN_HEIGHT); item++)
    {
        // If selection is off the screen, scroll screen
        if(selected >= entries_in_screen)
            scroll = selected - entries_in_screen + 1;
        snprintf(entry_buf, sizeof(entry_buf), "%s", entries[item + scroll]);
        if(item + scroll == selected)
        {
            // Draw rectangle under selected item, compensating for text height
            point_t rect_pos = {0, pos.y - layout.top_h + 3};
            gfx_drawRect(rect_pos, SCREEN_WIDTH, layout.top_h, color_white, true); 
            gfx_print(pos, entry_buf, layout.top_font, TEXT_ALIGN_LEFT, color_black);
        }
        else
        {
            gfx_print(pos, entry_buf, layout.top_font, TEXT_ALIGN_LEFT, color_white);
        }
        pos.y += layout.top_h;
    }
}

bool _ui_drawMainVFO(state_t* last_state)
{
    bool screen_update = false;
    // Total GUI redraw
    if(redraw_needed)
    {
        gfx_clearScreen();
        _ui_drawVFOBackground();
        point_t splash_origin = {0, SCREEN_HEIGHT / 2 - 6};
        color_t yellow = yellow_fab413;
        yellow.alpha = 0.1f * 255;
        gfx_print(splash_origin, "O P N\nR T X", FONT_SIZE_12PT, TEXT_ALIGN_CENTER,
                  yellow);
        _ui_drawVFOTop(last_state);
        _ui_drawVFOMiddle(last_state);
        _ui_drawVFOBottom();
        screen_update = true;
    }
    // Partial GUI page redraw
    // TODO: until gfx_clearRows() is implemented, we need to redraw everything
    else
    {
        gfx_clearScreen();
        _ui_drawVFOBackground();
        _ui_drawVFOTop(last_state);
        _ui_drawVFOMiddle(last_state);
        _ui_drawVFOBottom();
        screen_update = true;
    }
    return screen_update;
}

void _ui_drawMenuTop()
{
    gfx_clearScreen();
    // Print "Menu" on top bar
    gfx_print(layout.top_pos, "Menu", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print menu entries
    _ui_drawMenuList(layout.line1_pos, menu_items, menu_num, menu_selected);
}

void _ui_drawMenuSettings()
{
    gfx_clearScreen();
    // Print "Settings" on top bar
    gfx_print(layout.top_pos, "Settings", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print menu entries
    _ui_drawMenuList(layout.line1_pos, settings_items, settings_num, menu_selected);
}

void _ui_drawSettingsTimeDate(state_t* last_state)
{
    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print(layout.top_pos, "Time&Date", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print current time and date
    char date_buf[12] = "";
    char time_buf[9] = "";
    snprintf(date_buf, sizeof(date_buf), "%02d/%02d/%02d", 
             last_state->time.date, last_state->time.month, last_state->time.year);
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", 
             last_state->time.hour, last_state->time.minute, last_state->time.second);
    gfx_print(layout.line2_pos, date_buf, layout.line2_font, TEXT_ALIGN_CENTER,
              color_white);
    gfx_print(layout.line3_pos, time_buf, layout.line3_font, TEXT_ALIGN_CENTER,
              color_white);
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

bool _ui_drawLowBatteryScreen()
{
    gfx_clearScreen();
    uint16_t bat_width = SCREEN_WIDTH / 2;
    uint16_t bat_height = SCREEN_HEIGHT / 3;
    point_t bat_pos = {SCREEN_WIDTH / 4, SCREEN_HEIGHT / 8};
    gfx_drawBattery(bat_pos, bat_width, bat_height, 0.1f);
    point_t text_pos_1 = {0, SCREEN_HEIGHT * 2 / 3};
    point_t text_pos_2 = {0, SCREEN_HEIGHT * 2 / 3 + 16};

    gfx_print(text_pos_1,
              "For emergency use",
              FONT_SIZE_6PT,
              TEXT_ALIGN_CENTER,
              color_white);
    gfx_print(text_pos_2,
              "press any button.",
              FONT_SIZE_6PT,
              TEXT_ALIGN_CENTER,
              color_white);
    return true;
}

bool _kbd_number_pressed(kbd_msg_t msg)
{
    return msg.keys & kbd_num_mask;
}

void ui_updateFSM(event_t event, bool *sync_rtx)
{
    // Check if battery has enough charge to operate
    float charge = battery_getCharge(state.v_bat);
    if (!state.emergency && charge <= 0)
    {
        state.ui_screen = LOW_BAT;
        if(event.type == EVENT_KBD && event.payload) {
            state.ui_screen = VFO_MAIN;
            state.emergency = true;
        }
        return;
    }

    // Process pressed keys
    if(event.type == EVENT_KBD)
    {
        kbd_msg_t msg;
        msg.value = event.payload;
        switch(state.ui_screen)
        {
            // VFO screen
            case VFO_MAIN:
                if(msg.keys & KEY_UP)
                {
                    // Advance TX and RX frequency of 12.5KHz
                    state.channel.rx_frequency += 12500;
                    state.channel.tx_frequency += 12500;
                    *sync_rtx = true;
                }
                else if(msg.keys & KEY_DOWN)
                {
                    // Advance TX and RX frequency of 12.5KHz
                    state.channel.rx_frequency -= 12500;
                    state.channel.tx_frequency -= 12500;
                    *sync_rtx = true;
                }
                else if(msg.keys & KEY_ENTER)
                    // Open Menu
                    state.ui_screen = MENU_TOP;
                else if(input_isNumberPressed(msg))
                    // Open Frequency input screen
                    state.ui_screen = VFO_INPUT;
                break;
            // Top menu screen
            case MENU_TOP:
                if(msg.keys & KEY_UP)
                {
                    if(menu_selected > 0)
                        menu_selected -= 1;
                    else
                        menu_selected = menu_num-1;
                }
                else if(msg.keys & KEY_DOWN)
                {
                    if(menu_selected < menu_num-1)
                        menu_selected += 1;
                    else
                        menu_selected = 0;
                }
                else if(msg.keys & KEY_ENTER)
                {
                    // Open selected menu item
                    switch(menu_selected)
                    {
                        // TODO: Add missing submenu states
                        case 5:
                            state.ui_screen = MENU_SETTINGS;
                            break;
                        default:
                            state.ui_screen = MENU_TOP;
                    }
                    // Reset menu selection
                    menu_selected = 0;
                }
                else if(msg.keys & KEY_ESC)
                {
                    // Close Menu
                    state.ui_screen = VFO_MAIN;
                    // Reset menu selection
                    menu_selected = 0;
                }
                break;
            // Settings menu screen
            case MENU_SETTINGS:
                if(msg.keys & KEY_ENTER)
                {
                    // Open selected menu item
                    switch(menu_selected)
                    {
                        // TODO: Add missing submenu states
                        case 0:
                            state.ui_screen = SETTINGS_TIMEDATE;
                            break;
                        default:
                            state.ui_screen = MENU_TOP;
                    }
                    // Reset menu selection
                    menu_selected = 0;
                }
                else if(msg.keys & KEY_ESC)
                {
                    // Return to top menu
                    state.ui_screen = MENU_TOP;
                    // Reset menu selection
                    menu_selected = 0;
                }
                break;
            // Time&Date settingsscreen
            case SETTINGS_TIMEDATE:
                if(msg.keys & KEY_ESC)
                {
                    // Return to settings menu
                    state.ui_screen = MENU_SETTINGS;
                    // Reset menu selection
                    menu_selected = 0;
                }
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
    // TODO: Improve screen_update logic
    bool screen_update = false;
    // Draw current GUI page
    switch(last_state.ui_screen)
    {
        // VFO main screen
        case VFO_MAIN:
            screen_update = _ui_drawMainVFO(&last_state);
            break;
        // VFO frequency input screen
        case VFO_INPUT:
            screen_update = _ui_drawMainVFO(&last_state);
            gfx_print(layout.top_pos, "VFO INPUT", layout.top_font, TEXT_ALIGN_CENTER,
                      color_white);
            break;
        // Top menu screen
        case MENU_TOP:
            _ui_drawMenuTop();
            screen_update = true;
            break;
        // Settings menu screen
        case MENU_SETTINGS:
            _ui_drawMenuSettings();
            screen_update = true;
            break;
        // Time&Date settings screen
        case SETTINGS_TIMEDATE:
            _ui_drawSettingsTimeDate(&last_state);
            screen_update = true;
            break;
        // Low battery screen
        case LOW_BAT:
            screen_update = _ui_drawLowBatteryScreen();
            break;
    }
    return screen_update;
}

void ui_terminate()
{
}
