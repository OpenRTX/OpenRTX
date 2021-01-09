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
 *      │  top_status_bar (16 px) │  8 pt (11 px) font with 2 px vertical padding
 *      ├─────────────────────────┤  1 px line
 *      │                         │
 *      │      Line 1 (34px)      │  12 pt (18 px) font with 8 px vertical padding
 *      │                         │
 *      │      Line 2 (30px)      │  12 pt (18 px) font with 6 px vertical padding
 *      │                         │
 *      │      Line 3 (30px)      │  12 pt (18 px) font with 6 px vertical padding
 *      ├─────────────────────────┤  1 px line
 *      │bottom_status_bar (16 px)│  8 pt (11 px) font with 2 px vertical padding
 *      └─────────────────────────┘
 *
 *         128x64 display (GD-77)
 *      ┌─────────────────────────┐
 *      │  top_status_bar (11 px) │  6 pt (9 px) font with 1 px vertical padding
 *      ├─────────────────────────┤  1 px line
 *      │      Line 1 (10px)      │  6 pt (9 px) font without vertical padding
 *      │      Line 2 (15px)      │  8 pt (11 px) font with 2 px vertical padding
 *      │      Line 3 (15px)      │  8 pt (11 px) font with 2 px vertical padding
 *      ├─────────────────────────┤  1 px line
 *      │ bottom_status_bar(11 px)│  6 pt (9 px) font with 1 px vertical padding
 *      └─────────────────────────┘
 *
 *         128x48 display (RD-5R)
 *      ┌─────────────────────────┐
 *      │  top_status_bar (11 px) │  6 pt (9 px) font with 1 px vertical padding
 *      ├─────────────────────────┤  1 px line
 *      │      Line 2 (19px)      │  8 pt (11 px) font with 4 px vertical padding
 *      │      Line 3 (19px)      │  8 pt (11 px) font with 4 px vertical padding
 *      └─────────────────────────┘
 */

#include <stdio.h>
#include <stdint.h>
#include <ui.h>
#include <interfaces/rtx.h>
#include <interfaces/platform.h>
#include <interfaces/nvmem.h>
#include <string.h>
#include <battery.h>
#include <input.h>

 /* UI menu functions, their implementation is in "ui_menu.c" */
extern void _ui_drawMenuList(point_t pos, const char *entries[],
                            uint8_t num_entries, uint8_t selected);
extern void _ui_drawChannelList(point_t pos, uint8_t selected);
extern void _ui_drawMenuTop(ui_state_t* ui_state);
extern void _ui_drawMenuChannel(ui_state_t* ui_state);
extern void _ui_drawMenuSettings(ui_state_t* ui_state);
#ifdef HAS_RTC
extern void _ui_drawSettingsTimeDate(state_t* last_state, ui_state_t* ui_state);
extern void _ui_drawSettingsTimeDateSet(state_t* last_state, ui_state_t* ui_state);
#endif
extern bool _ui_drawMenuMacro(state_t* last_state);

const char *menu_items[6] =
{
    "Zone",
    "Channels",
    "Contacts",
    "Messages",
    "GPS",
    "Settings"
};

const char *settings_items[1] =
{
#ifdef HAS_RTC
    "Time & Date"
#endif
};

// Calculate number of main menu entries
const uint8_t menu_num = sizeof(menu_items)/sizeof(menu_items[0]);
// Calculate number of settings menu entries
const uint8_t settings_num = sizeof(settings_items)/sizeof(settings_items[0]);

const color_t color_black = {0, 0, 0, 255};
const color_t color_grey = {60, 60, 60, 255};
const color_t color_white = {255, 255, 255, 255};
const color_t yellow_fab413 = {250, 180, 19, 255};

layout_t layout;
ui_state_t ui_state;
bool layout_ready = false;
bool redraw_needed = true;

layout_t _ui_calculateLayout()
{
    // Horizontal line height
    const uint16_t hline_h = 1;
    // Compensate for fonts printing below the start position
    const uint16_t text_v_offset = 1;

    // Calculate UI layout depending on vertical resolution
    // Tytera MD380, MD-UV380
    #if SCREEN_HEIGHT > 127

    // Height and padding shown in diagram at beginning of file
    const uint16_t top_h = 16;
    const uint16_t bottom_h = top_h;
    const uint16_t line1_h = 34;
    const uint16_t line2_h = 30;
    const uint16_t line3_h = 30;
    const uint16_t status_v_pad = 2;
    const uint16_t line_v_pad = 6;
    const uint16_t horizontal_pad = 4;

    // Top bar font: 8 pt
    const fontSize_t top_font = FONT_SIZE_8PT;
    // Middle line fonts: 12 pt
    const fontSize_t line1_font = FONT_SIZE_12PT;
    const fontSize_t line2_font = FONT_SIZE_12PT;
    const fontSize_t line3_font = FONT_SIZE_12PT;
    // Bottom bar font: 8 pt
    const fontSize_t bottom_font = FONT_SIZE_8PT;

    // Radioddity GD-77
    #elif SCREEN_HEIGHT > 63

    // Height and padding shown in diagram at beginning of file
    const uint16_t top_h = 11;
    const uint16_t bottom_h = top_h;
    const uint16_t line1_h = 10;
    const uint16_t line2_h = 15;
    const uint16_t line3_h = 15;
    const uint16_t status_v_pad = 1;
    const uint16_t line_v_pad = 2;
    const uint16_t horizontal_pad = 4;

    // Top bar font: 6 pt
    const fontSize_t top_font = FONT_SIZE_6PT;
    // Middle line fonts: 5, 8, 8 pt
    const fontSize_t line1_font = FONT_SIZE_5PT;
    const fontSize_t line2_font = FONT_SIZE_8PT;
    const fontSize_t line3_font = FONT_SIZE_8PT;
    // Bottom bar font: 6 pt
    const fontSize_t bottom_font = FONT_SIZE_6PT;

    // Radioddity RD-5R
    #elif SCREEN_HEIGHT > 47

    // Height and padding shown in diagram at beginning of file
    const uint16_t top_h = 11;
    const uint16_t bottom_h = 0;
    const uint16_t line1_h = 0;
    const uint16_t line2_h = 19;
    const uint16_t line3_h = 19;
    const uint16_t status_v_pad = 1;
    const uint16_t line_v_pad = 4;
    const uint16_t horizontal_pad = 0;

    // Top bar font: 8 pt
    const fontSize_t top_font = FONT_SIZE_6PT;
    // Middle line fonts: 16, 16
    const fontSize_t line2_font = FONT_SIZE_8PT;
    const fontSize_t line3_font = FONT_SIZE_8PT;
    // Not present in this UI
    const fontSize_t line1_font = 0;
    const fontSize_t bottom_font = 0;

    #else
    #error Unsupported vertical resolution!
    #endif

    // Calculate printing positions
    point_t top_left    = {horizontal_pad, top_h - status_v_pad - text_v_offset};
    point_t line1_left  = {horizontal_pad, top_h + hline_h + line1_h - line_v_pad - text_v_offset};
    point_t line2_left  = {horizontal_pad, top_h + hline_h + line1_h + line2_h - line_v_pad - text_v_offset};
    point_t line3_left  = {horizontal_pad, top_h + hline_h + line1_h + line2_h + line3_h - line_v_pad - text_v_offset};
    point_t bottom_left = {horizontal_pad, top_h + hline_h + line1_h + line2_h + line3_h + hline_h + bottom_h - status_v_pad - text_v_offset};
    point_t top_right    = {SCREEN_WIDTH - horizontal_pad, top_h - status_v_pad - text_v_offset};
    point_t line1_right  = {SCREEN_WIDTH - horizontal_pad, top_h + hline_h + line1_h - line_v_pad - text_v_offset};
    point_t line2_right  = {SCREEN_WIDTH - horizontal_pad, top_h + hline_h + line1_h + line2_h - line_v_pad - text_v_offset};
    point_t line3_right  = {SCREEN_WIDTH - horizontal_pad, top_h + hline_h + line1_h + line2_h + line3_h - line_v_pad - text_v_offset};
    point_t bottom_right = {SCREEN_WIDTH - horizontal_pad, top_h + hline_h + line1_h + line2_h + line3_h + hline_h + bottom_h - status_v_pad - text_v_offset};

    layout_t new_layout =
    {
        hline_h,
        top_h,
        line1_h,
        line2_h,
        line3_h,
        bottom_h,
        status_v_pad,
        line_v_pad,
        horizontal_pad,
        text_v_offset,
        top_left,
        line1_left,
        line2_left,
        line3_left,
        bottom_left,
        top_right,
        line1_right,
        line2_right,
        line3_right,
        bottom_right,
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
    // Print top bar line of hline_h pixel height
    gfx_drawHLine(layout.top_h, layout.hline_h, color_grey);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(SCREEN_HEIGHT - layout.bottom_h - 1, layout.hline_h, color_grey);
    // Print transparent OPNRTX on the background
    point_t splash_origin = {0, SCREEN_HEIGHT / 2 - 6};
    color_t yellow = yellow_fab413;
    yellow.alpha = 0.1f * 255;
    gfx_print(splash_origin, "O P N\nR T X", FONT_SIZE_12PT, TEXT_ALIGN_CENTER,
              yellow);
}

void _ui_drawVFOTop(state_t* last_state)
{
#ifdef HAS_RTC
    // Print clock on top bar
    char clock_buf[9] = "";
    snprintf(clock_buf, sizeof(clock_buf), "%02d:%02d:%02d", last_state->time.hour,
             last_state->time.minute, last_state->time.second);
    gfx_print(layout.top_left, clock_buf, layout.top_font, TEXT_ALIGN_CENTER,
              color_white);
#endif

    // Print battery icon on top bar, use 4 px padding
    float charge = battery_getCharge(last_state->v_bat);
    uint16_t bat_width = SCREEN_WIDTH / 9;
    uint16_t bat_height = layout.top_h - (layout.status_v_pad * 2);
    point_t bat_pos = {SCREEN_WIDTH - bat_width - layout.horizontal_pad,
                       layout.status_v_pad};
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
    gfx_print(layout.top_left, mode, layout.top_font, TEXT_ALIGN_LEFT,
              color_white);
}

void _ui_drawVFOMiddle(state_t* last_state)
{
    // Print VFO frequencies
    char freq_buf[20] = "";
    snprintf(freq_buf, sizeof(freq_buf), " Rx:%03u.%05u",
             last_state->channel.rx_frequency/1000000,
             last_state->channel.rx_frequency%1000000/10);
    gfx_print(layout.line2_left, freq_buf, layout.line2_font, TEXT_ALIGN_CENTER,
              color_white);
    snprintf(freq_buf, sizeof(freq_buf), " Tx:%03u.%05u",
             last_state->channel.tx_frequency/1000000,
             last_state->channel.tx_frequency%1000000/10);
    gfx_print(layout.line3_left, freq_buf, layout.line3_font, TEXT_ALIGN_CENTER,
              color_white);
}

void _ui_drawVFOMiddleInput(state_t* last_state, ui_state_t* ui_state)
{
    // Add inserted number to string, skipping "Rx: "/"Tx: " and "."
    uint8_t insert_pos = ui_state->input_position + 3;
    if(ui_state->input_position > 3) insert_pos += 1;
    char input_char = ui_state->input_number + '0';
    char freq_buf[14] = "";

    if(ui_state->input_set == SET_RX)
    {
        if(ui_state->input_position == 0)
        {
            snprintf(freq_buf, sizeof(freq_buf), ">Rx:%03u.%05u",
                     ui_state->new_rx_frequency/1000000,
                     ui_state->new_rx_frequency%1000000/10);
            gfx_print(layout.line2_left, freq_buf, layout.line2_font, TEXT_ALIGN_CENTER,
                      color_white);
        }
        else
        {
            // Replace Rx frequency with underscorses
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_rx_freq_buf, ">Rx:___._____");
            ui_state->new_rx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line2_left, ui_state->new_rx_freq_buf, layout.line2_font, TEXT_ALIGN_CENTER,
                      color_white);
        }
        snprintf(freq_buf, sizeof(freq_buf), " Tx:%03u.%05u",
                 last_state->channel.tx_frequency/1000000,
                 last_state->channel.tx_frequency%1000000/10);
        gfx_print(layout.line3_left, freq_buf, layout.line3_font, TEXT_ALIGN_CENTER,
                  color_white);
    }
    else if(ui_state->input_set == SET_TX)
    {
        snprintf(freq_buf, sizeof(freq_buf), " Rx:%03u.%05u",
                 ui_state->new_rx_frequency/1000000,
                 ui_state->new_rx_frequency%1000000/10);
        gfx_print(layout.line2_left, freq_buf, layout.line2_font, TEXT_ALIGN_CENTER,
                  color_white);
        // Replace Rx frequency with underscorses
        if(ui_state->input_position == 0)
        {
            snprintf(freq_buf, sizeof(freq_buf), ">Tx:%03u.%05u",
                     ui_state->new_rx_frequency/1000000,
                     ui_state->new_rx_frequency%1000000/10);
            gfx_print(layout.line3_left, freq_buf, layout.line3_font, TEXT_ALIGN_CENTER,
                      color_white);
        }
        else
        {
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_tx_freq_buf, ">Tx:___._____");
            ui_state->new_tx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line3_left, ui_state->new_tx_freq_buf, layout.line3_font, TEXT_ALIGN_CENTER,
                      color_white);
        }
    }
}

void _ui_drawVFOBottom()
{
    gfx_print(layout.bottom_left, "OpenRTX", layout.bottom_font,
              TEXT_ALIGN_CENTER, color_white);
}

void _ui_drawVFOMain(state_t* last_state)
{
    gfx_clearScreen();
    _ui_drawVFOBackground();
    _ui_drawVFOTop(last_state);
    _ui_drawVFOMiddle(last_state);
    _ui_drawVFOBottom();
}

void _ui_drawVFOInput(state_t* last_state)
{
    gfx_clearScreen();
    _ui_drawVFOBackground();
    _ui_drawVFOTop(last_state);
    _ui_drawVFOMiddleInput(last_state, &ui_state);
    _ui_drawVFOBottom();
}

void ui_init()
{
    redraw_needed = true;
    layout = _ui_calculateLayout();
    layout_ready = true;
    // Initialize struct ui_state to all zeroes
    // This syntax is called compound literal
    // https://stackoverflow.com/questions/6891720/initialize-reset-struct-to-zero-null
    ui_state = (const struct ui_state_t){ 0 };
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

void _ui_drawLowBatteryScreen()
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
}

freq_t _ui_freq_add_digit(freq_t freq, uint8_t pos, uint8_t number)
{
    freq_t coefficient = 10;
    for(uint8_t i=0; i < FREQ_DIGITS - pos; i++)
    {
        coefficient *= 10;
    }
    return freq += number * coefficient;
}

#ifdef HAS_RTC
curTime_t _ui_timedate_add_digit(curTime_t timedate, uint8_t pos, uint8_t number)
{
    switch(pos)
    {
        // Set date
        case 1:
            timedate.date += number * 10;
            break;
        case 2:
            timedate.date += number;
            break;
        // Set month
        case 3:
            timedate.month += number * 10;
            break;
        case 4:
            timedate.month += number;
            break;
        // Set year
        case 5:
            timedate.year += number * 10;
            break;
        case 6:
            timedate.year += number;
            break;
        // Set hour
        case 7:
            timedate.hour += number * 10;
            break;
        case 8:
            timedate.hour += number;
            break;
        // Set minute
        case 9:
            timedate.minute += number * 10;
            break;
        case 10:
            timedate.minute += number;
            break;
    }
    return timedate;
}
#endif

bool _ui_freq_check_limits(freq_t freq)
{
    bool valid = false;
#ifdef BAND_VHF
    if(freq >= FREQ_LIMIT_VHF_LO && freq <= FREQ_LIMIT_VHF_HI)
        valid = true;
#endif
#ifdef BAND_UHF
    if(freq >= FREQ_LIMIT_UHF_LO && freq <= FREQ_LIMIT_UHF_HI)
        valid = true;
#endif
    return valid;
}

bool _ui_drawDarkOverlay() {
    // TODO: Make this 245 alpha and fix alpha frame swap
    color_t alpha_grey = {0, 0, 0, 255};
    point_t origin = {0, 0};
    gfx_drawRect(origin, SCREEN_WIDTH, SCREEN_HEIGHT, alpha_grey, true);
    return true;
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
                    // Increment TX and RX frequency of 12.5KHz
                    if(_ui_freq_check_limits(state.channel.rx_frequency + 12500) &&
                       _ui_freq_check_limits(state.channel.tx_frequency + 12500))
                    {
                        state.channel.rx_frequency += 12500;
                        state.channel.tx_frequency += 12500;
                        *sync_rtx = true;
                    }
                }
                else if(msg.keys & KEY_DOWN)
                {
                    // Decrement TX and RX frequency of 12.5KHz
                    if(_ui_freq_check_limits(state.channel.rx_frequency - 12500) &&
                       _ui_freq_check_limits(state.channel.tx_frequency - 12500))
                    {
                        state.channel.rx_frequency -= 12500;
                        state.channel.tx_frequency -= 12500;
                        *sync_rtx = true;
                    }
                }
                else if(msg.keys & KEY_ENTER)
                {
                    // Open Menu
                    state.ui_screen = MENU_TOP;
                }
                else if(input_isNumberPressed(msg))
                {
                    // Open Frequency input screen
                    state.ui_screen = VFO_INPUT;
                    // Reset input position and selection
                    ui_state.input_position = 1;
                    ui_state.input_set = SET_RX;
                    ui_state.new_rx_frequency = 0;
                    ui_state.new_tx_frequency = 0;
                    // Save pressed number to calculare frequency and show in GUI
                    ui_state.input_number = input_getPressedNumber(msg);
                    // Calculate portion of the new frequency
                    ui_state.new_rx_frequency = _ui_freq_add_digit(ui_state.new_rx_frequency, 
                                            ui_state.input_position, ui_state.input_number);
                }
                else if(msg.keys & KEY_MONI)
                {
                    // Open Macro Menu
                    _ui_drawDarkOverlay();
                    state.ui_screen = MENU_MACRO;
                }
                break;
            // VFO frequency input screen
            case VFO_INPUT:
                if(msg.keys & KEY_ENTER)
                {
                    // Switch to TX input
                    if(ui_state.input_set == SET_RX)
                    {
                        ui_state.input_set = SET_TX;
                        // Reset input position
                        ui_state.input_position = 0;
                    }
                    else if(ui_state.input_set == SET_TX)
                    {
                        // Save new frequency setting
                        // If TX frequency was not set, TX = RX
                        if(ui_state.new_tx_frequency == 0)
                        {
                            if(_ui_freq_check_limits(ui_state.new_rx_frequency))
                            {
                                state.channel.rx_frequency = ui_state.new_rx_frequency;
                                state.channel.tx_frequency = ui_state.new_rx_frequency;
                                *sync_rtx = true;
                            }
                        }
                        // Otherwise set both frequencies
                        else
                        {
                            if(_ui_freq_check_limits(ui_state.new_rx_frequency) && 
                               _ui_freq_check_limits(ui_state.new_tx_frequency))
                            {
                                state.channel.rx_frequency = ui_state.new_rx_frequency;
                                state.channel.tx_frequency = ui_state.new_tx_frequency;
                                *sync_rtx = true;
                            }
                        }
                        state.ui_screen = VFO_MAIN;
                    }
                }
                else if(msg.keys & KEY_ESC)
                {
                    state.ui_screen = VFO_MAIN;
                }
                else if(msg.keys & KEY_UP || msg.keys & KEY_DOWN)
                {
                    if(ui_state.input_set == SET_RX)
                        ui_state.input_set = SET_TX;
                    else if(ui_state.input_set == SET_TX)
                        ui_state.input_set = SET_RX;
                    // Reset input position
                    ui_state.input_position = 0;
                }
                else if(input_isNumberPressed(msg))
                {
                    // Advance input position
                    ui_state.input_position += 1;
                    // Save pressed number to calculare frequency and show in GUI
                    ui_state.input_number = input_getPressedNumber(msg);
                    if(ui_state.input_set == SET_RX)
                    {
                        if(ui_state.input_position == 1)
                            ui_state.new_rx_frequency = 0;
                        // Calculate portion of the new RX frequency
                        ui_state.new_rx_frequency = _ui_freq_add_digit(ui_state.new_rx_frequency, 
                                                ui_state.input_position, ui_state.input_number);
                        if(ui_state.input_position >= FREQ_DIGITS)
                        {
                            // Switch to TX input
                            ui_state.input_set = SET_TX;
                            // Reset input position
                            ui_state.input_position = 0;
                            // Reset TX frequency
                            ui_state.new_tx_frequency = 0;
                        }
                    }
                    else if(ui_state.input_set == SET_TX)
                    {
                        if(ui_state.input_position == 1)
                            ui_state.new_tx_frequency = 0;
                        // Calculate portion of the new TX frequency
                        ui_state.new_tx_frequency = _ui_freq_add_digit(ui_state.new_tx_frequency, 
                                                ui_state.input_position, ui_state.input_number);
                        if(ui_state.input_position >= FREQ_DIGITS)
                        {
                            // Save both inserted frequencies
                            if(_ui_freq_check_limits(ui_state.new_rx_frequency) && 
                               _ui_freq_check_limits(ui_state.new_tx_frequency))
                            {
                                state.channel.rx_frequency = ui_state.new_rx_frequency;
                                state.channel.tx_frequency = ui_state.new_tx_frequency;
                                *sync_rtx = true;
                            }
                            state.ui_screen = VFO_MAIN;
                        }
                    }
                }
                else if(msg.keys & KEY_MONI)
                {
                    // Open Macro Menu
                    _ui_drawDarkOverlay();
                    state.ui_screen = MENU_MACRO;
                }
                break;
            // Top menu screen
            case MENU_TOP:
                if(msg.keys & KEY_UP)
                {
                    if(ui_state.menu_selected > 0)
                        ui_state.menu_selected -= 1;
                    else
                        ui_state.menu_selected = menu_num-1;
                }
                else if(msg.keys & KEY_DOWN)
                {
                    if(ui_state.menu_selected < menu_num-1)
                        ui_state.menu_selected += 1;
                    else
                        ui_state.menu_selected = 0;
                }
                else if(msg.keys & KEY_ENTER)
                {
                    // Open selected menu item
                    switch(ui_state.menu_selected)
                    {
                        // TODO: Add missing submenu states
                        case 1:
                            state.ui_screen = MENU_CHANNEL;
                            break;
                        case 5:
                            state.ui_screen = MENU_SETTINGS;
                            break;
                        default:
                            state.ui_screen = MENU_TOP;
                    }
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                }
                else if(msg.keys & KEY_ESC)
                {
                    // Close Menu
                    state.ui_screen = VFO_MAIN;
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                }
                break;
            // Channel menu screen
            case MENU_CHANNEL:
                if(msg.keys & KEY_UP)
                {
                    if(ui_state.menu_selected > 0)
                        ui_state.menu_selected -= 1;
                }
                else if(msg.keys & KEY_DOWN)
                {
                    ui_state.menu_selected += 1;
                }
                else if(msg.keys & KEY_ESC)
                {
                    // Return to top menu
                    state.ui_screen = MENU_TOP;
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                }
                else if(msg.keys & KEY_MONI)
                {
                    // Open Macro Menu
                    state.ui_screen = MENU_MACRO;
                }
                break;
            // Macro menu
            case MENU_MACRO:
                _ui_drawDarkOverlay();
                // If a number is pressed perform the corresponding macro
                if(!msg.long_press && input_isNumberPressed(msg))
                {
                    ui_state.input_number = input_getPressedNumber(msg);
                    // Backlight
                    int32_t new_blight = state.backlight_level;
                    // CTCSS Encode/Decode Selection
                    bool tone_tx_enable = state.channel.fm.txToneEn;
                    bool tone_rx_enable = state.channel.fm.rxToneEn;
                    uint8_t tone_flags = tone_tx_enable << 1 | tone_rx_enable;
                    switch(ui_state.input_number)
                    {
                        case 1:
                            state.channel.fm.txTone++;
                            state.channel.fm.txTone %= MAX_TONE_INDEX;
                            state.channel.fm.rxTone = state.channel.fm.txTone;
                            *sync_rtx = true;
                            break;
                        case 2:
                            tone_flags++;
                            tone_flags %= 4;
                            tone_tx_enable = tone_flags >> 1;
                            tone_rx_enable = tone_flags & 1;
                            state.channel.fm.txToneEn = tone_tx_enable;
                            state.channel.fm.rxToneEn = tone_rx_enable;
                            *sync_rtx = true;
                            break;
                        case 3:
                            if (state.channel.power == 1.0f)
                                state.channel.power = 5.0f;
                            else
                                state.channel.power = 1.0f;
                            *sync_rtx = true;
                            break;
                        case 4:
                            state.channel.bandwidth++;
                            state.channel.bandwidth %= 3;
                            *sync_rtx = true;
                            break;
                        case 7:
                            new_blight += 25;
                            new_blight = (new_blight > 255) ? 255 : new_blight;
                            state.backlight_level = new_blight;
                            platform_setBacklightLevel(state.backlight_level);
                            break;
                        case 8:
                            new_blight -= 25;
                            new_blight = (new_blight < 0) ? 0 : new_blight;
                            state.backlight_level = new_blight;
                            platform_setBacklightLevel(state.backlight_level);
                            break;
                    }
                }
                // Exit from this menu when monitor key is released
                if(!(msg.keys & KEY_MONI))
                    state.ui_screen = VFO_MAIN;
                break;
            // Settings menu screen
            case MENU_SETTINGS:
                if(msg.keys & KEY_ENTER)
                {
                    // Open selected menu item
                    switch(ui_state.menu_selected)
                    {
#ifdef HAS_RTC
                        // TODO: Add missing submenu states
                        case 0:
                            state.ui_screen = SETTINGS_TIMEDATE;
                            break;
#endif
                        default:
                            state.ui_screen = MENU_TOP;
                    }
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                }
                else if(msg.keys & KEY_ESC)
                {
                    // Return to top menu
                    state.ui_screen = MENU_TOP;
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                }
                break;
#ifdef HAS_RTC
            // Time&Date settings screen
            case SETTINGS_TIMEDATE:
                if(msg.keys & KEY_ENTER)
                {
                    // Switch to set Time&Date mode
                    state.ui_screen = SETTINGS_TIMEDATE_SET;
                    // Reset input position and selection
                    ui_state.input_position = 0;
                    memset(&ui_state.new_timedate, 0, sizeof(curTime_t));
                }
                else if(msg.keys & KEY_ESC)
                {
                    // Return to settings menu
                    state.ui_screen = MENU_SETTINGS;
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                }
                break;
            // Time&Date settings screen, edit mode
            case SETTINGS_TIMEDATE_SET:
                if(msg.keys & KEY_ENTER)
                {
                    // Save time only if all digits have been inserted
                    if(ui_state.input_position < TIMEDATE_DIGITS)
                        break;
                    // Return to Time&Date menu, saving values
                    rtc_setTime(ui_state.new_timedate);
                    state.ui_screen = SETTINGS_TIMEDATE;
                }
                else if(msg.keys & KEY_ESC)
                {
                    // Return to Time&Date menu discarding values
                    state.ui_screen = SETTINGS_TIMEDATE;
                }
                else if(input_isNumberPressed(msg))
                {
                    // Discard excess digits
                    if(ui_state.input_position > TIMEDATE_DIGITS)
                        break;
                    ui_state.input_position += 1;
                    ui_state.input_number = input_getPressedNumber(msg);
                    ui_state.new_timedate = _ui_timedate_add_digit(ui_state.new_timedate, 
                                        ui_state.input_position, ui_state.input_number);
                }
                break;
#endif
        }
    }
}

void ui_updateGUI(state_t last_state)
{
    if(!layout_ready)
    {
        layout = _ui_calculateLayout();
        layout_ready = true;
    }
    // Draw current GUI page
    switch(last_state.ui_screen)
    {
        // VFO main screen
        case VFO_MAIN:
            _ui_drawVFOMain(&last_state);
            break;
        // VFO frequency input screen
        case VFO_INPUT:
            _ui_drawVFOInput(&last_state);
            break;
        // Top menu screen
        case MENU_TOP:
            _ui_drawMenuTop(&ui_state);
            break;
        // Channel menu screen
        case MENU_CHANNEL:
            _ui_drawMenuChannel(&ui_state);
            break;
        // Macro menu
        case MENU_MACRO:
            _ui_drawMenuMacro(&last_state);
            break;
        // Settings menu screen
        case MENU_SETTINGS:
            _ui_drawMenuSettings(&ui_state);
            break;
#ifdef HAS_RTC
        // Time&Date settings screen
        case SETTINGS_TIMEDATE:
            _ui_drawSettingsTimeDate(&last_state, &ui_state);
            break;
        // Time&Date settings screen, edit mode
        case SETTINGS_TIMEDATE_SET:
            _ui_drawSettingsTimeDateSet(&last_state, &ui_state);
            break;
#endif
        // Low battery screen
        case LOW_BAT:
            _ui_drawLowBatteryScreen();
            break;
    }
}

void ui_terminate()
{
}
