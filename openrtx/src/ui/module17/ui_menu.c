/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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
 *                                                                         *
 *   (2025) Modified by KD0OSS for new modes on Module17                   *
 ***************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>
#include <ui/ui_mod17.h>
#include <interfaces/nvmem.h>
#include <interfaces/cps_io.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <memory_profiling.h>
#include <hwconfig.h>

/* UI main screen helper functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainBottom();


const char *mic_gain_values[] =
{
    "40 dB",
    "50 dB",
    "60 dB"
};

const char *phase_values[] =
{
    "Normal",
    "Inverted"
};

const char *hwVersions[] =
{
    "0.1d",
    "0.1e",
    "1.0"
};

const char *hmiVersions[] =
{
    "None",
    "1.0"
};

const char *bbTuningPot[] =
{
    "Soft",
    "Hard"
};

void _ui_drawMenuList(uint8_t selected, int (*getCurrentEntry)(char *buf, uint8_t max_len, uint8_t index))
{
    point_t pos = layout.line1_pos;
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen = (CONFIG_SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    color_t text_color = color_white;
    for(int item=0, result=0; (result == 0) && (pos.y < CONFIG_SCREEN_HEIGHT); item++)
    {
        // If selection is off the screen, scroll screen
        if(selected >= entries_in_screen)
            scroll = selected - entries_in_screen + 1;
        // Call function pointer to get current menu entry string
        result = (*getCurrentEntry)(entry_buf, sizeof(entry_buf), item+scroll);
        if(result != -1)
        {
            text_color = color_white;
            if(item + scroll == selected)
            {
                text_color = color_black;
                // Draw rectangle under selected item, compensating for text height
                point_t rect_pos = {0, pos.y - layout.menu_h + 3};
                gfx_drawRect(rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h, color_white, true);
            }
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_LEFT, text_color, entry_buf);
            pos.y += layout.menu_h;
        }
    }
}

void _ui_drawMenuListValue(ui_state_t* ui_state, uint8_t selected,
                           int (*getCurrentEntry)(char *buf, uint8_t max_len, uint8_t index),
                           int (*getCurrentValue)(char *buf, uint8_t max_len, uint8_t index))
{
    point_t pos = layout.line1_pos;
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen = (CONFIG_SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    char value_buf[MAX_ENTRY_LEN] = "";
    color_t text_color = color_white;
    for(int item=0, result=0; (result == 0) && (pos.y < CONFIG_SCREEN_HEIGHT); item++)
    {
        // If selection is off the screen, scroll screen
        if(selected >= entries_in_screen)
            scroll = selected - entries_in_screen + 1;
        // Call function pointer to get current menu entry string
        result = (*getCurrentEntry)(entry_buf, sizeof(entry_buf), item+scroll);
        // Call function pointer to get current entry value string
        result = (*getCurrentValue)(value_buf, sizeof(value_buf), item+scroll);
        if(result != -1)
        {
            text_color = color_white;
            if(item + scroll == selected)
            {
                // Draw rectangle under selected item, compensating for text height
                // If we are in edit mode, draw a hollow rectangle
                text_color = color_black;
                bool full_rect = true;
                if(ui_state->edit_mode)
                {
                    text_color = color_white;
                    full_rect = false;
                }
                point_t rect_pos = {0, pos.y - layout.menu_h + 3};
                gfx_drawRect(rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h, color_white, full_rect);
            }
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_LEFT, text_color, entry_buf);
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_RIGHT, text_color, value_buf);
            pos.y += layout.menu_h;
        }
    }
}

bool _ui_viewSubString(char *in_string, char *out_string, uint16_t start_pos, uint16_t num_chars)
{
    uint16_t totalLen = strlen(in_string);
    if(start_pos >= totalLen || (num_chars + start_pos) > totalLen)
        return false;

    memset(out_string, 0, num_chars+1);

    uint16_t i;
    for(i=0;i<num_chars;i++)
    {
        out_string[i] = in_string[start_pos + i];
        // replace tab, line feed and carriage return with space
        if(out_string[i] == 0x09 || out_string[i] == 0x0a ||
            out_string[i] == 0x0d || out_string[i] == '_')
            out_string[i] = 0x20;
    }
    out_string[i] = 0;

    return true;
}

int _ui_getMenuTopEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    uint8_t maxEntries = menu_num;
    if(platform_getHwInfo()->hw_version < 1)
        maxEntries -= 1;

    if(index >= maxEntries) return -1;
    snprintf(buf, max_len, "%s", menu_items[index]);
    return 0;
}

int _ui_getSettingsEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_num) return -1;
    snprintf(buf, max_len, "%s", settings_items[index]);
    return 0;
}

int _ui_getDisplayEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= display_num) return -1;
    snprintf(buf, max_len, "%s", display_items[index]);
    return 0;
}

int _ui_getDisplayValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= display_num) return -1;
    uint8_t value = 0;
    switch(index)
    {
        case D_BRIGHTNESS:
            value = last_state.settings.brightness;
            break;
    }
    snprintf(buf, max_len, "%d", value);
    return 0;
}

int _ui_getSMSEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= m17sms_num) return -1;
    snprintf(buf, max_len, "%s", m17sms_items[index]);
    return 0;
}

int _ui_getSMSValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= m17sms_num)
        return -1;

    switch(index)
    {
        case M_SMSSEND:
            buf[0] = 0;
            break;

        case M_SMSVIEW:
            buf[0] = 0;
            break;

        case M_SMSMATCHCALL:
            snprintf(buf, max_len, "%s", last_state.settings.m17_sms_match_call ? "On" : "Off");
            break;
    }

    return 0;
}

int _ui_getM17EntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= m17_num) return -1;
    snprintf(buf, max_len, "%s", m17_items[index]);
    return 0;
}

int _ui_getM17ValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= m17_num) return -1;

    switch(index)
    {
        case M_CALLSIGN:
            snprintf(buf, max_len, "%s", last_state.settings.callsign);
            return 0;
        case M_METATEXT:
            // limit display to 8 characters
            if (strlen(last_state.settings.M17_meta_text) > 8)
            {
                char tmp[9];
                memcpy(tmp, last_state.settings.M17_meta_text, 8);
                tmp[8] = 0;
                // append asterisk to indicate more characters than displayed
                snprintf(buf, max_len, "%s*", tmp);
            }
            else
                snprintf(buf, max_len, "%s", last_state.settings.M17_meta_text);
            return 0;
        case M_SMS:
            buf[0] = 0;
            break;
        case M_CAN:
            snprintf(buf, max_len, "%d", last_state.settings.m17_can);
            break;
        case M_CAN_RX:
            snprintf(buf, max_len, "%s", (last_state.settings.m17_can_rx) ? "On" : "Off");
            break;
    }

    return 0;
}

int _ui_getModule17EntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= module17_num) return -1;
    snprintf(buf, max_len, "%s", module17_items[index]);
    return 0;
}

int _ui_getModule17ValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= module17_num) return -1;

    switch(index)
    {
        case D_MICGAIN:
            snprintf(buf, max_len, "%s", mic_gain_values[mod17CalData.mic_gain]);
            break;
        case D_PTTINLEVEL:
            snprintf(buf, max_len, "%s", mod17CalData.ptt_in_level ? "Act high" : "Act low");
            break;
        case D_PTTOUTLEVEL:
            snprintf(buf, max_len, "%s", mod17CalData.ptt_out_level ? "Act high" : "Act low");
            break;
        case D_TXINVERT:
            snprintf(buf, max_len, "%s", phase_values[mod17CalData.bb_tx_invert]);
            break;
        case D_RXINVERT:
            snprintf(buf, max_len, "%s", phase_values[mod17CalData.bb_rx_invert]);
            break;
        case D_TXWIPER:
            snprintf(buf, max_len, "%d", mod17CalData.tx_wiper);
            break;
        case D_RXWIPER:
            snprintf(buf, max_len, "%d", mod17CalData.rx_wiper);
            break;
    }

    return 0;
}

#ifdef CONFIG_GPS
int _ui_getSettingsGPSEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_gps_num) return -1;
    snprintf(buf, max_len, "%s", settings_gps_items[index]);
    return 0;
}

int _ui_getSettingsGPSValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_gps_num) return -1;
    switch(index)
    {
        case G_ENABLED:
            snprintf(buf, max_len, "%s", (last_state.settings.gps_enabled) ? "ON" : "OFF");
            break;
        case G_SET_TIME:
            snprintf(buf, max_len, "%s", (last_state.gps_set_time) ? "ON" : "OFF");
            break;
        case G_TIMEZONE:
            // Add + prefix to positive numbers
            if(last_state.settings.utc_timezone > 0)
                snprintf(buf, max_len, "+%d", last_state.settings.utc_timezone);
            else
                snprintf(buf, max_len, "%d", last_state.settings.utc_timezone);
            break;
    }
    return 0;
}
#endif

int _ui_getInfoEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= info_num) return -1;
    snprintf(buf, max_len, "%s", info_items[index]);
    return 0;
}

int _ui_getInfoValueName(char *buf, uint8_t max_len, uint8_t index)
{
    const hwInfo_t* hwinfo = platform_getHwInfo();
    if(index >= info_num) return -1;
    switch(index)
    {
        case 0: // Git Version
            snprintf(buf, max_len, "%s", GIT_VERSION);
            break;
        case 1: // Heap usage
            snprintf(buf, max_len, "%dB", getHeapSize() - getCurrentFreeHeap());
            break;
        case 2: // HW Revision
            snprintf(buf, max_len, "%s", hwVersions[hwinfo->hw_version & 0xFF]);
            break;
        case 3: // HMI Version
        #ifdef PLATFORM_LINUX
            snprintf(buf, max_len, "%s", "Linux");
        #else
            if(hwinfo->flags & MOD17_FLAGS_HMI_PRESENT)
                snprintf(buf, max_len, "%s", hmiVersions[(hwinfo->hw_version >> 8) & 0xFF]);
            else
                snprintf(buf, max_len, "%s", hmiVersions[0]);
        #endif
            break;
        case 4: // Baseband tuning potentiometers
        #ifdef PLATFORM_LINUX
            snprintf(buf, max_len, "%s", "Linux");
        #else
            if((hwinfo->flags & MOD17_FLAGS_SOFTPOT) != 0)
                snprintf(buf, max_len, "%s", bbTuningPot[0]);
            else
                snprintf(buf, max_len, "%s", bbTuningPot[1]);
        #endif
            break;
    }
    return 0;
}

void _ui_drawMenuTop(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Menu" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Menu");
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getMenuTopEntryName);
}

#ifdef CONFIG_GPS
void _ui_drawMenuGPS()
{
    char *fix_buf, *type_buf;
    gfx_clearScreen();
    // Print "GPS" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "GPS");
    point_t fix_pos = {layout.line2_pos.x, CONFIG_SCREEN_HEIGHT * 2 / 5};
    // Print GPS status, if no fix, hide details
    if(!last_state.settings.gps_enabled)
        gfx_print(fix_pos, layout.line3_font, TEXT_ALIGN_CENTER,
                  color_white, "GPS OFF");
    else if (last_state.gps_data.fix_quality == 0)
        gfx_print(fix_pos, layout.line3_font, TEXT_ALIGN_CENTER,
                  color_white, "No Fix");
    else if (last_state.gps_data.fix_quality == 6)
        gfx_print(fix_pos, layout.line3_font, TEXT_ALIGN_CENTER,
                  color_white, "Fix Lost");
    else
    {
        switch(last_state.gps_data.fix_quality)
        {
            case 1:
                fix_buf = "SPS";
                break;
            case 2:
                fix_buf = "DGPS";
                break;
            case 3:
                fix_buf = "PPS";
                break;
            default:
                fix_buf = "ERROR";
                break;
        }

        switch(last_state.gps_data.fix_type)
        {
            case 1:
                type_buf = "";
                break;
            case 2:
                type_buf = "2D";
                break;
            case 3:
                type_buf = "3D";
                break;
            default:
                type_buf = "ERROR";
                break;
        }
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, fix_buf);
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, "N     ");
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "%8.6f", last_state.gps_data.latitude);
        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, type_buf);
        // Convert from signed longitude, to unsigned + direction
        float longitude = last_state.gps_data.longitude;
        char *direction = (longitude < 0) ? "W     " : "E     ";
        longitude = (longitude < 0) ? -longitude : longitude;
        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, direction);
        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "%8.6f", longitude);
        gfx_print(layout.bottom_pos, layout.bottom_font, TEXT_ALIGN_CENTER,
                  color_white, "S %4.1fkm/h  A %4.1fm",
                  last_state.gps_data.speed,
                  last_state.gps_data.altitude);
    }
    // Draw compass
    point_t compass_pos = {layout.horizontal_pad * 2, CONFIG_SCREEN_HEIGHT / 2};
    gfx_drawGPScompass(compass_pos,
                       CONFIG_SCREEN_WIDTH / 9 + 2,
                       last_state.gps_data.tmg_true,
                       last_state.gps_data.fix_quality != 0 &&
                       last_state.gps_data.fix_quality != 6);
    // Draw satellites bar graph
    point_t bar_pos = {layout.line3_pos.x + CONFIG_SCREEN_WIDTH * 1 / 3, CONFIG_SCREEN_HEIGHT / 2};
    gfx_drawGPSgraph(bar_pos,
                     (CONFIG_SCREEN_WIDTH * 2 / 3) - layout.horizontal_pad,
                     CONFIG_SCREEN_HEIGHT / 3,
                     last_state.gps_data.satellites,
                     last_state.gps_data.active_sats);
}
#endif

void _ui_drawMenuSettings(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Settings");
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getSettingsEntryName);
}

void _ui_drawMenuInfo(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Info" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Info");
    // Print menu entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getInfoEntryName,
                           _ui_getInfoValueName);
}

void _ui_drawMenuAbout(ui_state_t* ui_state)
{
    gfx_clearScreen();

    point_t openrtx_pos = {layout.horizontal_pad, layout.line3_h};
    gfx_print(openrtx_pos, layout.line3_font, TEXT_ALIGN_CENTER, color_white,
              "OpenRTX");

    point_t pos = {CONFIG_SCREEN_WIDTH / 7, CONFIG_SCREEN_HEIGHT - (layout.menu_h * 3) - 5};
    uint8_t entries_in_screen = (CONFIG_SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t max_scroll = author_num - entries_in_screen;

    if(ui_state->menu_selected >= max_scroll)
        ui_state->menu_selected = max_scroll;

    for(uint8_t item = 0; item < entries_in_screen; item++)
    {
        uint8_t elem = ui_state->menu_selected + item;
        gfx_print(pos, layout.menu_font, TEXT_ALIGN_LEFT, color_white, authors[elem]);
        pos.y += layout.menu_h;
    }
}

void _ui_drawSettingsDisplay(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Display" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Display");
    // Print display settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getDisplayEntryName,
                           _ui_getDisplayValueName);
}

#ifdef CONFIG_GPS
void _ui_drawSettingsGPS(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "GPS Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "GPS Settings");
    // Print display settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected,
                          _ui_getSettingsGPSEntryName,
                          _ui_getSettingsGPSValueName);
}
#endif

#ifdef CONFIG_RTC
void _ui_drawSettingsTimeDate()
{
    gfx_clearScreen();
    datetime_t local_time = utcToLocalTime(last_state.time,
                                           last_state.settings.utc_timezone);
    // Print "Time&Date" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Time&Date");
    // Print current time and date
    gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, "%02d/%02d/%02d",
              local_time.date, local_time.month, local_time.year);
    gfx_print(layout.line3_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, "%02d:%02d:%02d",
              local_time.hour, local_time.minute, local_time.second);
}

void _ui_drawSettingsTimeDateSet(ui_state_t* ui_state)
{
    (void) last_state;

    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Time&Date");
    if(ui_state->input_position <= 0)
    {
        strcpy(ui_state->new_date_buf, "__/__/__");
        strcpy(ui_state->new_time_buf, "__:__:00");
    }
    else
    {
        char input_char = ui_state->input_number + '0';
        // Insert date digit
        if(ui_state->input_position <= 6)
        {
            uint8_t pos = ui_state->input_position -1;
            // Skip "/"
            if(ui_state->input_position > 2) pos += 1;
            if(ui_state->input_position > 4) pos += 1;
            ui_state->new_date_buf[pos] = input_char;
        }
        // Insert time digit
        else
        {
            uint8_t pos = ui_state->input_position -7;
            // Skip ":"
            if(ui_state->input_position > 8) pos += 1;
            ui_state->new_time_buf[pos] = input_char;
        }
    }
    gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, ui_state->new_date_buf);
    gfx_print(layout.line3_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, ui_state->new_time_buf);
}
#endif

void _ui_drawSMSMenu(ui_state_t* ui_state)
{
    gfx_clearScreen();
    point_t top_pos = layout.top_pos;
    point_t top_rect_pos = {0, top_pos.y - layout.menu_h + 3};
    point_t bot_pos = layout.bottom_pos;
    point_t bot_rect_pos = {0, bot_pos.y - layout.menu_h + 3};

    if(ui_state->edit_sms)
    {
        char text[41];
        uint16_t mesgLen = strlen(ui_state->new_message);

        // Draw rectangle under selected item, compensating for text height
        gfx_drawRect(top_rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h, color_white, true);

        gfx_print(top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_black, "SMS Message:");

        if(mesgLen > 40)
            _ui_viewSubString(ui_state->new_message, text, mesgLen - 40, 40);
        else
            strcpy(text, ui_state->new_message);

        gfx_printLine(1, 4, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.message_font,
                      TEXT_ALIGN_CENTER, color_white, text);

        // Print Button Info
        gfx_print(layout.line5_pos, layout.line5_font, TEXT_ALIGN_LEFT,
                  color_white, "Back");
        gfx_print(layout.line5_pos, layout.line5_font, TEXT_ALIGN_RIGHT,
                  color_white, "Send");
    }
    else if(ui_state->view_sms)
    {
        char title[20];
        char sender[10];
        char message[821];
        char text[31];
        uint16_t mesgLen = 0;
        uint16_t curPos = 0;
        uint16_t charsLeft = 0;

        if(state.delSMSMessage)
        {
            rtx_delSMSMessage(state.currentSMSMessage);
            if(state.totalSMSMessages > 0)
                state.totalSMSMessages--;
            state.currentSMSMessage--;
            state.delSMSMessage = false;
        }

        if(state.currentSMSLine < 0)
        {
            state.currentSMSMessage--;
            state.currentSMSLine = 0;
        }
        if(state.currentSMSMessage < 0)
            state.currentSMSMessage = state.totalSMSMessages - 1;

        if(rtx_getSMSMessage(state.currentSMSMessage, sender, message))
        {
            // Draw rectangle under selected item, compensating for text height
            gfx_drawRect(top_rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h, color_white, true);
            sprintf(title, "%s  M#: %d", sender, state.currentSMSMessage + 1);
            gfx_print(top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_black, title);

            gfx_drawRect(bot_rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h, color_white, true);
            strcpy(title, "Del");
            gfx_print(bot_pos, layout.top_font, TEXT_ALIGN_RIGHT, color_black, title);
            strcpy(title, "Back");
            gfx_print(bot_pos, layout.top_font, TEXT_ALIGN_LEFT, color_black, title);
            sprintf(title, "%c  %c", (char)SYMBOL_UP_ARROW, (char)SYMBOL_DOWN_ARROW);
            gfx_drawSymbols(bot_pos, layout.top_symbol_font, TEXT_ALIGN_CENTER, color_black, title);

            mesgLen = strlen(message);
            curPos = 18 * state.currentSMSLine;
            if(curPos >= mesgLen)
                curPos = mesgLen;
            charsLeft = mesgLen - curPos;
            if(charsLeft > 0)
            {
                _ui_viewSubString(message, text, curPos, charsLeft < 18 ? charsLeft : 18);

                gfx_print(layout.line1_pos, layout.message_font, TEXT_ALIGN_LEFT, color_white, text);

                curPos = 17 * (state.currentSMSLine + 1);
                if(curPos >= mesgLen)
                    curPos = mesgLen;
                charsLeft = mesgLen - curPos;
                if(charsLeft > 0)
                {
                    _ui_viewSubString(message, text, curPos, charsLeft < 18 ? charsLeft : 18);

                    gfx_print(layout.line2_pos, layout.message_font, TEXT_ALIGN_LEFT, color_white, text);
                }

                curPos = 17 * (state.currentSMSLine + 2);
                if(curPos >= mesgLen)
                    curPos = mesgLen;
                charsLeft = mesgLen - curPos;
                if(charsLeft > 0)
                {
                    _ui_viewSubString(message, text, curPos, charsLeft < 18 ? charsLeft : 18);

                    gfx_print(layout.line3_pos, layout.message_font, TEXT_ALIGN_LEFT, color_white, text);
                }

                curPos = 17 * (state.currentSMSLine + 3);
                if(curPos >= mesgLen)
                    curPos = mesgLen;
                charsLeft = mesgLen - curPos;
                if(charsLeft > 0)
                {
                    _ui_viewSubString(message, text, curPos, charsLeft < 18 ? charsLeft : 18);

                    gfx_print(layout.line4_pos, layout.message_font, TEXT_ALIGN_LEFT, color_white, text);
                }
            }
            else
            {
                state.currentSMSMessage++;
                if(state.currentSMSMessage >= state.totalSMSMessages)
                    state.currentSMSMessage = 0;
                state.currentSMSLine = 0;
            }
        }
        else
        {
            ui_state->view_sms = false;
            gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
                      color_white, "SMS Menu");

            _ui_drawMenuListValue(ui_state, 0, _ui_getSMSEntryName,
                                  _ui_getSMSValueName);
        }
    }
    else
    {
        if(ui_state->menu_selected > m17sms_num - 1)
            ui_state->menu_selected = 0;

        gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, "SMS Menu");

        _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getSMSEntryName,
                              _ui_getSMSValueName);

        state.currentSMSMessage = 0;
        state.currentSMSLine = 0;
    }
}

void _ui_drawSettingsM17(ui_state_t* ui_state)
{
    gfx_clearScreen();
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "M17 Settings");

    if(ui_state->edit_mode)
    {
        gfx_printLine(1, 4, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                    layout.horizontal_pad, layout.menu_font,
                    TEXT_ALIGN_LEFT, color_white, "Callsign:");

        // uint16_t rect_width = CONFIG_SCREEN_WIDTH - (layout.horizontal_pad * 2);
        // uint16_t rect_height = (CONFIG_SCREEN_HEIGHT - (layout.top_h + layout.bottom_h))/2;
        // point_t rect_origin = {(CONFIG_SCREEN_WIDTH - rect_width) / 2,
        //                        (CONFIG_SCREEN_HEIGHT - rect_height) / 2};
        // gfx_drawRect(rect_origin, rect_width, rect_height, color_white, false);
        // Print M17 callsign being typed
        gfx_printLine(1, 1, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.input_font,
                      TEXT_ALIGN_CENTER, color_white, ui_state->new_callsign);
        // Print Button Info
        gfx_print(layout.line5_pos, layout.line5_font, TEXT_ALIGN_LEFT,
                  color_white, "Cancel");
        gfx_print(layout.line5_pos, layout.line5_font, TEXT_ALIGN_RIGHT,
                  color_white, "Accept");
    }
    else if(ui_state->edit_message)
    {
        gfx_printLine(1, 4, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.menu_font,
                      TEXT_ALIGN_LEFT, color_white, "Message:");

        gfx_printLine(1, 1, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.message_font,
                      TEXT_ALIGN_CENTER, color_white, ui_state->new_message);
        // Print Button Info
        gfx_print(layout.line5_pos, layout.line5_font, TEXT_ALIGN_LEFT,
                  color_white, "Cancel");
        gfx_print(layout.line5_pos, layout.line5_font, TEXT_ALIGN_RIGHT,
                  color_white, "Accept");
    }
    else
    {
        _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getM17EntryName,
                             _ui_getM17ValueName);
    }
}

void _ui_drawSettingsModule17(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Module17 Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Module17 Settings");
    // Print Module17 settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getModule17EntryName,
                           _ui_getModule17ValueName);
}

void _ui_drawSettingsReset2Defaults(ui_state_t* ui_state)
{
    (void) ui_state;

    static int drawcnt = 0;
    static long long lastDraw = 0;

    gfx_clearScreen();
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Reset to Defaults");

    // Make text flash yellow once every 1s
    color_t textcolor = drawcnt % 2 == 0 ? color_white : yellow_fab413;
    gfx_printLine(1, 4, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.top_font,
                  TEXT_ALIGN_CENTER, textcolor, "To reset:");
    gfx_printLine(2, 4, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.top_font,
                  TEXT_ALIGN_CENTER, textcolor, "Press Enter twice");

    if((getTick() - lastDraw) > 1000)
    {
        drawcnt++;
        lastDraw = getTick();
    }
}
