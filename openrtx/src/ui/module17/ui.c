/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#include <stdio.h>
#include <stdint.h>
#include <ui/ui_mod17.h>
#include <rtx.h>
#include <interfaces/platform.h>
#include <interfaces/display.h>
#include <interfaces/cps_io.h>
#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include <string.h>
#include <battery.h>
#include <input.h>
#include <hwconfig.h>

/* UI main screen functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainBackground();
extern void _ui_drawMainTop();
extern void _ui_drawVFOMiddle();
extern void _ui_drawMEMMiddle();
extern void _ui_drawVFOBottom();
extern void _ui_drawMEMBottom();
extern void _ui_drawMainVFO(ui_state_t* ui_state);
extern void _ui_drawMainVFOInput(ui_state_t* ui_state);
extern void _ui_drawMainMEM(ui_state_t* ui_state);
/* UI menu functions, their implementation is in "ui_menu.c" */
extern void _ui_drawMenuTop(ui_state_t* ui_state);
#ifdef CONFIG_GPS
extern void _ui_drawMenuGPS();
extern void _ui_drawSettingsGPS(ui_state_t* ui_state);
#endif
extern void _ui_drawMenuSettings(ui_state_t* ui_state);
extern void _ui_drawMenuInfo(ui_state_t* ui_state);
extern void _ui_drawMenuAbout();
#ifdef CONFIG_RTC
extern void _ui_drawSettingsTimeDate();
extern void _ui_drawSettingsTimeDateSet(ui_state_t* ui_state);
#endif
extern void _ui_drawSettingsDisplay(ui_state_t* ui_state);
extern void _ui_drawSettingsM17(ui_state_t* ui_state);
extern void _ui_drawSettingsModule17(ui_state_t* ui_state);
extern void _ui_drawSettingsReset2Defaults(ui_state_t* ui_state);
extern bool _ui_drawMacroMenu(ui_state_t* ui_state);

const char *menu_items[] =
{
    "Settings",
#ifdef CONFIG_GPS
    "GPS",
#endif
    "Info",
    "About",
    "Shutdown"
};

const char *settings_items[] =
{
    "Display",
#ifdef CONFIG_RTC
    "Time & Date",
#endif
#ifdef CONFIG_GPS
    "GPS",
#endif
    "M17",
    "Module 17",
    "Default Settings"
};

const char *display_items[] =
{
    "Brightness",
};

const char *m17_items[] =
{
    "Callsign",
    "CAN",
    "CAN RX Check"
};

const char *module17_items[] =
{
    "Mic Gain",
    "PTT In",
    "PTT Out",
    "TX Phase",
    "RX Phase",
    "TX Softpot",
    "RX Softpot"
};

#ifdef CONFIG_GPS
const char *settings_gps_items[] =
{
    "GPS Enabled",
    "GPS Set Time",
    "UTC Timezone"
};
#endif

const char *info_items[] =
{
    "",
    "Used heap",
    "Hw Version",
    "HMI",
    "BB Tuning Pot",
};

const char *authors[] =
{
    "Niccolo' IU2KIN",
    "Silvano IU2KWO",
    "Federico IU2NUO",
    "Mathis DB9MAT",
    "Morgan ON4MOD",
    "Marco DM4RCO"
};

static const char symbols_callsign[] = "_ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890/-.";

// Calculate number of menu entries
const uint8_t menu_num = sizeof(menu_items)/sizeof(menu_items[0]);
const uint8_t settings_num = sizeof(settings_items)/sizeof(settings_items[0]);
const uint8_t display_num = sizeof(display_items)/sizeof(display_items[0]);
#ifdef CONFIG_GPS
const uint8_t settings_gps_num = sizeof(settings_gps_items)/sizeof(settings_gps_items[0]);
#endif
const uint8_t m17_num = sizeof(m17_items)/sizeof(m17_items[0]);
const uint8_t module17_num = sizeof(module17_items)/sizeof(module17_items[0]);
const uint8_t info_num = sizeof(info_items)/sizeof(info_items[0]);
const uint8_t author_num = sizeof(authors)/sizeof(authors[0]);

const color_t color_black = {0, 0, 0, 255};
const color_t color_grey = {60, 60, 60, 255};
const color_t color_white = {255, 255, 255, 255};
const color_t yellow_fab413 = {250, 180, 19, 255};

layout_t layout;
state_t last_state;
static ui_state_t ui_state;
static bool layout_ready = false;

// UI event queue
static uint8_t evQueue_rdPos;
static uint8_t evQueue_wrPos;
static event_t evQueue[MAX_NUM_EVENTS];

static layout_t _ui_calculateLayout()
{
    // Horizontal line height
    const uint16_t hline_h = 1;
    // Compensate for fonts printing below the start position
    const uint16_t text_v_offset = 1;

    // Calculate UI layout for the Module 17

    // Height and padding shown in diagram at beginning of file
    const uint16_t top_h = 11;
    const uint16_t top_pad = 1;
    const uint16_t line1_h = 10;
    const uint16_t line2_h = 10;
    const uint16_t line3_h = 10;
    const uint16_t line4_h = 10;
    const uint16_t line5_h = 10;
    const uint16_t menu_h = 10;
    const uint16_t bottom_h = 15;
    const uint16_t bottom_pad = 0;
    const uint16_t status_v_pad = 1;
    const uint16_t small_line_v_pad = 1;
    const uint16_t big_line_v_pad = 0;
    const uint16_t horizontal_pad = 4;

    // Top bar font: 6 pt
    const fontSize_t   top_font = FONT_SIZE_6PT;
    const symbolSize_t top_symbol_size = SYMBOLS_SIZE_6PT;
    // Middle line fonts: 5, 8, 8 pt
    const fontSize_t line1_font = FONT_SIZE_6PT;
    const symbolSize_t line1_symbol_size = SYMBOLS_SIZE_6PT;
    const fontSize_t line2_font = FONT_SIZE_6PT;
    const symbolSize_t line2_symbol_size = SYMBOLS_SIZE_6PT;
    const fontSize_t line3_font = FONT_SIZE_6PT;
    const symbolSize_t line3_symbol_size = SYMBOLS_SIZE_6PT;
    const fontSize_t line4_font = FONT_SIZE_6PT;
    const symbolSize_t line4_symbol_size = SYMBOLS_SIZE_6PT;
    const fontSize_t line5_font = FONT_SIZE_6PT;
    const symbolSize_t line5_symbol_size = SYMBOLS_SIZE_6PT;
    // Bottom bar font: 6 pt
    const fontSize_t bottom_font = FONT_SIZE_6PT;
    // TimeDate/Frequency input font
    const fontSize_t input_font = FONT_SIZE_8PT;
    // Menu font
    const fontSize_t menu_font = FONT_SIZE_6PT;
    // Mode screen frequency font: 9 pt
    const fontSize_t mode_font_big = FONT_SIZE_9PT;
    // Mode screen details font: 6 pt
    const fontSize_t mode_font_small = FONT_SIZE_6PT;

    // Calculate printing positions
    point_t top_pos    = {horizontal_pad, top_h - status_v_pad - text_v_offset};
    point_t line1_pos  = {horizontal_pad, top_h + top_pad + line1_h - small_line_v_pad - text_v_offset};
    point_t line2_pos  = {horizontal_pad, top_h + top_pad + line1_h + line2_h - small_line_v_pad - text_v_offset};
    point_t line3_pos  = {horizontal_pad, top_h + top_pad + line1_h + line2_h + line3_h - big_line_v_pad - text_v_offset};
    point_t line4_pos  = {horizontal_pad, top_h + top_pad + line1_h + line2_h + line3_h + line4_h - big_line_v_pad - text_v_offset};
    point_t line5_pos  = {horizontal_pad, top_h + top_pad + line1_h + line2_h + line3_h + line4_h + line5_h - big_line_v_pad - text_v_offset};
    point_t bottom_pos = {horizontal_pad, CONFIG_SCREEN_HEIGHT - bottom_pad - status_v_pad - text_v_offset};

    layout_t new_layout =
    {
        hline_h,
        top_h,
        line1_h,
        line2_h,
        line3_h,
        line4_h,
        line5_h,
        menu_h,
        bottom_h,
        bottom_pad,
        status_v_pad,
        horizontal_pad,
        text_v_offset,
        top_pos,
        line1_pos,
        line2_pos,
        line3_pos,
        line4_pos,
        line5_pos,
        bottom_pos,
        top_font,
        top_symbol_size,
        line1_font,
        line1_symbol_size,
        line2_font,
        line2_symbol_size,
        line3_font,
        line3_symbol_size,
        line4_font,
        line4_symbol_size,
        line5_font,
        line5_symbol_size,
        bottom_font,
        input_font,
        menu_font,
        mode_font_big,
        mode_font_small
    };
    return new_layout;
}


void ui_init()
{
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

    point_t origin = {0, (CONFIG_SCREEN_HEIGHT / 2) - 6};
    gfx_print(origin, FONT_SIZE_12PT, TEXT_ALIGN_CENTER, yellow_fab413, "O P N\nR T X");
}

#ifdef CONFIG_RTC
void _ui_timedate_add_digit(datetime_t *timedate, uint8_t pos, uint8_t number)
{
    switch(pos)
    {
        // Set date
        case 1:
            timedate->date += number * 10;
            break;
        case 2:
            timedate->date += number;
            break;
        // Set month
        case 3:
            timedate->month += number * 10;
            break;
        case 4:
            timedate->month += number;
            break;
        // Set year
        case 5:
            timedate->year += number * 10;
            break;
        case 6:
            timedate->year += number;
            break;
        // Set hour
        case 7:
            timedate->hour += number * 10;
            break;
        case 8:
            timedate->hour += number;
            break;
        // Set minute
        case 9:
            timedate->minute += number * 10;
            break;
        case 10:
            timedate->minute += number;
            break;
    }
}
#endif

static void _ui_changeBrightness(int variation)
{
    // Avoid rollover if current value is zero.
    if((state.settings.brightness == 0) && (variation < 0))
        return;

    // Cap max brightness to 100
    if((state.settings.brightness == 100) && (variation > 0))
        return;

    state.settings.brightness += variation;
    display_setBacklightLevel(state.settings.brightness);
}

static void _ui_changeCAN(int variation)
{
    int8_t can = state.settings.m17_can + variation;

    // M17 CAN ranges from 0 to 15
    if(can > 15)
        can = 0;

    if(can < 0)
        can = 15;

    state.settings.m17_can = can;
}

static void _ui_changeWiper(uint16_t *wiper, int variation)
{
    uint16_t value = *wiper;
    value         += variation;

    // Max value for softpot is 0x100, min value is set to 0x001
    if(value > 0x100)
        value = 0x100;

    if(value < 0x001)
        value = 0x001;

    *wiper = value;
}

static void _ui_changeMicGain(int variation)
{
    int8_t gain = mod17CalData.mic_gain + variation;

    if(gain > 2)
        gain = 0;

    if(gain < 0)
        gain = 2;

    mod17CalData.mic_gain = gain;
}

static void _ui_menuUp(uint8_t menu_entries)
{
    uint8_t maxEntries = menu_entries - 1;
    const hwInfo_t* hwinfo = platform_getHwInfo();

    // Hide the "shutdown" main menu entry for versions lower than 0.1e
    if((hwinfo->hw_version < 1) && (state.ui_screen == MENU_TOP))
        maxEntries -= 1;

    // Hide the softpot menu entries if hardware does not have them
    uint8_t softpot = hwinfo->flags & MOD17_FLAGS_SOFTPOT;
    if((softpot == 0) && (state.ui_screen == SETTINGS_MODULE17))
        maxEntries -= 2;

    if(ui_state.menu_selected > 0)
        ui_state.menu_selected -= 1;
    else
        ui_state.menu_selected = maxEntries;
}

static void _ui_menuDown(uint8_t menu_entries)
{
   uint8_t maxEntries = menu_entries - 1;
   const hwInfo_t* hwinfo = platform_getHwInfo();

    // Hide the "shutdown" main menu entry for versions lower than 0.1e
    if((hwinfo->hw_version < 1) && (state.ui_screen == MENU_TOP))
        maxEntries -= 1;

    // Hide the softpot menu entries if hardware does not have them
    uint8_t softpot = hwinfo->flags & MOD17_FLAGS_SOFTPOT;
    if((softpot == 0) && (state.ui_screen == SETTINGS_MODULE17))
        maxEntries -= 2;

    if(ui_state.menu_selected < maxEntries)
        ui_state.menu_selected += 1;
    else
        ui_state.menu_selected = 0;
}

static void _ui_menuBack(uint8_t prev_state)
{
    if(ui_state.edit_mode)
    {
        ui_state.edit_mode = false;
    }
    else
    {
        // Return to previous menu
        state.ui_screen = prev_state;
        // Reset menu selection
        ui_state.menu_selected = 0;
    }
}

static void _ui_textInputReset(char *buf)
{
    ui_state.input_number = 0;
    ui_state.input_position = 0;
    ui_state.input_set = 0;
    ui_state.last_keypress = 0;
    memset(buf, 0, 9);
}

static void _ui_textInputArrows(char *buf, uint8_t max_len, kbd_msg_t msg)
{
    if(ui_state.input_position >= max_len)
        return;

    uint8_t num_symbols = 0;
    num_symbols = strlen(symbols_callsign);

    if (msg.keys & KEY_RIGHT)
    {
        if (ui_state.input_position < (max_len - 1))
        {
            ui_state.input_position = ui_state.input_position + 1;
            ui_state.input_set = 0;
        }
    }
    else if (msg.keys & KEY_LEFT)
    {
        if (ui_state.input_position > 0)
        {
            buf[ui_state.input_position] = '\0';
            ui_state.input_position = ui_state.input_position - 1;
        }

        // get index of current selected character in symbol table
        ui_state.input_set = strcspn(symbols_callsign, &buf[ui_state.input_position]);
    }
    else if (msg.keys & KEY_UP)
        ui_state.input_set = (ui_state.input_set + 1) % num_symbols;
    else if (msg.keys & KEY_DOWN)
        ui_state.input_set = ui_state.input_set==0 ? num_symbols-1 : ui_state.input_set-1;

    buf[ui_state.input_position] = symbols_callsign[ui_state.input_set];
}

static void _ui_textInputConfirm(char *buf)
{
    buf[ui_state.input_position + 1] = '\0';
}

void ui_saveState()
{
    last_state = state;
}

void ui_updateFSM(bool *sync_rtx)
{
    // Check for events
    if(evQueue_wrPos == evQueue_rdPos) return;

    // Pop an event from the queue
    uint8_t newTail = (evQueue_rdPos + 1) % MAX_NUM_EVENTS;
    event_t event   = evQueue[evQueue_rdPos];
    evQueue_rdPos   = newTail;

    // Process pressed keys
    if(event.type == EVENT_KBD)
    {
        kbd_msg_t msg;
        msg.value = event.payload;

        switch(state.ui_screen)
        {
            // VFO screen
            case MAIN_VFO:
                if(ui_state.edit_mode)
                {
                    if(msg.keys & KEY_ENTER)
                    {
                        _ui_textInputConfirm(ui_state.new_callsign);
                        // Save selected callsign and disable input mode
                        strncpy(state.settings.m17_dest, ui_state.new_callsign, 10);
                        *sync_rtx = true;
                        ui_state.edit_mode = false;
                    }
                    else if(msg.keys & KEY_ESC)
                        ui_state.edit_mode = false;
                    else
                        _ui_textInputArrows(ui_state.new_callsign, 9, msg);
                }
                else
                {
                    if(msg.keys & KEY_ENTER)
                    {
                        // Save current main state
                        ui_state.last_main_state = state.ui_screen;
                        // Open Menu
                        state.ui_screen = MENU_TOP;
                    }
                    else if (msg.keys & KEY_RIGHT)
                    {
                        ui_state.edit_mode = true;
                    }
                }
                break;

            // Top menu screen
            case MENU_TOP:
                if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(menu_num);
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(menu_num);
                else if(msg.keys & KEY_ENTER)
                {
                    switch(ui_state.menu_selected)
                    {
                        case M_SETTINGS:
                            state.ui_screen = MENU_SETTINGS;
                            break;
                        case M_INFO:
                            state.ui_screen = MENU_INFO;
                            break;
                        case M_ABOUT:
                            state.ui_screen = MENU_ABOUT;
                            break;
                        case M_SHUTDOWN:
                            state.devStatus = SHUTDOWN;
                            break;
                    }
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                }
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(ui_state.last_main_state);
                break;

            // Settings menu screen
            case MENU_SETTINGS:
                if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(settings_num);
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(settings_num);
                else if(msg.keys & KEY_ENTER)
                {

                    switch(ui_state.menu_selected)
                    {
                        case S_DISPLAY:
                            state.ui_screen = SETTINGS_DISPLAY;
                            break;
                        case S_M17:
                            state.ui_screen = SETTINGS_M17;
                            break;
                        case S_MOD17:
                            state.ui_screen = SETTINGS_MODULE17;
                            break;
                        case S_RESET2DEFAULTS:
                            state.ui_screen = SETTINGS_RESET2DEFAULTS;
                            break;
                        default:
                            state.ui_screen = MENU_SETTINGS;
                    }
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                }
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
            // Info menu screen
            case MENU_INFO:
                if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(info_num);
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(info_num);
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
            // About screen, scroll without rollover
            case MENU_ABOUT:
                if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                {
                    if(ui_state.menu_selected > 0)
                        ui_state.menu_selected -= 1;
                }
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    ui_state.menu_selected += 1;
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;

            case SETTINGS_DISPLAY:
                if(msg.keys & KEY_LEFT)
                {
                    switch(ui_state.menu_selected)
                    {
                        case D_BRIGHTNESS:
                            _ui_changeBrightness(-5);
                            break;
                        default:
                            state.ui_screen = SETTINGS_DISPLAY;
                    }
                }
                else if(msg.keys & KEY_RIGHT)
                {
                    switch(ui_state.menu_selected)
                    {
                        case D_BRIGHTNESS:
                            _ui_changeBrightness(+5);
                            break;
                        default:
                            state.ui_screen = SETTINGS_DISPLAY;
                    }
                }
                else if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(display_num);
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(display_num);
                else if(msg.keys & KEY_ESC)
                    {
                        nvm_writeSettings(&state.settings);
                        _ui_menuBack(MENU_SETTINGS);
                    }
                break;

            // M17 Settings
            case SETTINGS_M17:

                if(ui_state.edit_mode)
                {
                    if(msg.keys & KEY_ENTER)
                    {
                        _ui_textInputConfirm(ui_state.new_callsign);
                        // Save selected callsign and disable input mode
                        strncpy(state.settings.callsign, ui_state.new_callsign, 10);
                        ui_state.edit_mode = false;
                    }
                    else if(msg.keys & KEY_ESC)
                        ui_state.edit_mode = false;
                    else
                        _ui_textInputArrows(ui_state.new_callsign, 9, msg);
                }
                else
                {
                    // Not in edit mode: handle CAN setting
                    if(msg.keys & KEY_LEFT)
                    {
                        switch(ui_state.menu_selected)
                        {
                            case M_CAN:
                                _ui_changeCAN(-1);
                                break;
                            case M_CAN_RX:
                                state.settings.m17_can_rx = !state.settings.m17_can_rx;
                                break;
                            default:
                                state.ui_screen = SETTINGS_M17;
                        }
                    }
                    else if(msg.keys & KEY_RIGHT)
                    {
                        switch(ui_state.menu_selected)
                        {
                            case M_CAN:
                                _ui_changeCAN(+1);
                                break;
                            case M_CAN_RX:
                                state.settings.m17_can_rx = !state.settings.m17_can_rx;
                                break;
                            default:
                                state.ui_screen = SETTINGS_M17;
                        }
                    }
                    else if(msg.keys & KEY_ENTER)
                    {
                        switch(ui_state.menu_selected)
                        {
                            // Enable callsign input
                            case M_CALLSIGN:
                                ui_state.edit_mode = true;
                                _ui_textInputReset(ui_state.new_callsign);
                                break;
                            default:
                                state.ui_screen = SETTINGS_M17;
                        }
                    }
                    else if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                        _ui_menuUp(m17_num);
                    else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                        _ui_menuDown(m17_num);
                    else if(msg.keys & KEY_ESC)
                    {
                        *sync_rtx = true;
                        nvm_writeSettings(&state.settings);
                        _ui_menuBack(MENU_SETTINGS);
                    }
                }
                break;
            case SETTINGS_RESET2DEFAULTS:
                if(! ui_state.edit_mode)
                {
                    //require a confirmation ENTER, then another
                    //edit_mode is slightly misused to allow for this
                    if(msg.keys & KEY_ENTER)
                    {
                        ui_state.edit_mode = true;
                    }
                    else if(msg.keys & KEY_ESC)
                    {
                        _ui_menuBack(MENU_SETTINGS);
                    }
                }
                else
                {
                    if(msg.keys & KEY_ENTER)
                    {
                        ui_state.edit_mode = false;

                        // Reset calibration values
                        mod17CalData.tx_wiper     = 0x080;
                        mod17CalData.rx_wiper     = 0x080;
                        mod17CalData.bb_tx_invert = 0;
                        mod17CalData.bb_rx_invert = 0;
                        mod17CalData.mic_gain     = 0;

                        state_resetSettingsAndVfo();
                        nvm_writeSettings(&state.settings);
                        _ui_menuBack(MENU_SETTINGS);
                    }
                    else if(msg.keys & KEY_ESC)
                    {
                        ui_state.edit_mode = false;
                        _ui_menuBack(MENU_SETTINGS);
                    }
                }
                break;
            // Module17 Settings
            case SETTINGS_MODULE17:
                if(msg.keys & KEY_LEFT)
                {
                    switch(ui_state.menu_selected)
                    {
                        case D_TXWIPER:
                            _ui_changeWiper(&mod17CalData.tx_wiper, -1);
                            break;
                        case D_RXWIPER:
                            _ui_changeWiper(&mod17CalData.rx_wiper, -1);
                            break;
                        case D_TXINVERT:
                            mod17CalData.bb_tx_invert -= 1;
                            break;
                        case D_RXINVERT:
                            mod17CalData.bb_rx_invert -= 1;
                            break;
                        case D_MICGAIN:
                            _ui_changeMicGain(-1);
                            break;
                        case D_PTTINLEVEL:
                            mod17CalData.ptt_in_level -= 1;
                            break;
                        case D_PTTOUTLEVEL:
                            mod17CalData.ptt_out_level -= 1;
                            break;
                        default:
                            state.ui_screen = SETTINGS_MODULE17;
                    }
                }
                else if(msg.keys & KEY_RIGHT)
                {
                    switch(ui_state.menu_selected)
                    {
                        case D_TXWIPER:
                            _ui_changeWiper(&mod17CalData.tx_wiper, +1);
                            break;
                        case D_RXWIPER:
                            _ui_changeWiper(&mod17CalData.rx_wiper, +1);
                            break;
                        case D_TXINVERT:
                            mod17CalData.bb_tx_invert += 1;
                            break;
                        case D_RXINVERT:
                            mod17CalData.bb_rx_invert += 1;
                            break;
                        case D_MICGAIN:
                            _ui_changeMicGain(+1);
                            break;
                        case D_PTTINLEVEL:
                            mod17CalData.ptt_in_level += 1;
                            break;
                        case D_PTTOUTLEVEL:
                            mod17CalData.ptt_out_level += 1;
                            break;
                        default:
                            state.ui_screen = SETTINGS_MODULE17;
                    }
                }
                else if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(module17_num);
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(module17_num);
                else if(msg.keys & KEY_ESC)
                {
                    nvm_writeSettings(&state.settings);
                    _ui_menuBack(MENU_SETTINGS);
                }
                break;
        }
    }
}

bool ui_updateGUI()
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
        case MAIN_VFO:
            _ui_drawMainVFO(&ui_state);
            break;
        // Top menu screen
        case MENU_TOP:
            _ui_drawMenuTop(&ui_state);
            break;
        // Settings menu screen
        case MENU_SETTINGS:
            _ui_drawMenuSettings(&ui_state);
            break;
        // Info menu screen
        case MENU_INFO:
            _ui_drawMenuInfo(&ui_state);
            break;
        // About menu screen
        case MENU_ABOUT:
            _ui_drawMenuAbout(&ui_state);
            break;
        // Display settings screen
        case SETTINGS_DISPLAY:
            _ui_drawSettingsDisplay(&ui_state);
            break;
        // M17 settings screen
        case SETTINGS_M17:
            _ui_drawSettingsM17(&ui_state);
            break;
        // Module 17 settings screen
        case SETTINGS_MODULE17:
            _ui_drawSettingsModule17(&ui_state);
            break;
        // Screen to support resetting Settings and VFO to defaults
        case SETTINGS_RESET2DEFAULTS:
            _ui_drawSettingsReset2Defaults(&ui_state);
            break;
    }

    return true;
}

bool ui_pushEvent(const uint8_t type, const uint32_t data)
{
    uint8_t newHead = (evQueue_wrPos + 1) % MAX_NUM_EVENTS;

    // Queue is full
    if(newHead == evQueue_rdPos) return false;

    // Preserve atomicity when writing the new element into the queue.
    event_t event;
    event.type    = type;
    event.payload = data;

    evQueue[evQueue_wrPos] = event;
    evQueue_wrPos = newHead;

    return true;
}

void ui_terminate()
{
}
