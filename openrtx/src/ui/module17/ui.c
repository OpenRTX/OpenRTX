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
/* UI menu functions, their implementation is in "ui_menu.c" */
extern void _ui_Draw_MenuTop(UI_State_st* ui_state);
#ifdef GPS_PRESENT
extern void _ui_Draw_MenuGPS();
extern void _ui_Draw_SettingsGPS(UI_State_st* ui_state);
#endif
extern void _ui_Draw_MenuSettings(UI_State_st* ui_state);
extern void _ui_Draw_MenuInfo(UI_State_st* ui_state);
extern void _ui_Draw_MenuAbout();
#ifdef RTC_PRESENT
extern void _ui_Draw_SettingsTimeDate();
extern void _ui_Draw_SettingsTimeDateSet(UI_State_st* ui_state);
#endif
extern void _ui_Draw_SettingsDisplay(UI_State_st* ui_state);
extern void _ui_Draw_SettingsM17(UI_State_st* ui_state);
extern void _ui_Draw_SettingsModule17(UI_State_st* ui_state);
extern void _ui_Draw_SettingsReset2Defaults(UI_State_st* ui_state);
extern bool _ui_Draw_MacroMenu(UI_State_st* ui_state);

const char *Page_MenuItems[] =
{
    "Settings",
#ifdef GPS_PRESENT
    "GPS",
#endif
    "Info",
    "About",
    "Shutdown"
};

const char *Page_MenuSettings[] =
{
    "Display",
#ifdef RTC_PRESENT
    "Time & Date",
#endif
#ifdef GPS_PRESENT
    "GPS",
#endif
    "M17",
    "Module 17",
    "Default Settings"
};

const char *PAGE_MENU_SETTINGSDisplay[] =
{
#ifdef SCREEN_CONTRAST
    "Contrast",
#endif
    "Timer"
};

const char *m17_items[] =
{
    "Callsign",
    "CAN",
    "CAN RX Check"
};

const char *module17_items[] =
{
    "TX Softpot",
    "RX Softpot",
    "TX Phase",
    "RX Phase",
    "Mic Gain"
};

#ifdef GPS_PRESENT
const char *PAGE_MENU_SETTINGSGPS[] =
{
    "GPS Enabled",
    "GPS Set Time",
    "UTC Timezone"
};
#endif

const char *Page_MenuInfo[] =
{
    "",
    "Used heap",
    "Hw Version"
};

const char *authors[] =
{
    "Niccolo' IU2KIN",
    "Silvano IU2KWO",
    "Federico IU2NUO",
    "Fred IU2NRO",
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

static const char symbols_callsign[] = "_ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890/- ";

// Calculate number of menu entries
const uint8_t menu_num = sizeof(Page_MenuItems)/sizeof(Page_MenuItems[0]);
const uint8_t settings_num = sizeof(Page_MenuSettings)/sizeof(Page_MenuSettings[0]);
const uint8_t display_num = sizeof(PAGE_MENU_SETTINGSDisplay)/sizeof(PAGE_MENU_SETTINGSDisplay[0]);
#ifdef GPS_PRESENT
const uint8_t settings_gps_num = sizeof(PAGE_MENU_SETTINGSGPS)/sizeof(PAGE_MENU_SETTINGSGPS[0]);
#endif
const uint8_t m17_num = sizeof(m17_items)/sizeof(m17_items[0]);
const uint8_t module17_num = sizeof(module17_items)/sizeof(module17_items[0]);
const uint8_t info_num = sizeof(Page_MenuInfo)/sizeof(Page_MenuInfo[0]);
const uint8_t author_num = sizeof(authors)/sizeof(authors[0]);

Layout_st layout;
State_st last_state;
static UI_State_st ui_state;
static bool layout_ready = false;

static bool standby = false;
static long long last_event_tick = 0;

// UI event queue
static uint8_t evQueue_rdPos;
static uint8_t evQueue_wrPos;
static Event_st evQueue[MAX_NUM_EVENTS];

Layout_st _ui_calculateLayout()
{
    // Horizontal line height
    const uint16_t hline_h = 1;
    // Compensate for fonts printing below the start position
    const uint16_t text_v_offset = 1;

    // Calculate UI layout for the Module 17

    // Height and padding shown in diagram at beginning of file
    const uint16_t.lines[ GUI_LINE_TOP ].height = 11;
    const uint16_t top_pad = 1;
    const uint16_t lines[ GUI_LINE_1 ].height = 10;
    const uint16_t lines[ GUI_LINE_2 ].height = 10;
    const uint16_t lines[ GUI_LINE_3 ].height = 10;
    const uint16_t lines[ GUI_LINE_4 ]. = 10;
    const uint16_t line5_h = 10;
    const uint16_t menu_h = 10;
    const uint16_t.lines[ GUI_LINE_BOTTOM ].height = 15;
    const uint16_t bottom_pad = 0;
    const uint16_t status_v_pad = 1;
    const uint16_t small_line_v_pad = 1;
    const uint16_t big_line_v_pad = 0;
    const uint16_t horizontal_pad = 4;

    // Top bar font: 6 pt
    const FontSize_t   top_font = FONT_SIZE_6PT;
    const SymbolSize_t top.symbolSize = SYMBOLS_SIZE_6PT;
    // Middle line fonts: 5, 8, 8 pt
    const FontSize_t line1_font = FONT_SIZE_6PT;
    const SymbolSize_t line1.symbolSize = SYMBOLS_SIZE_6PT;
    const FontSize_t line2_font = FONT_SIZE_6PT;
    const SymbolSize_t line2.symbolSize = SYMBOLS_SIZE_6PT;
    const FontSize_t line3_font = FONT_SIZE_6PT;
    const SymbolSize_t lines[ GUI_LINE_3 ].symbolSize = SYMBOLS_SIZE_6PT;
    const FontSize_t line4_font = FONT_SIZE_6PT;
    const SymbolSize_t lines[ GUI_LINE_4 ].symbolSize = SYMBOLS_SIZE_6PT;
    const FontSize_t line5_font = FONT_SIZE_6PT;
    const SymbolSize_t line5.symbolSize = SYMBOLS_SIZE_6PT;
    // Bottom bar font: 6 pt
    const FontSize_t bottom_font = FONT_SIZE_6PT;
    // TimeDate/Frequency input font
    const FontSize_t input_font.size = FONT_SIZE_8PT;
    // Menu font
    const FontSize_t menu_font.size = FONT_SIZE_6PT;
    // Mode screen frequency font: 9 pt
    const FontSize_t mode_font_big.size = FONT_SIZE_9PT;
    // Mode screen details font: 6 pt
    const FontSize_t mode_font_small.size = FONT_SIZE_6PT;

    // Calculate printing positions
    Pos_st top_pos    = {horizontal_pad,.lines[ GUI_LINE_TOP ].height - status_v_pad - text_v_offset};
    Pos_st line1_pos  = {horizontal_pad,.lines[ GUI_LINE_TOP ].height + top_pad + lines[ GUI_LINE_1 ].height - small_line_v_pad - text_v_offset};
    Pos_st line2_pos  = {horizontal_pad,.lines[ GUI_LINE_TOP ].height + top_pad + lines[ GUI_LINE_1 ].height + lines[ GUI_LINE_2 ].height - small_line_v_pad - text_v_offset};
    Pos_st line3_pos  = {horizontal_pad,.lines[ GUI_LINE_TOP ].height + top_pad + lines[ GUI_LINE_1 ].height + lines[ GUI_LINE_2 ].height + lines[ GUI_LINE_3 ].height - big_line_v_pad - text_v_offset};
    Pos_st line4_pos  = {horizontal_pad,.lines[ GUI_LINE_TOP ].height + top_pad + lines[ GUI_LINE_1 ].height + lines[ GUI_LINE_2 ].height + lines[ GUI_LINE_3 ].height + lines[ GUI_LINE_4 ]. - big_line_v_pad - text_v_offset};
    Pos_st line5_pos  = {horizontal_pad,.lines[ GUI_LINE_TOP ].height + top_pad + lines[ GUI_LINE_1 ].height + lines[ GUI_LINE_2 ].height + lines[ GUI_LINE_3 ].height + lines[ GUI_LINE_4 ]. + line5_h - big_line_v_pad - text_v_offset};
    Pos_st bottom_pos = {horizontal_pad, SCREEN_HEIGHT - bottom_pad - status_v_pad - text_v_offset};

    Layout_st new_layout =
    {
        hline_h,
       .lines[ GUI_LINE_TOP ].height,
        lines[ GUI_LINE_1 ].height,
        lines[ GUI_LINE_2 ].height,
        lines[ GUI_LINE_3 ].height,
        lines[ GUI_LINE_4 ].,
        line5_h,
        menu_h,
       .lines[ GUI_LINE_BOTTOM ].height,
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
        top.symbolSize,
        line1_font,
        line1.symbolSize,
        line2_font,
        line2.symbolSize,
        line3_font,
        lines[ GUI_LINE_3 ].symbolSize,
        line4_font,
        lines[ GUI_LINE_4 ].symbolSize,
        line5_font,
        line5.symbolSize,
        bottom_font,
        input_font.size,
        menu_font.size,
        mode_font_big.size,
        mode_font_small.size
    };
    return new_layout;
}


void ui_init()
{
    last_event_tick = getTick();
    redraw_needed = true;
    layout = _ui_calculateLayout();
    layout_ready = true;
    // Initialize struct ui_state to all zeroes
    // This syntax is called compound literal
    // https://stackoverflow.com/questions/6891720/initialize-reset-struct-to-zero-null
    ui_state = (const struct UI_State_st){ 0 };
}

void ui_Draw_SplashScreen()
{
    gfx_clearScreen();

    Pos_st origin = {0, (SCREEN_HEIGHT / 2) - 6};
    gfx_print(origin, FONT_SIZE_12PT, GFX_ALIGN_CENTER, yellow_fab413, "O P N\nR T X");
}

freq_t _ui_freq_add_digit(freq_t freq, uint8_t pos, uint8_t number)
{
    freq_t coefficient = 100;
    for(uint8_t i=0; i < FREQ_DIGITS - pos; i++)
    {
        coefficient *= 10;
    }
    return freq += number * coefficient;
}

#ifdef RTC_PRESENT
void _ui_timedate_add_digit(datetime_t *timedate, uint8_t pos, uint8_t number)
{
    switch(pos)
    {
        // Set date
        case 1:
        {
            timedate->date += number * 10;
            break;
        }
        case 2:
        {
            timedate->date += number;
            break;
        }
        // Set month
        case 3:
        {
            timedate->month += number * 10;
            break;
        }
        case 4:
        {
            timedate->month += number;
            break;
        }
        // Set year
        case 5:
        {
            timedate->year += number * 10;
            break;
        }
        case 6:
        {
            timedate->year += number;
            break;
        }
        // Set hour
        case 7:
        {
            timedate->hour += number * 10;
            break;
        }
        case 8:
        {
            timedate->hour += number;
            break;
        }
        // Set minute
        case 9:
        {
            timedate->minute += number * 10;
            break;
        }
        case 10:
        {
            timedate->minute += number;
            break;
        }
    }
}
#endif

bool _ui_freq_check_limits(freq_t freq)
{
    bool valid = false;
    const hwInfo_t* hwinfo = platform_getHwInfo();
    if(hwinfo->vhf_band)
    {
        // hwInfo_t frequencies are in MHz
        if(freq >= (hwinfo->vhf_minFreq * 1000000) &&
           freq <= (hwinfo->vhf_maxFreq * 1000000))
        {
            valid = true;
        }
    }
    if(hwinfo->uhf_band)
    {
        // hwInfo_t frequencies are in MHz
        if(freq >= (hwinfo->uhf_minFreq * 1000000) &&
           freq <= (hwinfo->uhf_maxFreq * 1000000))
        {
            valid = true;
        }
    }
    return valid;
}

bool _ui_channel_valid(channel_t* channel)
{
    return _ui_freq_check_limits(channel->rx_frequency) &&
           _ui_freq_check_limits(channel->tx_frequency);
}

int _ui_fsm_loadChannel(int16_t channel_index, bool *sync_rtx)
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
    if(result != -1 && _ui_channel_valid(&channel))
    {
        // Set new channel index
        state.channel_index = selected_channel;
        // Copy channel read to state
        state.channel = channel;
        *sync_rtx = true;
    }
    return result;
}

void _ui_fsm_confirmVFOInput(bool *sync_rtx)
{
    // Switch to TX input
    if(ui_state.input_set == SET_RX)
    {
        ui_state.input_set = SET_TX;
        // Reset input position
        ui_state.input_position = 0;
    }
    else
    {
        if(ui_state.input_set == SET_TX)
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
            }
            state.ui_screen = PAGE_MAIN_VFO;
        }
    }
}

void _ui_fsm_insertVFONumber(kbd_msg_t msg, bool *sync_rtx)
{
    // Advance input position
    ui_state.input_position += 1;
    // Save pressed number to calculate frequency and show in GUI
    ui_state.input_number = input_getPressedNumber(msg);
    if(ui_state.input_set == SET_RX)
    {
        if(ui_state.input_position == 1)
        {
            ui_state.new_rx_frequency = 0;
        }
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
    else
    {
        if(ui_state.input_set == SET_TX)
        {
            if(ui_state.input_position == 1)
            {
                ui_state.new_tx_frequency = 0;
            }
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
                state.ui_screen = PAGE_MAIN_VFO;
            }
        }
    }
}

void _ui_changeContrast(int variation)
{
    if(variation >= 0)
    {
        state.settings.contrast = (255 - state.settings.contrast < variation) ? 255 : state.settings.contrast + variation;
    }
    else
    {
        state.settings.contrast = (state.settings.contrast < -variation) ? 0 : state.settings.contrast + variation;
    }
    display_setContrast(state.settings.contrast);
}

void _ui_changeTimer(int variation)
{
    if ((state.settings.display_timer == TIMER_OFF && variation < 0) ||
        (state.settings.display_timer == TIMER_1H && variation > 0))
    {
        return;
    }

    state.settings.display_timer += variation;
}

bool _ui_checkStandby(long long time_since_last_event)
{
    if (standby)
    {
        return false;
    }

    switch (state.settings.display_timer)
    {
        case TIMER_OFF:
        {
            return false;
        }
        case TIMER_5S:
        case TIMER_10S:
        case TIMER_15S:
        case TIMER_20S:
        case TIMER_25S:
        case TIMER_30S:
        {
            return time_since_last_event >= (5000 * state.settings.display_timer);
        }
        case TIMER_1M:
        case TIMER_2M:
        case TIMER_3M:
        case TIMER_4M:
        case TIMER_5M:
        {
            return time_since_last_event >= (60000 * (state.settings.display_timer - (TIMER_1M - 1)));
        }
        case TIMER_15M:
        case TIMER_30M:
        case TIMER_45M:
        {
            return time_since_last_event >= (60000 * 15 * (state.settings.display_timer - (TIMER_15M - 1)));
        }
        case TIMER_1H:
        {
            return time_since_last_event >= 60 * 60 * 1000;
        }
    }

    // unreachable code
    return false;
}

void _ui_enterStandby()
{
    if(standby)
    {
        return;
    }

    standby = true;
    redraw_needed = false;
    display_setBacklightLevel(0);
}

bool _ui_exitStandby(long long now)
{
    last_event_tick = now;

    if(!standby)
    {
        return false;
    }
    standby = false;
    redraw_needed = true;
    display_setBacklightLevel(state.settings.brightness);
    return true;
}

void _ui_changeCAN(int variation)
{
    // M17 CAN ranges from 0 to 15
    int8_t can = state.settings.m17_can + variation;
    if(can > 15) can = 0;
    if(can < 0)  can = 15;

    state.settings.m17_can = can;
}

void _ui_changeTxWiper(int variation)
{
    mod17CalData.tx_wiper += variation;

    // Max value for softpot is 0x100, min value is set to 0x001
    if(mod17CalData.tx_wiper > 0x100) mod17CalData.tx_wiper = 0x100;
    if(mod17CalData.tx_wiper < 0x001) mod17CalData.tx_wiper = 0x001;
}

void _ui_changeRxWiper(int variation)
{
    mod17CalData.rx_wiper += variation;

    // Max value for softpot is 0x100, min value is set to 0x001
    if(mod17CalData.rx_wiper > 0x100) mod17CalData.rx_wiper = 0x100;
    if(mod17CalData.rx_wiper < 0x001) mod17CalData.rx_wiper = 0x001;
}

void _ui_changeTxInvert(int variation)
{
    // Inversion can be 1 or 0, bit field value ensures no overflow
    mod17CalData.tx_invert += variation;
}

void _ui_changeRxInvert(int variation)
{
    // Inversion can be 1 or 0, bit field value ensures no overflow
    mod17CalData.rx_invert += variation;
}

void _ui_changeMicGain(int variation)
{
    int8_t gain = mod17CalData.mic_gain + variation;
    if(gain > 2) gain = 0;
    if(gain < 0) gain = 2;

    mod17CalData.mic_gain = gain;
}

void _ui_menuUp(uint8_t menu_entries)
{
    uint8_t maxEntries = menu_entries - 1;
    uint8_t ver = platform_getHwInfo()->hw_version;

    // Hide the "shutdown" main menu entry for versions lower than 0.1e
    if((ver < 1) && (state.ui_screen == PAGE_MENU_TOP))
    {
        maxEntries -= 1;
    }
    if(ui_state.entrySelected > 0)
    {
        ui_state.entrySelected -= 1;
    }
    else
    {
        ui_state.entrySelected = maxEntries;
    }
}

void _ui_menuDown(uint8_t menu_entries)
{
   uint8_t maxEntries = menu_entries - 1;
   uint8_t ver = platform_getHwInfo()->hw_version;

    // Hide the "shutdown" main menu entry for versions lower than 0.1e
    if((ver < 1) && (state.ui_screen == PAGE_MENU_TOP))
    {
        maxEntries -= 1;
    }
    if(ui_state.entrySelected < maxEntries)
    {
        ui_state.entrySelected += 1;
    }
    else
    {
        ui_state.entrySelected = 0;
    }
}

void _ui_menuBack(uint8_t prev_state)
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
        ui_state.entrySelected = 0;
    }
}

void _ui_textInputReset(char *buf)
{
    ui_state.input_number = 0;
    ui_state.input_position = 0;
    ui_state.input_set = 0;
    ui_state.last_keypress = 0;
    memset(buf, 0, 9);
}

void _ui_textInputKeypad(char *buf, uint8_t max_len, kbd_msg_t msg, bool callsign)
{
    if(ui_state.input_position >= max_len)
    {
        return;
    }
    long long now = getTick();
    // Get currently pressed number key
    uint8_t num_key = input_getPressedNumber(msg);
    // Get number of symbols related to currently pressed key
    uint8_t num_symbols = 0;
    if(callsign)
    {
        num_symbols = strlen(symbols_ITU_T_E161_callsign[num_key]);
    }
    else
    {
        num_symbols = strlen(symbols_ITU_T_E161[num_key]);
    }
    // Skip keypad logic for first keypress
    if(ui_state.last_keypress != 0)
    {
        // Same key pressed and timeout not expired: cycle over chars of current key
        if((ui_state.input_number == num_key) && ((now - ui_state.last_keypress) < input_longPressTimeout))
        {
            ui_state.input_set = (ui_state.input_set + 1) % num_symbols;
        }
        // Differnt key pressed: save current char and change key
        else
        {
            ui_state.input_position += 1;
            ui_state.input_set = 0;
        }
    }
    // Show current character on buffer
    if(callsign)
    {
        buf[ui_state.input_position] = symbols_ITU_T_E161_callsign[num_key][ui_state.input_set];
    }
    else
    {
        buf[ui_state.input_position] = symbols_ITU_T_E161[num_key][ui_state.input_set];
    }
    // Update reference values
    ui_state.input_number = num_key;
    ui_state.last_keypress = now;
}

void _ui_textInputArrows(char *buf, uint8_t max_len, kbd_msg_t msg)
{
    if(ui_state.input_position >= max_len)
    {
        return;
    }

    uint8_t num_symbols = 0;
    num_symbols = strlen(symbols_callsign);

    if (msg.keys & KEY_RIGHT)
    {
        ui_state.input_position = (ui_state.input_position + 1) % max_len;
        ui_state.input_set = 0;
    }
    else
    {
        if (msg.keys & KEY_LEFT)
        {
            ui_state.input_position = (ui_state.input_position - 1) % max_len;
            ui_state.input_set = 0;
        }
        else
        {
            if (msg.keys & KEY_UP)
            {
                ui_state.input_set = (ui_state.input_set + 1) % num_symbols;
            }
            else
            {
                if (msg.keys & KEY_DOWN)
                {
                    ui_state.input_set = ui_state.input_set==0 ? num_symbols-1 : ui_state.input_set-1;
                }
            }
        }
    }
    buf[ui_state.input_position] = symbols_callsign[ui_state.input_set];
}

void _ui_textInputConfirm(char *buf)
{
    buf[ui_state.input_position + 1] = '\0';
}

void _ui_textInputDel(char *buf)
{
    buf[ui_state.input_position] = '\0';
    // Move back input cursor
    if(ui_state.input_position > 0)
        ui_state.input_position--;
    // If we deleted the initial character, reset starting condition
    else
        ui_state.last_keypress = 0;
    ui_state.input_set = 0;
}

void ui_saveState()
{
    last_state = state;
}

void ui_updateFSM( bool* sync_rtx , Event_st* event )
{
    // Check for events
    if(evQueue_wrPos == evQueue_rdPos) return;

    // Pop an event from the queue
    uint8_t newTail = (evQueue_rdPos + 1) % MAX_NUM_EVENTS;
    Event_st event  = evQueue[evQueue_rdPos];
    evQueue_rdPos   = newTail;

    // There is some event to process, we need an UI redraw.
    // UI redraw request is cancelled if we're in standby mode.
    if(standby) redraw_needed = false;

    long long now = getTick();
    // Process pressed keys
    if(event.type == EVENT_TYPE_KBD)
    {
        kbd_msg_t msg;
        msg.value = event.payload;

        // If we get out of standby, we ignore the kdb event
        // unless is the MONI key for the MACRO functions
        if (_ui_exitStandby(now) && !(msg.keys & KEY_MONI))
        {
            return;
        }
        switch(state.ui_screen)
        {
            // VFO screen
            case PAGE_MAIN_VFO:
            {
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
                    else
                    {
                        if(msg.keys & KEY_ESC)
                        {
                            ui_state.edit_mode = false;
                        }
                        else
                        {
                            _ui_textInputArrows(ui_state.new_callsign, 9, msg);
                        }
                    }
                }
                else
                {
                    if(msg.keys & KEY_ENTER)
                    {
                        // Save current main state
                        ui_state.last_main_state = state.ui_screen;
                        // Open Menu
                        state.ui_screen = PAGE_MENU_TOP;
                    }
                    else if (msg.keys & KEY_RIGHT)
                    {
                        ui_state.edit_mode = true;
                    }
                }
                break;
            }
            // Top menu screen
            case PAGE_MENU_TOP:
            {
                if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                {
                    _ui_menuUp(menu_num);
                }
                else
                {
                    if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    {
                        _ui_menuDown(menu_num);
                    }
                    else
                    {
                        if(msg.keys & KEY_ENTER)
                        {
                            switch(ui_state.entrySelected)
                            {
                                case M_SETTINGS:
                                {
                                    state.ui_screen = PAGE_MENU_SETTINGS;
                                    break;
                                }
                                case M_INFO:
                                {
                                    state.ui_screen = PAGE_MENU_INFO;
                                    break;
                                }
                                case M_ABOUT:
                                {
                                    state.ui_screen = PAGE_ABOUT;
                                    break;
                                }
                                case M_SHUTDOWN:
                                {
                                    state.devStatus = SHUTDOWN;
                                    break;
                                }
                            }
                            // Reset menu selection
                            ui_state.entrySelected = 0;
                        }
                        else
                        {
                            if(msg.keys & KEY_ESC)
                            {
                                _ui_menuBack(ui_state.last_main_state);
                            }
                        }
                    }
                }
                break;
            }
            // Settings menu screen
            case PAGE_MENU_SETTINGS:
            {
                if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                {
                    _ui_menuUp(settings_num);
                }
                else
                {
                    if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    {
                        _ui_menuDown(settings_num);
                    }
                    else
                    {
                        if(msg.keys & KEY_ENTER)
                        {
                            switch(ui_state.entrySelected)
                            {
                                case S_DISPLAY:
                                {
                                    state.ui_screen = PAGE_SETTINGS_DISPLAY;
                                    break;
                                }
                                case S_M17:
                                {
                                    state.ui_screen = PAGE_SETTINGS_M17;
                                    break;
                                }
                                case S_MOD17:
                                {
                                    state.ui_screen = PAGE_SETTINGS_MODULE17;
                                    break;
                                }
                                case S_RESET2DEFAULTS:
                                {
                                    state.ui_screen = PAGE_SETTINGS_RESET_TO_DEFAULTS;
                                    break;
                                }
                                default:
                                {
                                    state.ui_screen = PAGE_MENU_SETTINGS;
                                }
                            }
                            // Reset menu selection
                            ui_state.entrySelected = 0;
                        }
                        else
                        {
                            if(msg.keys & KEY_ESC)
                            {
                                _ui_menuBack(PAGE_MENU_TOP);
                            }
                        }
                    }
                }
                break;
            }
            // Info menu screen
            case PAGE_MENU_INFO:
            {
                if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                {
                    _ui_menuUp(info_num);
                }
                else
                {
                    if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    {
                        _ui_menuDown(info_num);
                    }
                    else
                    {
                        if(msg.keys & KEY_ESC)
                        {
                        _   ui_menuBack(PAGE_MENU_TOP);
                        }
                    }
                }
                break;
            }
            // About screen
            case PAGE_ABOUT:
            {
                if(msg.keys & KEY_ESC)
                {
                    _ui_menuBack(PAGE_MENU_TOP);
                }
                break;
            }
            case PAGE_SETTINGS_DISPLAY:
            {
                if(msg.keys & KEY_LEFT)
                {
                    switch(ui_state.entrySelected)
                    {
#ifdef SCREEN_CONTRAST
                        case D_CONTRAST:
                        {
                            _ui_changeContrast(-4);
                            break;
                        }
#endif
                        case D_TIMER:
                        {
                            _ui_changeTimer(-1);
                            break;
                        }
                        default:
                        {
                            state.ui_screen = PAGE_SETTINGS_DISPLAY;
                        }
                    }
                }
                else
                {
                    if(msg.keys & KEY_RIGHT)
                    {
                        switch(ui_state.entrySelected)
                        {
#ifdef SCREEN_CONTRAST
                            case D_CONTRAST:
                            {
                                _ui_changeContrast(+4);
                                break;
                            }
#endif
                            case D_TIMER:
                            {
                                _ui_changeTimer(+1);
                                break;
                            }
                            default:
                            {
                                state.ui_screen = PAGE_SETTINGS_DISPLAY;
                            }
                        }
                    }
                    else
                    {
                        if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                        {
                            _ui_menuUp(display_num);
                        }
                        else
                        {
                            if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                            {
                                _ui_menuDown(display_num);
                            }
                            else
                            {
                                if(msg.keys & KEY_ESC)
                                {
                                    nvm_writeSettings(&state.settings);
                                    _ui_menuBack(PAGE_MENU_SETTINGS);
                                }
                            }
                        }
                    }
                }
                break;
            }
            // M17 Settings
            case PAGE_SETTINGS_M17:
            {
                if(ui_state.edit_mode)
                {
                    if(msg.keys & KEY_ENTER)
                    {
                        _ui_textInputConfirm(ui_state.new_callsign);
                        // Save selected callsign and disable input mode
                        strncpy(state.settings.callsign, ui_state.new_callsign, 10);
                        ui_state.edit_mode = false;
                    }
                    else
                    {
                        if(msg.keys & KEY_ESC)
                        {
                            ui_state.edit_mode = false;
                        }
                        else
                        {
                            _ui_textInputArrows(ui_state.new_callsign, 9, msg);
                        }
                    }
                }
                else
                {
                    // Not in edit mode: handle CAN setting
                    if(msg.keys & KEY_LEFT)
                    {
                        switch(ui_state.entrySelected)
                        {
                            case M_CAN:
                            {
                                _ui_changeCAN(-1);
                                break;
                            }
                            case M_CAN_RX:
                            {
                                state.settings.m17_can_rx = !state.settings.m17_can_rx;
                                break;
                            }
                            default:
                            {
                                state.ui_screen = PAGE_SETTINGS_M17;
                            }
                        }
                    }
                    else
                    {
                        if(msg.keys & KEY_RIGHT)
                        {
                            switch(ui_state.entrySelected)
                            {
                                case M_CAN:
                                {
                                    _ui_changeCAN(+1);
                                    break;
                                }
                                case M_CAN_RX:
                                {
                                    state.settings.m17_can_rx = !state.settings.m17_can_rx;
                                    break;
                                }
                                default:
                                {
                                    state.ui_screen = PAGE_SETTINGS_M17;
                                }
                            }
                        }
                        else
                        {
                            if(msg.keys & KEY_ENTER)
                            {
                                switch(ui_state.entrySelected)
                                {
                                    // Enable callsign input
                                    case M_CALLSIGN:
                                    {
                                        ui_state.edit_mode = true;
                                        _ui_textInputReset(ui_state.new_callsign);
                                        break;
                                    }
                                    default:
                                    {
                                        state.ui_screen = PAGE_SETTINGS_M17;
                                    }
                                }
                            }
                            else
                            {
                                if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                                {
                                    _ui_menuUp(m17_num);
                                }
                                else
                                {
                                    if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                                    {
                                        _ui_menuDown(m17_num);
                                    }
                                    else
                                    {
                                        if(msg.keys & KEY_ESC)
                                        {
                                            *sync_rtx = true;
                                            nvm_writeSettings(&state.settings);
                                            _ui_menuBack(PAGE_MENU_SETTINGS);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            }
            case PAGE_SETTINGS_RESET_TO_DEFAULTS:
            {
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
                        _ui_menuBack(PAGE_MENU_SETTINGS);
                    }
                }
                else
                {
                    if(msg.keys & KEY_ENTER)
                    {
                        ui_state.edit_mode = false;

                        // Reset calibration values
                        mod17CalData.tx_wiper  = 0x080;
                        mod17CalData.rx_wiper  = 0x080;
                        mod17CalData.tx_invert = 0;
                        mod17CalData.rx_invert = 0;
                        mod17CalData.mic_gain  = 0;

                        state_resetSettingsAndVfo();
                        nvm_writeSettings(&state.settings);
                        _ui_menuBack(PAGE_MENU_SETTINGS);
                    }
                    else
                    {
                        if(msg.keys & KEY_ESC)
                        {
                            ui_state.edit_mode = false;
                            _ui_menuBack(PAGE_MENU_SETTINGS);
                        }
                    }
                }
                break;
            }
            // Module17 Settings
            case PAGE_SETTINGS_MODULE17:
            {
                if(msg.keys & KEY_LEFT)
                {
                    switch(ui_state.entrySelected)
                    {
                        case D_TXWIPER:
                        {
                            _ui_changeTxWiper(-1);
                            break;
                        }
                        case D_RXWIPER:
                        {
                            _ui_changeRxWiper(-1);
                            break;
                        }
                        case D_TXINVERT:
                        {
                            _ui_changeTxInvert(-1);
                            break;
                        }
                        case D_RXINVERT:
                        {
                            _ui_changeRxInvert(-1);
                            break;
                        }
                        case D_MICGAIN:
                        {
                            _ui_changeMicGain(-1);
                            break;
                        }
                        default:
                        {
                            state.ui_screen = PAGE_SETTINGS_MODULE17;
                        }
                    }
                }
                else
                {
                    if(msg.keys & KEY_RIGHT)
                    {
                        switch(ui_state.entrySelected)
                        {
                            case D_TXWIPER:
                            {
                                _ui_changeTxWiper(+1);
                                break;
                            }
                            case D_RXWIPER:
                            {
                                _ui_changeRxWiper(+1);
                                break;
                            }
                            case D_TXINVERT:
                            {
                                _ui_changeTxInvert(+1);
                                break;
                            }
                            case D_RXINVERT:
                            {
                                _ui_changeRxInvert(+1);
                                break;
                            }
                            case D_MICGAIN:
                            {
                                _ui_changeMicGain(+1);
                                break;
                            }
                            default:
                            {
                                state.ui_screen = PAGE_SETTINGS_MODULE17;
                            }
                        }
                    }
                    else
                    {
                        if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                        {
                            _ui_menuUp(module17_num);
                        }
                        else
                        {
                            if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                            {
                                _ui_menuDown(module17_num);
                            }
                            else
                            {
                                if(msg.keys & KEY_ESC)
                                {
                                    nvm_writeSettings(&state.settings);
                                    _ui_menuBack(PAGE_MENU_SETTINGS);
                                }
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    else
    {
        if(event.type == EVENT_TYPE_STATUS)
        {
            if (platform_getPttStatus() || rtx_rxSquelchOpen())
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
}

bool ui_updateGUI( Event_st* event )
{
    if(redraw_needed == false)
    {
        return false;
    }
    if(!layout_ready)
    {
        layout = _ui_calculateLayout();
        layout_ready = true;
    }
    // Draw current GUI page
    switch(last_state.ui_screen)
    {
        // VFO main screen
        case PAGE_MAIN_VFO:
        {
            _ui_Draw_MainVFO(&ui_state);
            break;
        }
        // Top menu screen
        case PAGE_MENU_TOP:
        {
            _ui_Draw_MenuTop(&ui_state);
            break;
        }
        // Settings menu screen
        case PAGE_MENU_SETTINGS:
        {
            _ui_Draw_MenuSettings(&ui_state);
            break;
        }
        // Info menu screen
        case PAGE_MENU_INFO:
        {
            _ui_Draw_MenuInfo(&ui_state);
            break;
        }
        // About menu screen
        case PAGE_ABOUT:
        {
            _ui_Draw_MenuAbout();
            break;
        }
        // Display settings screen
        case PAGE_SETTINGS_DISPLAY:
        {
            _ui_Draw_SettingsDisplay(&ui_state);
            break;
        }
        // M17 settings screen
        case PAGE_SETTINGS_M17:
        {
            _ui_Draw_SettingsM17(&ui_state);
            break;
        }
        // Module 17 settings screen
        case PAGE_SETTINGS_MODULE17:
        {
            _ui_Draw_SettingsModule17(&ui_state);
            break;
        }
        // Screen to support resetting Settings and VFO to defaults
        case PAGE_SETTINGS_RESET_TO_DEFAULTS:
        {
            _ui_Draw_SettingsReset2Defaults(&ui_state);
            break;
        }
    }

    redraw_needed = false;
    return true;
}

bool ui_pushEvent(const uint8_t type, const uint32_t data)
{
    uint8_t newHead = (evQueue_wrPos + 1) % MAX_NUM_EVENTS;

    // Queue is full
    if(newHead == evQueue_rdPos)
    {
        return false;
    }

    // Preserve atomicity when writing the new element into the queue.
    Event_st event;
    event.type    = type;
    event.payload = data;

    evQueue[evQueue_wrPos] = event;
    evQueue_wrPos = newHead;

    return true;
}

void ui_terminate()
{
}
