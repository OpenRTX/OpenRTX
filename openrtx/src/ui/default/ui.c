/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
 *      │  top_status_bar (16px)  │  8 pt (11 px) font with 2 px vertical padding
 *      │      top_pad (4px)      │  4 px padding
 *      │      Line 1 (20px)      │  8 pt (11 px) font with 4 px vertical padding
 *      │      Line 2 (20px)      │  8 pt (11 px) font with 4 px vertical padding
 *      │                         │
 *      │      Line 3 (40px)      │  16 pt (xx px) font with 6 px vertical padding
 *      │ RSSI+squelch bar (20px) │  20 px
 *      │      bottom_pad (4px)   │  4 px padding
 *      └─────────────────────────┘
 *
 *         128x64 display (GD-77)
 *      ┌─────────────────────────┐
 *      │  top_status_bar (11 px) │  6 pt (9 px) font with 1 px vertical padding
 *      │      top_pad (1px)      │  1 px padding
 *      │      Line 1 (10px)      │  6 pt (9 px) font without vertical padding
 *      │      Line 2 (10px)      │  6 pt (9 px) font with 2 px vertical padding
 *      │      Line 3 (18px)      │  12 pt (xx px) font with 0 px vertical padding
 *      │ RSSI+squelch bar (11px) │  11 px
 *      │      bottom_pad (1px)   │  1 px padding
 *      └─────────────────────────┘
 *
 *         128x48 display (RD-5R)
 *      ┌─────────────────────────┐
 *      │  top_status_bar (11 px) │  6 pt (9 px) font with 1 px vertical padding
 *      ├─────────────────────────┤  1 px line
 *      │      Line 2 (10px)      │  8 pt (11 px) font with 4 px vertical padding
 *      │      Line 3 (18px)      │  8 pt (11 px) font with 4 px vertical padding
 *      └─────────────────────────┘
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <ui/ui_default.h>
#include <rtx.h>
#include <interfaces/platform.h>
#include <interfaces/display.h>
#include <interfaces/cps_io.h>
#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include <string.h>
#include <battery.h>
#include <input.h>
#include <utils.h>
#include <hwconfig.h>
#include <voicePromptUtils.h>
#include <beeps.h>

/* UI main screen functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainBackground();
extern void _ui_drawMainTop(ui_state_t* ui_state);
extern void _ui_drawVFOMiddle();
extern void _ui_drawMEMMiddle();
extern void _ui_drawVFOBottom();
extern void _ui_drawMEMBottom();
extern void _ui_drawMainVFO(ui_state_t* ui_state);
extern void _ui_drawMainVFOInput(ui_state_t* ui_state);
extern void _ui_drawMainMEM(ui_state_t* ui_state);
/* UI menu functions, their implementation is in "ui_menu.c" */
extern void _ui_drawMenuTop(ui_state_t* ui_state);
extern void _ui_drawMenuBank(ui_state_t* ui_state);
extern void _ui_drawMenuChannel(ui_state_t* ui_state);
extern void _ui_drawMenuContacts(ui_state_t* ui_state);
#ifdef CONFIG_GPS
extern void _ui_drawMenuGPS();
extern void _ui_drawSettingsGPS(ui_state_t* ui_state);
#endif
extern void _ui_drawSettingsAccessibility(ui_state_t* ui_state);
extern void _ui_drawMenuSettings(ui_state_t* ui_state);
extern void _ui_drawMenuBackupRestore(ui_state_t* ui_state);
extern void _ui_drawMenuBackup(ui_state_t* ui_state);
extern void _ui_drawMenuRestore(ui_state_t* ui_state);
extern void _ui_drawMenuInfo(ui_state_t* ui_state);
extern void _ui_drawMenuAbout();
#ifdef CONFIG_RTC
extern void _ui_drawSettingsTimeDate();
extern void _ui_drawSettingsTimeDateSet(ui_state_t* ui_state);
#endif
extern void _ui_drawSettingsDisplay(ui_state_t* ui_state);
extern void _ui_drawSettingsM17(ui_state_t* ui_state);
extern void _ui_drawSettingsVoicePrompts(ui_state_t* ui_state);
extern void _ui_drawSettingsReset2Defaults(ui_state_t* ui_state);
extern void _ui_drawSettingsRadio(ui_state_t* ui_state);
extern bool _ui_drawMacroMenu(ui_state_t* ui_state);
extern void _ui_reset_menu_anouncement_tracking();

const char *menu_items[] =
{
    "Banks",
    "Channels",
    "Contacts",
#ifdef CONFIG_GPS
    "GPS",
#endif
    "Settings",
    "Info",
    "About"
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
    "Radio",
#ifdef CONFIG_M17
    "M17",
#endif
    "Accessibility",
    "Default Settings"
};

const char *display_items[] =
{
#ifdef CONFIG_SCREEN_BRIGHTNESS
    "Brightness",
#endif
#ifdef CONFIG_SCREEN_CONTRAST
    "Contrast",
#endif
    "Timer"
};

#ifdef CONFIG_GPS
const char *settings_gps_items[] =
{
    "GPS Enabled",
    "GPS Set Time",
    "UTC Timezone"
};
#endif

const char *settings_radio_items[] =
{
    "Offset",
    "Direction",
    "Step",
};

const char * settings_m17_items[] =
{
    "Callsign",
    "CAN",
    "CAN RX Check"
};

const char * settings_accessibility_items[] =
{
    "Macro Latch",
    "Voice",
    "Phonetic"
};

const char *backup_restore_items[] =
{
    "Backup",
    "Restore"
};

const char *info_items[] =
{
    "",
    "Bat. Voltage",
    "Bat. Charge",
    "RSSI",
    "Used heap",
    "Band",
    "VHF",
    "UHF",
    "Hw Version",
#ifdef PLATFORM_TTWRPLUS
    "Radio",
    "Radio FW",
#endif
};

const char *authors[] =
{
    "Niccolo' IU2KIN",
    "Silvano IU2KWO",
    "Federico IU2NUO",
    "Fred IU2NRO",
    "Joseph VK7JS",
    "Morgan ON4MOD",
    "Marco DM4RCO"
};

static const char *symbols_ITU_T_E161[] =
{
    " 0",
    ",.?1",
    "abc2ABC",
    "def3DEF",
    "ghi4GHI",
    "jkl5JKL",
    "mno6MNO",
    "pqrs7PQRS",
    "tuv8TUV",
    "wxyz9WXYZ",
    "-/*",
    "#"
};

static const char *symbols_ITU_T_E161_callsign[] =
{
    "0 ",
    "1",
    "ABC2",
    "DEF3",
    "GHI4",
    "JKL5",
    "MNO6",
    "PQRS7",
    "TUV8",
    "WXYZ9",
    "-/",
    ""
};

// Calculate number of menu entries
const uint8_t menu_num = sizeof(menu_items)/sizeof(menu_items[0]);
const uint8_t settings_num = sizeof(settings_items)/sizeof(settings_items[0]);
const uint8_t display_num = sizeof(display_items)/sizeof(display_items[0]);
#ifdef CONFIG_GPS
const uint8_t settings_gps_num = sizeof(settings_gps_items)/sizeof(settings_gps_items[0]);
#endif
const uint8_t settings_radio_num = sizeof(settings_radio_items)/sizeof(settings_radio_items[0]);
#ifdef CONFIG_M17
const uint8_t settings_m17_num = sizeof(settings_m17_items)/sizeof(settings_m17_items[0]);
#endif
const uint8_t settings_accessibility_num = sizeof(settings_accessibility_items)/sizeof(settings_accessibility_items[0]);
const uint8_t backup_restore_num = sizeof(backup_restore_items)/sizeof(backup_restore_items[0]);
const uint8_t info_num = sizeof(info_items)/sizeof(info_items[0]);
const uint8_t author_num = sizeof(authors)/sizeof(authors[0]);

const color_t color_black = {0, 0, 0, 255};
const color_t color_grey = {60, 60, 60, 255};
const color_t color_white = {255, 255, 255, 255};
const color_t yellow_fab413 = {250, 180, 19, 255};

layout_t layout;
state_t last_state;
bool macro_latched;
static ui_state_t ui_state;
static bool macro_menu = false;
static bool layout_ready = false;
static bool redraw_needed = true;

static bool standby = false;
static long long last_event_tick = 0;

// UI event queue
static uint8_t evQueue_rdPos;
static uint8_t evQueue_wrPos;
static event_t evQueue[MAX_NUM_EVENTS];


static void _ui_calculateLayout(layout_t *layout)
{
    // Horizontal line height
    static const uint16_t hline_h = 1;
    // Compensate for fonts printing below the start position
    static const uint16_t text_v_offset = 1;

    // Calculate UI layout depending on vertical resolution
    // Tytera MD380, MD-UV380
    #if CONFIG_SCREEN_HEIGHT > 127

    // Height and padding shown in diagram at beginning of file
    static const uint16_t top_h = 16;
    static const uint16_t top_pad = 4;
    static const uint16_t line1_h = 20;
    static const uint16_t line2_h = 20;
    static const uint16_t line3_h = 20;
    static const uint16_t line3_large_h = 40;
    static const uint16_t line4_h = 20;
    static const uint16_t menu_h = 16;
    static const uint16_t bottom_h = 23;
    static const uint16_t bottom_pad = top_pad;
    static const uint16_t status_v_pad = 2;
    static const uint16_t small_line_v_pad = 2;
    static const uint16_t big_line_v_pad = 6;
    static const uint16_t horizontal_pad = 4;

    // Top bar font: 8 pt
    static const fontSize_t   top_font = FONT_SIZE_8PT;
    static const symbolSize_t top_symbol_size = SYMBOLS_SIZE_8PT;
    // Text line font: 8 pt
    static const fontSize_t line1_font = FONT_SIZE_8PT;
    static const symbolSize_t line1_symbol_size = SYMBOLS_SIZE_8PT;
    static const fontSize_t line2_font = FONT_SIZE_8PT;
    static const symbolSize_t line2_symbol_size = SYMBOLS_SIZE_8PT;
    static const fontSize_t line3_font = FONT_SIZE_8PT;
    static const symbolSize_t line3_symbol_size = SYMBOLS_SIZE_8PT;
    static const fontSize_t line4_font = FONT_SIZE_8PT;
    static const symbolSize_t line4_symbol_size = SYMBOLS_SIZE_8PT;
    // Frequency line font: 16 pt
    static const fontSize_t line3_large_font = FONT_SIZE_16PT;
    // Bottom bar font: 8 pt
    static const fontSize_t bottom_font = FONT_SIZE_8PT;
    // TimeDate/Frequency input font
    static const fontSize_t input_font = FONT_SIZE_12PT;
    // Menu font
    static const fontSize_t menu_font = FONT_SIZE_8PT;

    // Radioddity GD-77
    #elif CONFIG_SCREEN_HEIGHT > 63

    // Height and padding shown in diagram at beginning of file
    static const uint16_t top_h = 11;
    static const uint16_t top_pad = 1;
    static const uint16_t line1_h = 10;
    static const uint16_t line2_h = 10;
    static const uint16_t line3_h = 10;
    static const uint16_t line3_large_h = 16;
    static const uint16_t line4_h = 10;
    static const uint16_t menu_h = 10;
    static const uint16_t bottom_h = 15;
    static const uint16_t bottom_pad = 0;
    static const uint16_t status_v_pad = 1;
    static const uint16_t small_line_v_pad = 1;
    static const uint16_t big_line_v_pad = 0;
    static const uint16_t horizontal_pad = 4;

    // Top bar font: 6 pt
    static const fontSize_t   top_font = FONT_SIZE_6PT;
    static const symbolSize_t top_symbol_size = SYMBOLS_SIZE_6PT;
    // Middle line fonts: 5, 8, 8 pt
    static const fontSize_t line1_font = FONT_SIZE_6PT;
    static const symbolSize_t line1_symbol_size = SYMBOLS_SIZE_6PT;
    static const fontSize_t line2_font = FONT_SIZE_6PT;
    static const symbolSize_t line2_symbol_size = SYMBOLS_SIZE_6PT;
    static const fontSize_t line3_font = FONT_SIZE_6PT;
    static const symbolSize_t line3_symbol_size = SYMBOLS_SIZE_6PT;
    static const fontSize_t line3_large_font = FONT_SIZE_10PT;
    static const fontSize_t line4_font = FONT_SIZE_6PT;
    static const symbolSize_t line4_symbol_size = SYMBOLS_SIZE_6PT;
    // Bottom bar font: 6 pt
    static const fontSize_t bottom_font = FONT_SIZE_6PT;
    // TimeDate/Frequency input font
    static const fontSize_t input_font = FONT_SIZE_8PT;
    // Menu font
    static const fontSize_t menu_font = FONT_SIZE_6PT;

    // Radioddity RD-5R
    #elif CONFIG_SCREEN_HEIGHT > 47

    // Height and padding shown in diagram at beginning of file
    static const uint16_t top_h = 11;
    static const uint16_t top_pad = 1;
    static const uint16_t line1_h = 0;
    static const uint16_t line2_h = 10;
    static const uint16_t line3_h = 10;
    static const uint16_t line3_large_h = 18;
    static const uint16_t line4_h = 10;
    static const uint16_t menu_h = 10;
    static const uint16_t bottom_h = 0;
    static const uint16_t bottom_pad = 0;
    static const uint16_t status_v_pad = 1;
    static const uint16_t small_line_v_pad = 1;
    static const uint16_t big_line_v_pad = 0;
    static const uint16_t horizontal_pad = 4;

    // Top bar font: 6 pt
    static const fontSize_t   top_font = FONT_SIZE_6PT;
    static const symbolSize_t top_symbol_size = SYMBOLS_SIZE_6PT;
    // Middle line fonts: 16, 16
    static const fontSize_t line2_font = FONT_SIZE_6PT;
    static const fontSize_t line3_font = FONT_SIZE_6PT;
    static const fontSize_t line4_font = FONT_SIZE_6PT;
    static const fontSize_t line3_large_font = FONT_SIZE_12PT;
    // TimeDate/Frequency input font
    static const fontSize_t input_font = FONT_SIZE_8PT;
    // Menu font
    static const fontSize_t menu_font = FONT_SIZE_6PT;
    // Not present on this resolution
    static const fontSize_t line1_font = 0;
    static const fontSize_t bottom_font = 0;

    #else
    #error Unsupported vertical resolution!
    #endif

    // Calculate printing positions
    static const uint16_t top_pos   = top_h - status_v_pad - text_v_offset;
    static const uint16_t line1_pos = top_h + top_pad + line1_h - small_line_v_pad - text_v_offset;
    static const uint16_t line2_pos = top_h + top_pad + line1_h + line2_h - small_line_v_pad - text_v_offset;
    static const uint16_t line3_pos = top_h + top_pad + line1_h + line2_h + line3_h - small_line_v_pad - text_v_offset;
    static const uint16_t line4_pos = top_h + top_pad + line1_h + line2_h + line3_h + line4_h - small_line_v_pad - text_v_offset;
    static const uint16_t line3_large_pos = top_h + top_pad + line1_h + line2_h + line3_large_h - big_line_v_pad - text_v_offset;
    static const uint16_t bottom_pos = CONFIG_SCREEN_HEIGHT - bottom_pad - status_v_pad - text_v_offset;

    layout_t new_layout =
    {
        hline_h,
        top_h,
        line1_h,
        line2_h,
        line3_h,
        line3_large_h,
        line4_h,
        menu_h,
        bottom_h,
        bottom_pad,
        status_v_pad,
        horizontal_pad,
        text_v_offset,
        {horizontal_pad, top_pos},
        {horizontal_pad, line1_pos},
        {horizontal_pad, line2_pos},
        {horizontal_pad, line3_pos},
        {horizontal_pad, line3_large_pos},
        {horizontal_pad, line4_pos},
        {horizontal_pad, bottom_pos},
        top_font,
        top_symbol_size,
        line1_font,
        line1_symbol_size,
        line2_font,
        line2_symbol_size,
        line3_font,
        line3_symbol_size,
        line3_large_font,
        line4_font,
        line4_symbol_size,
        bottom_font,
        input_font,
        menu_font
    };

    memcpy(layout, &new_layout, sizeof(layout_t));
}

static void _ui_drawLowBatteryScreen()
{
    gfx_clearScreen();
    uint16_t bat_width = CONFIG_SCREEN_WIDTH / 2;
    uint16_t bat_height = CONFIG_SCREEN_HEIGHT / 3;
    point_t bat_pos = {CONFIG_SCREEN_WIDTH / 4, CONFIG_SCREEN_HEIGHT / 8};
    gfx_drawBattery(bat_pos, bat_width, bat_height, 10);
    point_t text_pos_1 = {0, CONFIG_SCREEN_HEIGHT * 2 / 3};
    point_t text_pos_2 = {0, CONFIG_SCREEN_HEIGHT * 2 / 3 + 16};

    gfx_print(text_pos_1,
              FONT_SIZE_6PT,
              TEXT_ALIGN_CENTER,
              color_white,
              currentLanguage->forEmergencyUse);
    gfx_print(text_pos_2,
              FONT_SIZE_6PT,
              TEXT_ALIGN_CENTER,
              color_white,
              currentLanguage->pressAnyButton);
}

static freq_t _ui_freq_add_digit(freq_t freq, uint8_t pos, uint8_t number)
{
    freq_t coefficient = 100;
    for(uint8_t i=0; i < FREQ_DIGITS - pos; i++)
    {
        coefficient *= 10;
    }
    return freq += number * coefficient;
}

#ifdef CONFIG_RTC
static void _ui_timedate_add_digit(datetime_t *timedate, uint8_t pos,
                                   uint8_t number)
{
    vp_flush();
    vp_queueInteger(number);
    if (pos == 2 || pos == 4)
        vp_queuePrompt(PROMPT_SLASH);
    // just indicates separation of date and time.
    if (pos==6) // start of time.
        vp_queueString("hh:mm", vpAnnounceCommonSymbols|vpAnnounceLessCommonSymbols);
    if (pos == 8)
        vp_queuePrompt(PROMPT_COLON);
    vp_play();

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

static bool _ui_freq_check_limits(freq_t freq)
{
    bool valid = false;
    const hwInfo_t* hwinfo = platform_getHwInfo();
    if(hwinfo->vhf_band)
    {
        // hwInfo_t frequencies are in MHz
        if(freq >= (hwinfo->vhf_minFreq * 1000000) &&
           freq <= (hwinfo->vhf_maxFreq * 1000000))
        valid = true;
    }
    if(hwinfo->uhf_band)
    {
        // hwInfo_t frequencies are in MHz
        if(freq >= (hwinfo->uhf_minFreq * 1000000) &&
           freq <= (hwinfo->uhf_maxFreq * 1000000))
        valid = true;
    }
    return valid;
}

static bool _ui_channel_valid(channel_t* channel)
{
return _ui_freq_check_limits(channel->rx_frequency) &&
       _ui_freq_check_limits(channel->tx_frequency);
}

static bool _ui_drawDarkOverlay()
{
    color_t alpha_grey = {0, 0, 0, 255};
    point_t origin = {0, 0};
    gfx_drawRect(origin, CONFIG_SCREEN_WIDTH, CONFIG_SCREEN_HEIGHT, alpha_grey, true);
    return true;
}

static int _ui_fsm_loadChannel(int16_t channel_index, bool *sync_rtx)
{
    channel_t channel;
    int32_t selected_channel = channel_index;
    // If a bank is active, get index from current bank
    if(state.bank_enabled)
    {
        bankHdr_t bank = { 0 };
        cps_readBankHeader(&bank, state.bank);
        if((channel_index < 0) || (channel_index >= bank.ch_count))
            return -1;
        channel_index = cps_readBankData(state.bank, channel_index);
    }

    int result = cps_readChannel(&channel, channel_index);
    // Read successful and channel is valid
    if((result != -1) && _ui_channel_valid(&channel))
    {
        // Set new channel index
        state.channel_index = selected_channel;
        // Copy channel read to state
        state.channel = channel;
        *sync_rtx = true;
    }

    return result;
}

static void _ui_fsm_confirmVFOInput(bool *sync_rtx)
{
    vp_flush();
    // Switch to TX input
    if(ui_state.input_set == SET_RX)
    {
        ui_state.input_set = SET_TX;
        // Reset input position
        ui_state.input_position = 0;
        // announce the rx frequency just confirmed with Enter.
        vp_queueFrequency(ui_state.new_rx_frequency);
        // defer playing till the end.
        // indicate that the user has moved to the tx freq field.
        vp_announceInputReceiveOrTransmit(true, vpqDefault);
    }
    else if(ui_state.input_set == SET_TX)
    {
        // Save new frequency setting
        // If TX frequency was not set, TX = RX
        if(ui_state.new_tx_frequency == 0)
        {
            ui_state.new_tx_frequency = ui_state.new_rx_frequency;
        }
        // Apply new frequencies if they are valid
        if(_ui_freq_check_limits(ui_state.new_rx_frequency) &&
           _ui_freq_check_limits(ui_state.new_tx_frequency))
        {
            state.channel.rx_frequency = ui_state.new_rx_frequency;
            state.channel.tx_frequency = ui_state.new_tx_frequency;
            *sync_rtx = true;
            // force init to clear any prompts in progress.
            // defer play because play is called at the end of the function
            //due to above freq queuing.
            vp_announceFrequencies(state.channel.rx_frequency,
                                   state.channel.tx_frequency, vpqInit);
        }
        else
        {
            vp_announceError(vpqInit);
        }

        state.ui_screen = MAIN_VFO;
    }

    vp_play();
}

static void _ui_fsm_insertVFONumber(kbd_msg_t msg, bool *sync_rtx)
{
    // Advance input position
    ui_state.input_position += 1;
    // clear any prompts in progress.
    vp_flush();
    // Save pressed number to calculate frequency and show in GUI
    ui_state.input_number = input_getPressedNumber(msg);
    // queue the digit just pressed.
    vp_queueInteger(ui_state.input_number);
    // queue  point if user has entered three digits.
    if (ui_state.input_position == 3)
        vp_queuePrompt(PROMPT_POINT);

    if(ui_state.input_set == SET_RX)
    {
        if(ui_state.input_position == 1)
            ui_state.new_rx_frequency = 0;
        // Calculate portion of the new RX frequency
        ui_state.new_rx_frequency = _ui_freq_add_digit(ui_state.new_rx_frequency,
                                                       ui_state.input_position,
                                                       ui_state.input_number);
        if(ui_state.input_position >= FREQ_DIGITS)
        {// queue the rx freq just completed.
            vp_queueFrequency(ui_state.new_rx_frequency);
            /// now queue tx as user has changed fields.
            vp_queuePrompt(PROMPT_TRANSMIT);
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
                                                       ui_state.input_position,
                                                       ui_state.input_number);
        if(ui_state.input_position >= FREQ_DIGITS)
        {
            // Save both inserted frequencies
            if(_ui_freq_check_limits(ui_state.new_rx_frequency) &&
               _ui_freq_check_limits(ui_state.new_tx_frequency))
            {
                state.channel.rx_frequency = ui_state.new_rx_frequency;
                state.channel.tx_frequency = ui_state.new_tx_frequency;
                *sync_rtx = true;
                // play is called at end.
                vp_announceFrequencies(state.channel.rx_frequency,
                                       state.channel.tx_frequency, vpqInit);
            }

            state.ui_screen = MAIN_VFO;
        }
    }

    vp_play();
}

#ifdef CONFIG_SCREEN_BRIGHTNESS
static void _ui_changeBrightness(int variation)
{
    state.settings.brightness += variation;

    // Max value for brightness is 100, min value is set to 5 to avoid complete
    //  display shutdown.
    if(state.settings.brightness > 100) state.settings.brightness = 100;
    if(state.settings.brightness < 5)   state.settings.brightness = 5;

    display_setBacklightLevel(state.settings.brightness);
}
#endif

#ifdef CONFIG_SCREEN_CONTRAST
static void _ui_changeContrast(int variation)
{
    if(variation >= 0)
        state.settings.contrast =
        (255 - state.settings.contrast < variation) ? 255 : state.settings.contrast + variation;
    else
        state.settings.contrast =
        (state.settings.contrast < -variation) ? 0 : state.settings.contrast + variation;

    display_setContrast(state.settings.contrast);
}
#endif

static void _ui_changeTimer(int variation)
{
    if ((state.settings.display_timer == TIMER_OFF && variation < 0) ||
        (state.settings.display_timer == TIMER_1H && variation > 0))
    {
        return;
    }

    state.settings.display_timer += variation;
}

static void _ui_changeMacroLatch(bool newVal)
{
    state.settings.macroMenuLatch = newVal ? 1 : 0;
    vp_announceSettingsOnOffToggle(&currentLanguage->macroLatching,
                                   vp_getVoiceLevelQueueFlags(),
                                   state.settings.macroMenuLatch);
}

#ifdef CONFIG_M17
static inline void _ui_changeM17Can(int variation)
{
    uint8_t can = state.settings.m17_can;
    state.settings.m17_can = (can + variation) % 16;
}
#endif

static void _ui_changeVoiceLevel(int variation)
{
    if ((state.settings.vpLevel == vpNone && variation < 0) ||
        (state.settings.vpLevel == vpHigh && variation > 0))
        {
            return;
        }

    state.settings.vpLevel += variation;

    // Force these flags to ensure the changes are spoken for levels 1 through 3.
    vpQueueFlags_t flags = vpqInit
                         | vpqAddSeparatingSilence
                         | vpqPlayImmediately;

    if (!vp_isPlaying())
    {
        flags |= vpqIncludeDescriptions;
    }

    vp_announceSettingsVoiceLevel(flags);
}

static void _ui_changePhoneticSpell(bool newVal)
{
    state.settings.vpPhoneticSpell = newVal ? 1 : 0;

    vp_announceSettingsOnOffToggle(&currentLanguage->phonetic,
                                   vp_getVoiceLevelQueueFlags(),
                                   state.settings.vpPhoneticSpell);
}

static bool _ui_checkStandby(long long time_since_last_event)
{
    if (standby)
    {
        return false;
    }

    switch (state.settings.display_timer)
    {
        case TIMER_OFF:
            return false;
        case TIMER_5S:
        case TIMER_10S:
        case TIMER_15S:
        case TIMER_20S:
        case TIMER_25S:
        case TIMER_30S:
            return time_since_last_event >= (5000 * state.settings.display_timer);
        case TIMER_1M:
        case TIMER_2M:
        case TIMER_3M:
        case TIMER_4M:
        case TIMER_5M:
            return time_since_last_event >=
                (60000 * (state.settings.display_timer - (TIMER_1M - 1)));
        case TIMER_15M:
        case TIMER_30M:
        case TIMER_45M:
            return time_since_last_event >=
                (60000 * 15 * (state.settings.display_timer - (TIMER_15M - 1)));
        case TIMER_1H:
            return time_since_last_event >= 60 * 60 * 1000;
    }

    // unreachable code
    return false;
}

static void _ui_enterStandby()
{
    if(standby)
        return;

    standby = true;
    redraw_needed = false;
    display_setBacklightLevel(0);
}

static bool _ui_exitStandby(long long now)
{
    last_event_tick = now;

    if(!standby)
        return false;

    standby = false;
    redraw_needed = true;
    display_setBacklightLevel(state.settings.brightness);

    return true;
}

static void _ui_fsm_menuMacro(kbd_msg_t msg, bool *sync_rtx)
{
    // If there is no keyboard left and right select the menu entry to edit
#if defined(CONFIG_UI_NO_KEYBOARD)
    if (msg.keys & KNOB_LEFT)
    {
        ui_state.macro_menu_selected--;
        ui_state.macro_menu_selected += 9;
        ui_state.macro_menu_selected %= 9;
    }
    if (msg.keys & KNOB_RIGHT)
    {
        ui_state.macro_menu_selected++;
        ui_state.macro_menu_selected %= 9;
    }
    if ((msg.keys & KEY_ENTER) && !msg.long_press)
        ui_state.input_number = ui_state.macro_menu_selected + 1;
    else
        ui_state.input_number = 0;
#else // CONFIG_UI_NO_KEYBOARD
    ui_state.input_number = input_getPressedNumber(msg);
#endif // CONFIG_UI_NO_KEYBOARD
    // CTCSS Encode/Decode Selection
    bool tone_tx_enable = state.channel.fm.txToneEn;
    bool tone_rx_enable = state.channel.fm.rxToneEn;
    uint8_t tone_flags = tone_tx_enable << 1 | tone_rx_enable;
    vpQueueFlags_t queueFlags = vp_getVoiceLevelQueueFlags();

    switch(ui_state.input_number)
    {
        case 1:
            if(state.channel.mode == OPMODE_FM)
            {
                if(state.channel.fm.txTone == 0)
                {
                    state.channel.fm.txTone = MAX_TONE_INDEX-1;
                }
                else
                {
                    state.channel.fm.txTone--;
                }

                state.channel.fm.txTone %= MAX_TONE_INDEX;
                state.channel.fm.rxTone = state.channel.fm.txTone;
                *sync_rtx = true;
                vp_announceCTCSS(state.channel.fm.rxToneEn,
                                 state.channel.fm.rxTone,
                                 state.channel.fm.txToneEn,
                                 state.channel.fm.txTone,
                                 queueFlags);
            }
            break;

        case 2:
            if(state.channel.mode == OPMODE_FM)
            {
                state.channel.fm.txTone++;
                state.channel.fm.txTone %= MAX_TONE_INDEX;
                state.channel.fm.rxTone = state.channel.fm.txTone;
                *sync_rtx = true;
                vp_announceCTCSS(state.channel.fm.rxToneEn,
                                 state.channel.fm.rxTone,
                                 state.channel.fm.txToneEn,
                                 state.channel.fm.txTone,
                                 queueFlags);
            }
            break;
        case 3:
            if(state.channel.mode == OPMODE_FM)
            {
                tone_flags++;
                tone_flags %= 4;
                tone_tx_enable = tone_flags >> 1;
                tone_rx_enable = tone_flags & 1;
                state.channel.fm.txToneEn = tone_tx_enable;
                state.channel.fm.rxToneEn = tone_rx_enable;
                *sync_rtx = true;
                vp_announceCTCSS(state.channel.fm.rxToneEn,
                                 state.channel.fm.rxTone,
                                 state.channel.fm.txToneEn,
                                 state.channel.fm.txTone,
                                 queueFlags |vpqIncludeDescriptions);
            }
            break;
        case 4:
            if(state.channel.mode == OPMODE_FM)
            {
                state.channel.bandwidth++;
                state.channel.bandwidth %= 2;
                *sync_rtx = true;
                vp_announceBandwidth(state.channel.bandwidth, queueFlags);
            }
            break;
        case 5:
            // Cycle through radio modes
            #ifdef CONFIG_M17
            if(state.channel.mode == OPMODE_FM)
                state.channel.mode = OPMODE_M17;
            else if(state.channel.mode == OPMODE_M17)
                state.channel.mode = OPMODE_FM;
            else //catch any invalid states so they don't get locked out
            #endif
                state.channel.mode = OPMODE_FM;
            *sync_rtx = true;
            vp_announceRadioMode(state.channel.mode, queueFlags);
            break;
        case 6:
            if (state.channel.power == 1000)
                state.channel.power = 5000;
            else
                state.channel.power = 1000;
            *sync_rtx = true;
            vp_announcePower(state.channel.power, queueFlags);
            break;
#ifdef CONFIG_SCREEN_BRIGHTNESS
        case 7:
            _ui_changeBrightness(-5);
            vp_announceSettingsInt(&currentLanguage->brightness, queueFlags,
                                   state.settings.brightness);
            break;
        case 8:
            _ui_changeBrightness(+5);
            vp_announceSettingsInt(&currentLanguage->brightness, queueFlags,
                                   state.settings.brightness);
            break;
#endif
        case 9:
            if (!ui_state.input_locked)
                ui_state.input_locked = true;
            else
                ui_state.input_locked = false;
            break;
    }

#if defined(PLATFORM_TTWRPLUS)
    if(msg.keys & KEY_VOLDOWN)
#else
    if(msg.keys & KEY_LEFT || msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT)
#endif // PLATFORM_TTWRPLUS
    {
#ifdef CONFIG_KNOB_ABSOLUTE // If the radio has an absolute position knob
        state.settings.sqlLevel = platform_getChSelector() - 1;
#endif // CONFIG_KNOB_ABSOLUTE
        if(state.settings.sqlLevel > 0)
        {
            state.settings.sqlLevel -= 1;
            *sync_rtx = true;
            vp_announceSquelch(state.settings.sqlLevel, queueFlags);
        }
    }

#if defined(PLATFORM_TTWRPLUS)
    else if(msg.keys & KEY_VOLUP)
#else
    else if(msg.keys & KEY_RIGHT || msg.keys & KEY_UP || msg.keys & KNOB_RIGHT)
#endif // PLATFORM_TTWRPLUS
    {
#ifdef CONFIG_KNOB_ABSOLUTE
        state.settings.sqlLevel = platform_getChSelector() - 1;
#endif
        if(state.settings.sqlLevel < 15)
        {
            state.settings.sqlLevel += 1;
            *sync_rtx = true;
            vp_announceSquelch(state.settings.sqlLevel, queueFlags);
        }
    }
}

static void _ui_menuUp(uint8_t menu_entries)
{
    if(ui_state.menu_selected > 0)
        ui_state.menu_selected -= 1;
    else
        ui_state.menu_selected = menu_entries - 1;
    vp_playMenuBeepIfNeeded(ui_state.menu_selected==0);
}

static void _ui_menuDown(uint8_t menu_entries)
{
    if(ui_state.menu_selected < menu_entries - 1)
        ui_state.menu_selected += 1;
    else
        ui_state.menu_selected = 0;
    vp_playMenuBeepIfNeeded(ui_state.menu_selected==0);
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
        vp_playMenuBeepIfNeeded(true);
    }
}

static void _ui_textInputReset(char *buf)
{
    ui_state.input_number = 0;
    ui_state.input_position = 0;
    ui_state.input_set = 0;
    ui_state.last_keypress = 0;
    memset(buf, 0, 9);
    buf[0] = '_';
}

static void _ui_textInputKeypad(char *buf, uint8_t max_len, kbd_msg_t msg,
                         bool callsign)
{
    long long now = getTimeMs();
    // Get currently pressed number key
    uint8_t num_key = input_getPressedChar(msg);

    bool key_timeout = ((now - ui_state.last_keypress) >= input_longPressTimeout);
    bool same_key = ui_state.input_number == num_key;
    // Get number of symbols related to currently pressed key
    uint8_t num_symbols = 0;
    if(callsign)
    {
        num_symbols = strlen(symbols_ITU_T_E161_callsign[num_key]);
        if(num_symbols == 0)
            return;
    }
    else
        num_symbols = strlen(symbols_ITU_T_E161[num_key]);

    // Return if max length is reached or finished editing last character
    if((ui_state.input_position >= max_len) || ((ui_state.input_position == (max_len-1)) && (key_timeout || !same_key)))
        return;

    // Skip keypad logic for first keypress
    if(ui_state.last_keypress != 0)
    {
        // Same key pressed and timeout not expired: cycle over chars of current key
        if(same_key && !key_timeout)
        {
            ui_state.input_set = (ui_state.input_set + 1) % num_symbols;
        }
        // Different key pressed: save current char and change key
        else
        {
            ui_state.input_position += 1;
            ui_state.input_set = 0;
        }
    }
    // Show current character on buffer
    if(callsign)
        buf[ui_state.input_position] = symbols_ITU_T_E161_callsign[num_key][ui_state.input_set];
    else
    {
        buf[ui_state.input_position] = symbols_ITU_T_E161[num_key][ui_state.input_set];
    }
    // Announce the character
    vp_announceInputChar(buf[ui_state.input_position]);
    // Update reference values
    ui_state.input_number = num_key;
    ui_state.last_keypress = now;
}

static void _ui_textInputConfirm(char *buf)
{
    buf[ui_state.input_position + 1] = '\0';
}

static void _ui_textInputDel(char *buf)
{
    // announce the char about to be backspaced.
    // Note this assumes editing callsign.
    // If we edit a different buffer which allows the underline char, we may
    // not want to exclude it, but when editing callsign, we do not want to say
    // underline since it means the field is empty.
    if(buf[ui_state.input_position]
    && buf[ui_state.input_position]!='_')
        vp_announceInputChar(buf[ui_state.input_position]);

    buf[ui_state.input_position] = '\0';
    // Move back input cursor
    if(ui_state.input_position > 0)
    {
        ui_state.input_position--;
    // If we deleted the initial character, reset starting condition
    }
    else
        ui_state.last_keypress = 0;
    ui_state.input_set = 0;
}

static void _ui_numberInputKeypad(uint32_t *num, kbd_msg_t msg)
{
    long long now = getTimeMs();

#ifdef CONFIG_UI_NO_KEYBOARD
    // If knob is turned, increment or Decrement
    if (msg.keys & KNOB_LEFT)
    {
        *num = *num + 1;
        if (*num % 10 == 0)
            *num = *num - 10;
    }

    if (msg.keys & KNOB_RIGHT)
    {
        if (*num == 0)
            *num = 9;
        else
        {
            *num = *num - 1;
            if (*num % 10 == 9)
                *num = *num + 10;
        }
    }

    // If enter is pressed, advance to the next digit
    if (msg.keys & KEY_ENTER)
        *num *= 10;

    // Announce the character
    vp_announceInputChar('0' + *num % 10);

    // Update reference values
    ui_state.input_number = *num % 10;
#else
    // Maximum frequency len is uint32_t max value number of decimal digits
    if(ui_state.input_position >= 10)
        return;

    // Get currently pressed number key
    uint8_t num_key = input_getPressedNumber(msg);
    *num *= 10;
    *num += num_key;

    // Announce the character
    vp_announceInputChar('0' + num_key);

    // Update reference values
    ui_state.input_number = num_key;
#endif

    ui_state.last_keypress = now;
}

static void _ui_numberInputDel(uint32_t *num)
{
    // announce the digit about to be backspaced.
    vp_announceInputChar('0' + *num % 10);

    // Move back input cursor
    if(ui_state.input_position > 0)
        ui_state.input_position--;
    else
        ui_state.last_keypress = 0;

    ui_state.input_set = 0;
}

void ui_init()
{
    last_event_tick = getTimeMs();
    redraw_needed = true;
    _ui_calculateLayout(&layout);
    layout_ready = true;
    // Initialize struct ui_state to all zeroes
    // This syntax is called compound literal
    // https://stackoverflow.com/questions/6891720/initialize-reset-struct-to-zero-null
    ui_state = (const struct ui_state_t){ 0 };
}

void ui_drawSplashScreen()
{
    gfx_clearScreen();

    #if CONFIG_SCREEN_HEIGHT > 64
    static const point_t    logo_orig = {0, (CONFIG_SCREEN_HEIGHT / 2) - 6};
    static const point_t    call_orig = {0, CONFIG_SCREEN_HEIGHT - 8};
    static const fontSize_t logo_font = FONT_SIZE_12PT;
    static const fontSize_t call_font = FONT_SIZE_8PT;
    #else
    static const point_t    logo_orig = {0, 19};
    static const point_t    call_orig = {0, CONFIG_SCREEN_HEIGHT - 8};
    static const fontSize_t logo_font = FONT_SIZE_8PT;
    static const fontSize_t call_font = FONT_SIZE_6PT;
    #endif

    gfx_print(logo_orig, logo_font, TEXT_ALIGN_CENTER, yellow_fab413, "O P N\nR T X");
    gfx_print(call_orig, call_font, TEXT_ALIGN_CENTER, color_white, state.settings.callsign);

    vp_announceSplashScreen();
}

void ui_saveState()
{
    last_state = state;
}

#ifdef CONFIG_GPS
static uint16_t priorGPSSpeed = 0;
static int16_t  priorGPSAltitude = 0;
static int16_t  priorGPSDirection = 500; // impossible value init.
static uint8_t  priorGPSFixQuality= 0;
static uint8_t  priorGPSFixType = 0;
static uint8_t  priorSatellitesInView = 0;
static uint32_t vpGPSLastUpdate = 0;

static vpGPSInfoFlags_t GetGPSDirectionOrSpeedChanged()
{
    if (!state.settings.gps_enabled)
        return vpGPSNone;

    uint32_t now = getTimeMs();
    if (now - vpGPSLastUpdate < 8000)
        return vpGPSNone;

    vpGPSInfoFlags_t whatChanged=  vpGPSNone;

    if (state.gps_data.fix_quality != priorGPSFixQuality)
    {
        whatChanged |= vpGPSFixQuality;
        priorGPSFixQuality= state.gps_data.fix_quality;
    }

    if (state.gps_data.fix_type != priorGPSFixType)
    {
        whatChanged |= vpGPSFixType;
        priorGPSFixType = state.gps_data.fix_type;
    }

    if (state.gps_data.speed != priorGPSSpeed)
    {
        whatChanged |= vpGPSSpeed;
        priorGPSSpeed = state.gps_data.speed;
    }

    if (state.gps_data.altitude != priorGPSAltitude)
    {
        whatChanged |= vpGPSAltitude;
        priorGPSAltitude = state.gps_data.altitude;
    }

    if (state.gps_data.tmg_true != priorGPSDirection)
    {
        whatChanged |= vpGPSDirection;
        priorGPSDirection = state.gps_data.tmg_true;
    }

    if (state.gps_data.satellites_in_view != priorSatellitesInView)
    {
        whatChanged |= vpGPSSatCount;
        priorSatellitesInView = state.gps_data.satellites_in_view;
    }

    if (whatChanged)
        vpGPSLastUpdate=now;

    return whatChanged;
}
#endif // CONFIG_GPS

void ui_updateFSM(bool *sync_rtx)
{
    // Check for events
    if(evQueue_wrPos == evQueue_rdPos) return;

    // Pop an event from the queue
    uint8_t newTail = (evQueue_rdPos + 1) % MAX_NUM_EVENTS;
    event_t event   = evQueue[evQueue_rdPos];
    evQueue_rdPos   = newTail;

    // There is some event to process, we need an UI redraw.
    // UI redraw request is cancelled if we're in standby mode.
    redraw_needed = true;
    if(standby) redraw_needed = false;

    // Check if battery has enough charge to operate.
    // Check is skipped if there is an ongoing transmission, since the voltage
    // drop caused by the RF PA power absorption causes spurious triggers of
    // the low battery alert.
    bool txOngoing = platform_getPttStatus();
#if !defined(PLATFORM_TTWRPLUS)
    if ((!state.emergency) && (!txOngoing) && (state.charge <= 0))
    {
        state.ui_screen = LOW_BAT;
        if(event.type == EVENT_KBD && event.payload)
        {
            state.ui_screen = MAIN_VFO;
            state.emergency = true;
        }
        return;
    }
#endif // PLATFORM_TTWRPLUS

    // Unlatch and exit from macro menu on PTT press
    if(macro_latched && txOngoing)
    {
        macro_latched = false;
        macro_menu = false;
    }

    long long now = getTimeMs();
    // Process pressed keys
    if(event.type == EVENT_KBD)
    {
        kbd_msg_t msg;
        msg.value = event.payload;
        bool f1Handled = false;
        vpQueueFlags_t queueFlags = vp_getVoiceLevelQueueFlags();
        // If we get out of standby, we ignore the kdb event
        // unless is the MONI key for the MACRO functions
        if (_ui_exitStandby(now) && !(msg.keys & KEY_MONI))
            return;
        // If MONI is pressed, activate MACRO functions
        bool moniPressed = msg.keys & KEY_MONI;
        if(moniPressed || macro_latched)
        {
            macro_menu = true;

            if(state.settings.macroMenuLatch == 1)
            {
                // long press moni on its own latches function.
                if (moniPressed && msg.long_press && !macro_latched)
                {
                    macro_latched = true;
                    vp_beep(BEEP_FUNCTION_LATCH_ON, LONG_BEEP);
                }
                else if (moniPressed && macro_latched)
                {
                    macro_latched = false;
                    vp_beep(BEEP_FUNCTION_LATCH_OFF, LONG_BEEP);
                }
            }

            _ui_fsm_menuMacro(msg, sync_rtx);
            return;
        }
        else
        {
            macro_menu = false;
        }
#if defined(PLATFORM_TTWRPLUS)
        // T-TWR Plus has no KEY_MONI, using KEY_VOLDOWN long press instead
        if ((msg.keys & KEY_VOLDOWN) && msg.long_press)
        {
            macro_menu = true;
            macro_latched = true;
        }
#endif // PLA%FORM_TTWRPLUS

        if(state.tone_enabled && !(msg.keys & KEY_HASH))
        {
            state.tone_enabled = false;
            *sync_rtx = true;
        }

        int priorUIScreen = state.ui_screen;
        switch(state.ui_screen)
        {
            // VFO screen
            case MAIN_VFO:
            {
                // Enable Tx in MAIN_VFO mode
                if (state.txDisable)
                {
                    state.txDisable = false;
                    *sync_rtx = true;
                }

                // Break out of the FSM if the keypad is locked but allow the
                // use of the hash key in FM mode for the 1750Hz tone.
                bool skipLock =  (state.channel.mode == OPMODE_FM)
                              && (msg.keys == KEY_HASH);

                if ((ui_state.input_locked == true) && (skipLock == false))
                    break;

                if(ui_state.edit_mode)
                {
                    #ifdef CONFIG_M17
                    if(state.channel.mode == OPMODE_M17)
                    {
                        if(msg.keys & KEY_ENTER)
                        {
                            _ui_textInputConfirm(ui_state.new_callsign);
                            // Save selected dst ID and disable input mode
                            strncpy(state.settings.m17_dest, ui_state.new_callsign, 10);
                            ui_state.edit_mode = false;
                            *sync_rtx = true;
                            vp_announceM17Info(NULL,  ui_state.edit_mode,
                                               queueFlags);
                        }
                        else if(msg.keys & KEY_HASH)
                        {
                            // Save selected dst ID and disable input mode
                            strncpy(state.settings.m17_dest, "", 1);
                            ui_state.edit_mode = false;
                            *sync_rtx = true;
                            vp_announceM17Info(NULL,  ui_state.edit_mode,
                                               queueFlags);
                        }
                        else if(msg.keys & KEY_ESC)
                            // Discard selected dst ID and disable input mode
                            ui_state.edit_mode = false;
                        else if(msg.keys & KEY_UP || msg.keys & KEY_DOWN ||
                                msg.keys & KEY_LEFT || msg.keys & KEY_RIGHT)
                            _ui_textInputDel(ui_state.new_callsign);
                        else if(input_isCharPressed(msg))
                            _ui_textInputKeypad(ui_state.new_callsign, 9, msg, true);
                        break;
                    }
                    #endif
                }
                else
                {
                    if(msg.keys & KEY_ENTER)
                    {
                        // Save current main state
                        ui_state.last_main_state = state.ui_screen;
                        // Open Menu
                        state.ui_screen = MENU_TOP;
                        // The selected item will be announced when the item is first selected.
                    }
                    else if(msg.keys & KEY_ESC)
                    {
                        // Save VFO channel
                        state.vfo_channel = state.channel;
                        int result = _ui_fsm_loadChannel(state.channel_index, sync_rtx);
                        // Read successful and channel is valid
                        if(result != -1)
                        {
                            // Switch to MEM screen
                            state.ui_screen = MAIN_MEM;
                            // anounce the active channel name.
                            vp_announceChannelName(&state.channel,
                                                   state.channel_index,
                                                   queueFlags);
                        }
                    }
                    else if(msg.keys & KEY_HASH)
                    {
                        #ifdef CONFIG_M17
                        // Only enter edit mode when using M17
                        if(state.channel.mode == OPMODE_M17)
                        {
                            // Enable dst ID input
                            ui_state.edit_mode = true;
                            // Reset text input variables
                            _ui_textInputReset(ui_state.new_callsign);
                            vp_announceM17Info(NULL,  ui_state.edit_mode,
                                               queueFlags);
                        }
                        else
                        #endif
                        {
                            if(!state.tone_enabled)
                            {
                                state.tone_enabled = true;
                                *sync_rtx = true;
                            }
                        }
                    }
                    else if(msg.keys & KEY_UP || msg.keys & KNOB_RIGHT)
                    {
                        // Increment TX and RX frequency of 12.5KHz
                        if(_ui_freq_check_limits(state.channel.rx_frequency + freq_steps[state.step_index]) &&
                           _ui_freq_check_limits(state.channel.tx_frequency + freq_steps[state.step_index]))
                        {
                            state.channel.rx_frequency += freq_steps[state.step_index];
                            state.channel.tx_frequency += freq_steps[state.step_index];
                            *sync_rtx = true;
                            vp_announceFrequencies(state.channel.rx_frequency,
                                                   state.channel.tx_frequency,
                                                   queueFlags);
                        }
                    }
                    else if(msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT)
                    {
                        // Decrement TX and RX frequency of 12.5KHz
                        if(_ui_freq_check_limits(state.channel.rx_frequency - freq_steps[state.step_index]) &&
                           _ui_freq_check_limits(state.channel.tx_frequency - freq_steps[state.step_index]))
                        {
                            state.channel.rx_frequency -= freq_steps[state.step_index];
                            state.channel.tx_frequency -= freq_steps[state.step_index];
                            *sync_rtx = true;
                            vp_announceFrequencies(state.channel.rx_frequency,
                                                   state.channel.tx_frequency,
                                                   queueFlags);
                        }
                    }
                    else if(msg.keys & KEY_F1)
                    {
                        if (state.settings.vpLevel > vpBeep)
                        {// quick press repeat vp, long press summary.
                            if (msg.long_press)
                                vp_announceChannelSummary(&state.channel, 0,
                                                          state.bank, vpAllInfo);
                            else
                                vp_replayLastPrompt();
                            f1Handled = true;
                        }
                    }
                    else if(input_isNumberPressed(msg))
                    {
                        // Open Frequency input screen
                        state.ui_screen = MAIN_VFO_INPUT;
                        // Reset input position and selection
                        ui_state.input_position = 1;
                        ui_state.input_set = SET_RX;
                        // do not play  because we will also announce the number just entered.
                        vp_announceInputReceiveOrTransmit(false, vpqInit);
                        vp_queueInteger(input_getPressedNumber(msg));
                        vp_play();

                        ui_state.new_rx_frequency = 0;
                        ui_state.new_tx_frequency = 0;
                        // Save pressed number to calculare frequency and show in GUI
                        ui_state.input_number = input_getPressedNumber(msg);
                        // Calculate portion of the new frequency
                        ui_state.new_rx_frequency = _ui_freq_add_digit(ui_state.new_rx_frequency,
                                                                       ui_state.input_position,
                                                                       ui_state.input_number);
                    }
                }
            }
                break;
            // VFO frequency input screen
            case MAIN_VFO_INPUT:
                if(msg.keys & KEY_ENTER)
                {
                    _ui_fsm_confirmVFOInput(sync_rtx);
                }
                else if(msg.keys & KEY_ESC)
                {
                    // Cancel frequency input, return to VFO mode
                    state.ui_screen = MAIN_VFO;
                }
                else if(msg.keys & KEY_UP || msg.keys & KEY_DOWN)
                {
                    if(ui_state.input_set == SET_RX)
                    {
                        ui_state.input_set = SET_TX;
                        vp_announceInputReceiveOrTransmit(true, queueFlags);
                    }
                    else if(ui_state.input_set == SET_TX)
                    {
                        ui_state.input_set = SET_RX;
                        vp_announceInputReceiveOrTransmit(false, queueFlags);
                    }
                    // Reset input position
                    ui_state.input_position = 0;
                }
                else if(input_isNumberPressed(msg))
                {
                    _ui_fsm_insertVFONumber(msg, sync_rtx);
                }
                break;
            // MEM screen
            case MAIN_MEM:
                // Enable Tx in MAIN_MEM mode
                if (state.txDisable)
                {
                    state.txDisable = false;
                    *sync_rtx = true;
                }
                if (ui_state.input_locked)
                    break;
                // M17 Destination callsign input
                if(ui_state.edit_mode)
                {
                    {
                        if(msg.keys & KEY_ENTER)
                        {
                            _ui_textInputConfirm(ui_state.new_callsign);
                            // Save selected dst ID and disable input mode
                            strncpy(state.settings.m17_dest, ui_state.new_callsign, 10);
                            ui_state.edit_mode = false;
                            *sync_rtx = true;
                        }
                        else if(msg.keys & KEY_HASH)
                        {
                            // Save selected dst ID and disable input mode
                            strncpy(state.settings.m17_dest, "", 1);
                            ui_state.edit_mode = false;
                            *sync_rtx = true;
                        }
                        else if(msg.keys & KEY_ESC)
                            // Discard selected dst ID and disable input mode
                            ui_state.edit_mode = false;
                        else if(msg.keys & KEY_F1)
                        {
                            if (state.settings.vpLevel > vpBeep)
                            {
                                // Quick press repeat vp, long press summary.
                                if (msg.long_press)
                                {
                                    vp_announceChannelSummary(
                                            &state.channel,
                                            state.channel_index,
                                            state.bank,
                                            vpAllInfo);
                                }
                                else
                                {
                                    vp_replayLastPrompt();
                                }

                                f1Handled = true;
                            }
                        }
                        else if(msg.keys & KEY_UP || msg.keys & KEY_DOWN ||
                                msg.keys & KEY_LEFT || msg.keys & KEY_RIGHT)
                            _ui_textInputDel(ui_state.new_callsign);
                        else if(input_isCharPressed(msg))
                            _ui_textInputKeypad(ui_state.new_callsign, 9, msg, true);
                        break;
                    }
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
                    else if(msg.keys & KEY_ESC)
                    {
                        // Restore VFO channel
                        state.channel = state.vfo_channel;
                        // Update RTX configuration
                        *sync_rtx = true;
                        // Switch to VFO screen
                        state.ui_screen = MAIN_VFO;
                    }
                    else if(msg.keys & KEY_HASH)
                    {
                        // Only enter edit mode when using M17
                        if(state.channel.mode == OPMODE_M17)
                        {
                            // Enable dst ID input
                            ui_state.edit_mode = true;
                            // Reset text input variables
                            _ui_textInputReset(ui_state.new_callsign);
                        }
                        else
                        {
                            if(!state.tone_enabled)
                            {
                                state.tone_enabled = true;
                                *sync_rtx = true;
                            }
                        }
                    }
                    else if(msg.keys & KEY_F1)
                    {
                        if (state.settings.vpLevel > vpBeep)
                        {// quick press repeat vp, long press summary.
                            if (msg.long_press)
                            {
                                vp_announceChannelSummary(&state.channel,
                                                          state.channel_index+1,
                                                          state.bank, vpAllInfo);
                            }
                            else
                            {
                                vp_replayLastPrompt();
                            }

                            f1Handled = true;
                        }
                    }
                    else if(msg.keys & KEY_UP || msg.keys & KNOB_RIGHT)
                    {
                        _ui_fsm_loadChannel(state.channel_index + 1, sync_rtx);
                        vp_announceChannelName(&state.channel,
                                               state.channel_index+1,
                                               queueFlags);
                    }
                    else if(msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT)
                    {
                        _ui_fsm_loadChannel(state.channel_index - 1, sync_rtx);
                        vp_announceChannelName(&state.channel,
                                               state.channel_index+1,
                                               queueFlags);
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
                        case M_BANK:
                            state.ui_screen = MENU_BANK;
                            break;
                        case M_CHANNEL:
                            state.ui_screen = MENU_CHANNEL;
                            break;
                        case M_CONTACTS:
                            state.ui_screen = MENU_CONTACTS;
                            break;
#ifdef CONFIG_GPS
                        case M_GPS:
                            state.ui_screen = MENU_GPS;
                            break;
#endif
                        case M_SETTINGS:
                            state.ui_screen = MENU_SETTINGS;
                            break;
                        case M_INFO:
                            state.ui_screen = MENU_INFO;
                            break;
                        case M_ABOUT:
                            state.ui_screen = MENU_ABOUT;
                            break;
                    }
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                }
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(ui_state.last_main_state);
                break;
            // Zone menu screen
            case MENU_BANK:
            // Channel menu screen
            case MENU_CHANNEL:
            // Contacts menu screen
            case MENU_CONTACTS:
                if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    // Using 1 as parameter disables menu wrap around
                    _ui_menuUp(1);
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                {
                    if(state.ui_screen == MENU_BANK)
                    {
                        bankHdr_t bank;
                        // manu_selected is 0-based
                        // bank 0 means "All Channel" mode
                        // banks (1, n) are mapped to banks (0, n-1)
                        if(cps_readBankHeader(&bank, ui_state.menu_selected) != -1)
                            ui_state.menu_selected += 1;
                    }
                    else if(state.ui_screen == MENU_CHANNEL)
                    {
                        channel_t channel;
                        if(cps_readChannel(&channel, ui_state.menu_selected + 1) != -1)
                            ui_state.menu_selected += 1;
                    }
                    else if(state.ui_screen == MENU_CONTACTS)
                    {
                        contact_t contact;
                        if(cps_readContact(&contact, ui_state.menu_selected + 1) != -1)
                            ui_state.menu_selected += 1;
                    }
                }
                else if(msg.keys & KEY_ENTER)
                {
                    if(state.ui_screen == MENU_BANK)
                    {
                        bankHdr_t newbank;
                        int result = 0;
                        // If "All channels" is selected, load default bank
                        if(ui_state.menu_selected == 0)
                            state.bank_enabled = false;
                        else
                        {
                            state.bank_enabled = true;
                            result = cps_readBankHeader(&newbank, ui_state.menu_selected - 1);
                        }
                        if(result != -1)
                        {
                            state.bank = ui_state.menu_selected - 1;
                            // If we were in VFO mode, save VFO channel
                            if(ui_state.last_main_state == MAIN_VFO)
                                state.vfo_channel = state.channel;
                            // Load bank first channel
                            _ui_fsm_loadChannel(0, sync_rtx);
                            // Switch to MEM screen
                            state.ui_screen = MAIN_MEM;
                        }
                    }
                    if(state.ui_screen == MENU_CHANNEL)
                    {
                        // If we were in VFO mode, save VFO channel
                        if(ui_state.last_main_state == MAIN_VFO)
                            state.vfo_channel = state.channel;
                        _ui_fsm_loadChannel(ui_state.menu_selected, sync_rtx);
                        // Switch to MEM screen
                        state.ui_screen = MAIN_MEM;
                    }
                }
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
#ifdef CONFIG_GPS
            // GPS menu screen
            case MENU_GPS:
                if ((msg.keys & KEY_F1) && (state.settings.vpLevel > vpBeep))
                {// quick press repeat vp, long press summary.
                    if (msg.long_press)
                        vp_announceGPSInfo(vpGPSAll);
                    else
                        vp_replayLastPrompt();
                    f1Handled = true;
                }
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
#endif
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
#ifdef CONFIG_RTC
                        case S_TIMEDATE:
                            state.ui_screen = SETTINGS_TIMEDATE;
                            break;
#endif
#ifdef CONFIG_GPS
                        case S_GPS:
                            state.ui_screen = SETTINGS_GPS;
                            break;
#endif
                        case S_RADIO:
                            state.ui_screen = SETTINGS_RADIO;
                            break;
#ifdef CONFIG_M17
                        case S_M17:
                            state.ui_screen = SETTINGS_M17;
                            break;
#endif
                        case S_ACCESSIBILITY:
                            state.ui_screen = SETTINGS_ACCESSIBILITY;
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
            // Flash backup and restore menu screen
            case MENU_BACKUP_RESTORE:
                if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(settings_num);
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(settings_num);
                else if(msg.keys & KEY_ENTER)
                {

                    switch(ui_state.menu_selected)
                    {
                        case BR_BACKUP:
                            state.ui_screen = MENU_BACKUP;
                            break;
                        case BR_RESTORE:
                            state.ui_screen = MENU_RESTORE;
                            break;
                        default:
                            state.ui_screen = MENU_BACKUP_RESTORE;
                    }
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                }
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
            case MENU_BACKUP:
            case MENU_RESTORE:
                if(msg.keys & KEY_ESC)
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
#ifdef CONFIG_RTC
            // Time&Date settings screen
            case SETTINGS_TIMEDATE:
                if(msg.keys & KEY_ENTER)
                {
                    // Switch to set Time&Date mode
                    state.ui_screen = SETTINGS_TIMEDATE_SET;
                    // Reset input position and selection
                    ui_state.input_position = 0;
                    memset(&ui_state.new_timedate, 0, sizeof(datetime_t));
                    vp_announceBuffer(&currentLanguage->timeAndDate,
                                      true, false, "dd/mm/yy");
                }
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                break;
            // Time&Date settings screen, edit mode
            case SETTINGS_TIMEDATE_SET:
                if(msg.keys & KEY_ENTER)
                {
                    // Save time only if all digits have been inserted
                    if(ui_state.input_position < TIMEDATE_DIGITS)
                        break;
                    // Return to Time&Date menu, saving values
                    // NOTE: The user inserted a local time, we must save an UTC time
                    datetime_t utc_time = localTimeToUtc(ui_state.new_timedate,
                                                         state.settings.utc_timezone);
                    platform_setTime(utc_time);
                    state.time = utc_time;
                    vp_announceSettingsTimeDate();
                    state.ui_screen = SETTINGS_TIMEDATE;
                }
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(SETTINGS_TIMEDATE);
                else if(input_isNumberPressed(msg))
                {
                    // Discard excess digits
                    if(ui_state.input_position > TIMEDATE_DIGITS)
                        break;
                    ui_state.input_position += 1;
                    ui_state.input_number = input_getPressedNumber(msg);
                    _ui_timedate_add_digit(&ui_state.new_timedate, ui_state.input_position,
                                            ui_state.input_number);
                }
                break;
#endif
            case SETTINGS_DISPLAY:
                if(msg.keys & KEY_LEFT || (ui_state.edit_mode &&
                   (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT)))
                {
                    switch(ui_state.menu_selected)
                    {
#ifdef CONFIG_SCREEN_BRIGHTNESS
                        case D_BRIGHTNESS:
                            _ui_changeBrightness(-5);
                            vp_announceSettingsInt(&currentLanguage->brightness, queueFlags,
                                                   state.settings.brightness);
                            break;
#endif
#ifdef CONFIG_SCREEN_CONTRAST
                        case D_CONTRAST:
                            _ui_changeContrast(-4);
                            vp_announceSettingsInt(&currentLanguage->brightness, queueFlags,
                                                   state.settings.contrast);
                            break;
#endif
                        case D_TIMER:
                            _ui_changeTimer(-1);
                            vp_announceDisplayTimer();
                            break;
                        default:
                            state.ui_screen = SETTINGS_DISPLAY;
                    }
                }
                else if(msg.keys & KEY_RIGHT || (ui_state.edit_mode &&
                        (msg.keys & KEY_UP || msg.keys & KNOB_RIGHT)))
                {
                    switch(ui_state.menu_selected)
                    {
#ifdef CONFIG_SCREEN_BRIGHTNESS
                        case D_BRIGHTNESS:
                            _ui_changeBrightness(+5);
                            vp_announceSettingsInt(&currentLanguage->brightness, queueFlags,
                                                   state.settings.brightness);
                            break;
#endif
#ifdef CONFIG_SCREEN_CONTRAST
                        case D_CONTRAST:
                            _ui_changeContrast(+4);
                            vp_announceSettingsInt(&currentLanguage->brightness, queueFlags,
                                                   state.settings.contrast);
                            break;
#endif
                        case D_TIMER:
                            _ui_changeTimer(+1);
                            vp_announceDisplayTimer();
                            break;
                        default:
                            state.ui_screen = SETTINGS_DISPLAY;
                    }
                }
                else if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(display_num);
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(display_num);
                else if(msg.keys & KEY_ENTER)
                    ui_state.edit_mode = !ui_state.edit_mode;
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                break;
#ifdef CONFIG_GPS
            case SETTINGS_GPS:
                if(msg.keys & KEY_LEFT || msg.keys & KEY_RIGHT ||
                   (ui_state.edit_mode &&
                   (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT ||
                    msg.keys & KEY_UP || msg.keys & KNOB_RIGHT)))
                {
                    switch(ui_state.menu_selected)
                    {
                        case G_ENABLED:
                            if(state.settings.gps_enabled)
                                state.settings.gps_enabled = 0;
                            else
                                state.settings.gps_enabled = 1;
                            vp_announceSettingsOnOffToggle(&currentLanguage->gpsEnabled,
                                                           queueFlags,
                                                           state.settings.gps_enabled);
                            break;
                        case G_SET_TIME:
                            state.gps_set_time = !state.gps_set_time;
                            vp_announceSettingsOnOffToggle(&currentLanguage->gpsSetTime,
                                                           queueFlags,
                                                           state.gps_set_time);
                            break;
                        case G_TIMEZONE:
                            if(msg.keys & KEY_LEFT || msg.keys & KEY_DOWN ||
                               msg.keys & KNOB_LEFT)
                                state.settings.utc_timezone -= 1;
                            else if(msg.keys & KEY_RIGHT || msg.keys & KEY_UP ||
                                    msg.keys & KNOB_RIGHT)
                                state.settings.utc_timezone += 1;
                            vp_announceTimeZone(state.settings.utc_timezone, queueFlags);
                            break;
                        default:
                            state.ui_screen = SETTINGS_GPS;
                    }
                }
                else if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(settings_gps_num);
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(settings_gps_num);
                else if(msg.keys & KEY_ENTER)
                    ui_state.edit_mode = !ui_state.edit_mode;
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                break;
#endif
            // Radio Settings
            case SETTINGS_RADIO:
                // If the entry is selected with enter we are in edit_mode
                if (ui_state.edit_mode)
                {
                    switch(ui_state.menu_selected)
                    {
                        case R_OFFSET:
                            // Handle offset frequency input
#if defined(CONFIG_UI_NO_KEYBOARD)
                            if(msg.long_press && msg.keys & KEY_ENTER)
                            {
                                // Long press on CONFIG_UI_NO_KEYBOARD causes digits to advance by one
                                ui_state.new_offset /= 10;
#else
                            if(msg.keys & KEY_ENTER)
                            {
#endif
                                // Apply new offset
                                state.channel.tx_frequency = state.channel.rx_frequency + ui_state.new_offset;
                                vp_queueStringTableEntry(&currentLanguage->frequencyOffset);
                                vp_queueFrequency(ui_state.new_offset);
                                ui_state.edit_mode = false;
                            }
                            else
                            if(msg.keys & KEY_ESC)
                            {
                                // Announce old frequency offset
                                vp_queueStringTableEntry(&currentLanguage->frequencyOffset);
                                vp_queueFrequency((int32_t)state.channel.tx_frequency - (int32_t)state.channel.rx_frequency);
                            }
                            else if(msg.keys & KEY_UP || msg.keys & KEY_DOWN ||
                                    msg.keys & KEY_LEFT || msg.keys & KEY_RIGHT)
                            {
                                _ui_numberInputDel(&ui_state.new_offset);
                            }
#if defined(CONFIG_UI_NO_KEYBOARD)
                            else if(msg.keys & KNOB_LEFT || msg.keys & KNOB_RIGHT || msg.keys & KEY_ENTER)
#else
                            else if(input_isNumberPressed(msg))
#endif
                            {
                                _ui_numberInputKeypad(&ui_state.new_offset, msg);
                                ui_state.input_position += 1;
                            }
                            else if (msg.long_press && (msg.keys & KEY_F1) && (state.settings.vpLevel > vpBeep))
                            {
                                vp_queueFrequency(ui_state.new_offset);
                                f1Handled=true;
                            }
                            break;
                        case R_DIRECTION:
                            if(msg.keys & KEY_UP || msg.keys & KEY_DOWN ||
                               msg.keys & KEY_LEFT || msg.keys & KEY_RIGHT ||
                               msg.keys & KNOB_LEFT || msg.keys & KNOB_RIGHT)
                            {
                                // Invert frequency offset direction
                                if (state.channel.tx_frequency >= state.channel.rx_frequency)
                                    state.channel.tx_frequency -= 2 * ((int32_t)state.channel.tx_frequency - (int32_t)state.channel.rx_frequency);
                                else // Switch to positive offset
                                    state.channel.tx_frequency -= 2 * ((int32_t)state.channel.tx_frequency - (int32_t)state.channel.rx_frequency);
                            }
                            break;
                        case R_STEP:
                            if (msg.keys & KEY_UP || msg.keys & KEY_RIGHT || msg.keys & KNOB_RIGHT)
                            {
                                // Cycle over the available frequency steps
                                state.step_index++;
                                state.step_index %= n_freq_steps;
                            }
                            else if(msg.keys & KEY_DOWN || msg.keys & KEY_LEFT || msg.keys & KNOB_LEFT)
                            {
                                state.step_index += n_freq_steps;
                                state.step_index--;
                                state.step_index %= n_freq_steps;
                            }
                            break;
                        default:
                            state.ui_screen = SETTINGS_RADIO;
                    }
                    // If ENTER or ESC are pressed, exit edit mode, R_OFFSET is managed separately
                    if((ui_state.menu_selected != R_OFFSET && msg.keys & KEY_ENTER) || msg.keys & KEY_ESC)
                        ui_state.edit_mode = false;
                }
                else if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(settings_radio_num);
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(settings_radio_num);
                else if(msg.keys & KEY_ENTER) {
                    ui_state.edit_mode = true;
                    // If we are entering R_OFFSET clear temp offset
                    if (ui_state.menu_selected == R_OFFSET)
                        ui_state.new_offset = 0;
                    // Reset input position
                    ui_state.input_position = 0;
                }
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                break;
#ifdef CONFIG_M17
            // M17 Settings
            case SETTINGS_M17:
                if(ui_state.edit_mode)
                {
                    switch (ui_state.menu_selected)
                    {
                        case M17_CALLSIGN:
                            // Handle text input for M17 callsign
                            if(msg.keys & KEY_ENTER)
                            {
                                _ui_textInputConfirm(ui_state.new_callsign);
                                // Save selected callsign and disable input mode
                                strncpy(state.settings.callsign, ui_state.new_callsign, 10);
                                ui_state.edit_mode = false;
                                vp_announceBuffer(&currentLanguage->callsign,
                                                  false, true, state.settings.callsign);
                            }
                            else if(msg.keys & KEY_ESC)
                            {
                                // Discard selected callsign and disable input mode
                                ui_state.edit_mode = false;
                                vp_announceBuffer(&currentLanguage->callsign,
                                                  false, true, state.settings.callsign);
                            }
                            else if(msg.keys & KEY_UP || msg.keys & KEY_DOWN ||
                                     msg.keys & KEY_LEFT || msg.keys & KEY_RIGHT)
                            {
                                _ui_textInputDel(ui_state.new_callsign);
                            }
                            else if(input_isCharPressed(msg))
                            {
                                _ui_textInputKeypad(ui_state.new_callsign, 9, msg, true);
                            }
                            else if (msg.long_press && (msg.keys & KEY_F1) && (state.settings.vpLevel > vpBeep))
                            {
                                vp_announceBuffer(&currentLanguage->callsign,
                                                  true, true, ui_state.new_callsign);
                                f1Handled=true;
                            }
                            break;
                        case M17_CAN:
                            if(msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT)
                                _ui_changeM17Can(-1);
                            else if(msg.keys & KEY_UP || msg.keys & KNOB_RIGHT)
                                _ui_changeM17Can(+1);
                            else if(msg.keys & KEY_ENTER)
                                ui_state.edit_mode = !ui_state.edit_mode;
                            else if(msg.keys & KEY_ESC)
                                ui_state.edit_mode = false;
                            break;
                        case M17_CAN_RX:
                            if(msg.keys & KEY_LEFT || msg.keys & KEY_RIGHT ||
                                (ui_state.edit_mode &&
                                 (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT ||
                                  msg.keys & KEY_UP || msg.keys & KNOB_RIGHT)))
                            {
                                state.settings.m17_can_rx =
                                    !state.settings.m17_can_rx;
                            }
                            else if(msg.keys & KEY_ENTER)
                                ui_state.edit_mode = !ui_state.edit_mode;
                            else if(msg.keys & KEY_ESC)
                                ui_state.edit_mode = false;
                    }
                }
                else
                {
                    if(msg.keys & KEY_ENTER)
                    {
                        // Enable edit mode
                        ui_state.edit_mode = true;

                        // If callsign input, reset text input variables
                        if(ui_state.menu_selected == M17_CALLSIGN)
                        {
                            _ui_textInputReset(ui_state.new_callsign);
                            vp_announceBuffer(&currentLanguage->callsign,
                                            true, true, ui_state.new_callsign);
                        }
                    }
                    else if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                        _ui_menuUp(settings_m17_num);
                    else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                        _ui_menuDown(settings_m17_num);
                    else if((msg.keys & KEY_RIGHT) && (ui_state.menu_selected == M17_CAN))
                            _ui_changeM17Can(+1);
                    else if((msg.keys & KEY_LEFT)  && (ui_state.menu_selected == M17_CAN))
                            _ui_changeM17Can(-1);
                    else if(msg.keys & KEY_ESC)
                    {
                        *sync_rtx = true;
                        _ui_menuBack(MENU_SETTINGS);
                    }
                }
                break;
#endif
            case SETTINGS_ACCESSIBILITY:
                if(msg.keys & KEY_LEFT || (ui_state.edit_mode &&
                   (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT)))
                {
                    switch(ui_state.menu_selected)
                    {
                        case A_MACRO_LATCH:
                            _ui_changeMacroLatch(false);
                            break;
                        case A_LEVEL:
                            _ui_changeVoiceLevel(-1);
                            break;
                        case A_PHONETIC:
                            _ui_changePhoneticSpell(false);
                            break;
                        default:
                            state.ui_screen = SETTINGS_ACCESSIBILITY;
                    }
                }
                else if(msg.keys & KEY_RIGHT || (ui_state.edit_mode &&
                        (msg.keys & KEY_UP || msg.keys & KNOB_RIGHT)))
                {
                    switch(ui_state.menu_selected)
                    {
                        case A_MACRO_LATCH:
                            _ui_changeMacroLatch(true);
                            break;
                        case A_LEVEL:
                            _ui_changeVoiceLevel(1);
                            break;
                        case A_PHONETIC:
                            _ui_changePhoneticSpell(true);
                            break;
                        default:
                            state.ui_screen = SETTINGS_ACCESSIBILITY;
                    }
                }
                else if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(settings_accessibility_num);
                else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(settings_accessibility_num);
                else if(msg.keys & KEY_ENTER)
                    ui_state.edit_mode = !ui_state.edit_mode;
                else if(msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                break;
            case SETTINGS_RESET2DEFAULTS:
                if(! ui_state.edit_mode){
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
                } else {
                    if(msg.keys & KEY_ENTER)
                    {
                        ui_state.edit_mode = false;
                        state_resetSettingsAndVfo();
                        _ui_menuBack(MENU_SETTINGS);
                    }
                    else if(msg.keys & KEY_ESC)
                    {
                        ui_state.edit_mode = false;
                        _ui_menuBack(MENU_SETTINGS);
                    }
                }
                break;
        }

        // Enable Tx only if in MAIN_VFO or MAIN_MEM states
        bool inMemOrVfo = (state.ui_screen == MAIN_VFO) || (state.ui_screen == MAIN_MEM);
        if ((macro_menu == true) || ((inMemOrVfo == false) && (state.txDisable == false)))
        {
            state.txDisable = true;
            *sync_rtx = true;
        }
        if (!f1Handled && (msg.keys & KEY_F1) && (state.settings.vpLevel > vpBeep))
        {
            vp_replayLastPrompt();
        }
        else if ((priorUIScreen!=state.ui_screen) && state.settings.vpLevel > vpLow)
        {
            // When we switch to VFO or Channel screen, we need to announce it.
            // Likewise for information screens.
            // All other cases are handled as needed.
            vp_announceScreen(state.ui_screen);
        }
        // generic beep for any keydown if beep is enabled.
        // At vp levels higher than beep, keys will generate voice so no need
        // to beep or you'll get an unwanted click.
        if ((msg.keys &0xffff) && (state.settings.vpLevel == vpBeep))
            vp_beep(BEEP_KEY_GENERIC, SHORT_BEEP);
        // If we exit and re-enter the same menu, we want to ensure it speaks.
        if (msg.keys & KEY_ESC)
            _ui_reset_menu_anouncement_tracking();
    }
    else if(event.type == EVENT_STATUS)
    {
#ifdef CONFIG_GPS
        if ((state.ui_screen == MENU_GPS) &&
            (!vp_isPlaying()) &&
            (state.settings.vpLevel > vpLow) &&
            (!txOngoing && !rtx_rxSquelchOpen()))
        {// automatically read speed and direction changes only!
            vpGPSInfoFlags_t whatChanged = GetGPSDirectionOrSpeedChanged();
            if (whatChanged != vpGPSNone)
                vp_announceGPSInfo(whatChanged);
        }
#endif //            CONFIG_GPS

        if (txOngoing || rtx_rxSquelchOpen())
        {
            _ui_exitStandby(now);
            return;
        }

        if (_ui_checkStandby(now - last_event_tick))
        {
            _ui_enterStandby();
        }
    }
}

bool ui_updateGUI()
{
    if(redraw_needed == false)
        return false;

    if(!layout_ready)
    {
        _ui_calculateLayout(&layout);
        layout_ready = true;
    }
    // Draw current GUI page
    switch(last_state.ui_screen)
    {
        // VFO main screen
        case MAIN_VFO:
            _ui_drawMainVFO(&ui_state);
            break;
        // VFO frequency input screen
        case MAIN_VFO_INPUT:
            _ui_drawMainVFOInput(&ui_state);
            break;
        // MEM main screen
        case MAIN_MEM:
            _ui_drawMainMEM(&ui_state);
            break;
        // Top menu screen
        case MENU_TOP:
            _ui_drawMenuTop(&ui_state);
            break;
        // Zone menu screen
        case MENU_BANK:
            _ui_drawMenuBank(&ui_state);
            break;
        // Channel menu screen
        case MENU_CHANNEL:
            _ui_drawMenuChannel(&ui_state);
            break;
        // Contacts menu screen
        case MENU_CONTACTS:
            _ui_drawMenuContacts(&ui_state);
            break;
#ifdef CONFIG_GPS
        // GPS menu screen
        case MENU_GPS:
            _ui_drawMenuGPS();
            break;
#endif
        // Settings menu screen
        case MENU_SETTINGS:
            _ui_drawMenuSettings(&ui_state);
            break;
        // Flash backup and restore screen
        case MENU_BACKUP_RESTORE:
            _ui_drawMenuBackupRestore(&ui_state);
            break;
        // Flash backup screen
        case MENU_BACKUP:
            _ui_drawMenuBackup(&ui_state);
            break;
        // Flash restore screen
        case MENU_RESTORE:
            _ui_drawMenuRestore(&ui_state);
            break;
        // Info menu screen
        case MENU_INFO:
            _ui_drawMenuInfo(&ui_state);
            break;
        // About menu screen
        case MENU_ABOUT:
            _ui_drawMenuAbout(&ui_state);
            break;
#ifdef CONFIG_RTC
        // Time&Date settings screen
        case SETTINGS_TIMEDATE:
            _ui_drawSettingsTimeDate();
            break;
        // Time&Date settings screen, edit mode
        case SETTINGS_TIMEDATE_SET:
            _ui_drawSettingsTimeDateSet(&ui_state);
            break;
#endif
        // Display settings screen
        case SETTINGS_DISPLAY:
            _ui_drawSettingsDisplay(&ui_state);
            break;
#ifdef CONFIG_GPS
        // GPS settings screen
        case SETTINGS_GPS:
            _ui_drawSettingsGPS(&ui_state);
            break;
#endif
#ifdef CONFIG_M17
        // M17 settings screen
        case SETTINGS_M17:
            _ui_drawSettingsM17(&ui_state);
            break;
#endif
        case SETTINGS_ACCESSIBILITY:
            _ui_drawSettingsAccessibility(&ui_state);
            break;
        // Screen to support resetting Settings and VFO to defaults
        case SETTINGS_RESET2DEFAULTS:
            _ui_drawSettingsReset2Defaults(&ui_state);
            break;
        // Screen to set frequency offset and step
        case SETTINGS_RADIO:
            _ui_drawSettingsRadio(&ui_state);
            break;
        // Low battery screen
        case LOW_BAT:
            _ui_drawLowBatteryScreen();
            break;
    }

    // If MACRO menu is active draw it
    if(macro_menu)
    {
        _ui_drawDarkOverlay();
        _ui_drawMacroMenu(&ui_state);
    }

    redraw_needed = false;
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
