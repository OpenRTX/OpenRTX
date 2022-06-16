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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <backup.h>
#include <utils.h>
#include <ui.h>
#include <interfaces/nvmem.h>
#include <interfaces/cps_io.h>
#include <interfaces/platform.h>

/* UI main screen helper functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainBottom();

const char *display_timer_values[] =
{
    "Off",
    "5 s",
    "10 s",
    "15 s",
    "20 s",
    "25 s",
    "30 s",
    "1 min",
    "2 min",
    "3 min",
    "4 min",
    "5 min",
    "15 min",
    "30 min",
    "45 min",
    "1 hour"
};

void _ui_drawMenuList(uint8_t selected, int (*getCurrentEntry)(char *buf, uint8_t max_len, uint8_t index))
{
    point_t pos = layout.line1_pos;
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen = (SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    color_t text_color = color_white;
    for(int item=0, result=0; (result == 0) && (pos.y < SCREEN_HEIGHT); item++)
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
                gfx_drawRect(rect_pos, SCREEN_WIDTH, layout.menu_h, color_white, true);
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
    uint8_t entries_in_screen = (SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    char value_buf[MAX_ENTRY_LEN] = "";
    color_t text_color = color_white;
    for(int item=0, result=0; (result == 0) && (pos.y < SCREEN_HEIGHT); item++)
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
                gfx_drawRect(rect_pos, SCREEN_WIDTH, layout.menu_h, color_white, full_rect);
            }
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_LEFT, text_color, entry_buf);
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_RIGHT, text_color, value_buf);
            pos.y += layout.menu_h;
        }
    }
}

int _ui_getMenuTopEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= menu_num) return -1;
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
#ifdef SCREEN_CONTRAST
        case D_CONTRAST:
            value = last_state.settings.contrast;
            break;
#endif
        case D_TIMER:
            snprintf(buf, max_len, "%s",
                     display_timer_values[last_state.settings.display_timer]);
            return 0;
    }
    snprintf(buf, max_len, "%d", value);
    return 0;
}

#ifdef HAS_GPS
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

int _ui_getBackupRestoreEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= backup_restore_num) return -1;
    snprintf(buf, max_len, "%s", backup_restore_items[index]);
    return 0;
}

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
        case 1: // Battery voltage
        {
            // Compute integer part and mantissa of voltage value, adding 50mV
            // to mantissa for rounding to nearest integer
            uint16_t volt  = (last_state.v_bat + 50) / 1000;
            uint16_t mvolt = ((last_state.v_bat - volt * 1000) + 50) / 100;
            snprintf(buf, max_len, "%d.%dV", volt, mvolt);
        }
            break;
        case 2: // Battery charge
            snprintf(buf, max_len, "%d%%", last_state.charge);
            break;
        case 3: // RSSI
            snprintf(buf, max_len, "%.1fdBm", last_state.rssi);
            break;
        case 4: // Model
            snprintf(buf, max_len, "%s", hwinfo->name);
            break;
        case 5: // Band
            snprintf(buf, max_len, "%s %s", hwinfo->vhf_band ? "VHF" : "", hwinfo->uhf_band ? "UHF" : "");
            break;
        case 6: // VHF
            snprintf(buf, max_len, "%d - %d", hwinfo->vhf_minFreq, hwinfo->vhf_maxFreq);
            break;
        case 7: // UHF
            snprintf(buf, max_len, "%d - %d", hwinfo->uhf_minFreq, hwinfo->uhf_maxFreq);
            break;
        case 8: // LCD Type
            snprintf(buf, max_len, "%d", hwinfo->lcd_type);
            break;
    }
    return 0;
}

int _ui_getBankName(char *buf, uint8_t max_len, uint8_t index)
{
    int result = 0;
    // First bank "All channels" is not read from flash
    if(index == 0)
    {
        snprintf(buf, max_len, "All channels");
    }
    else
    {
        bankHdr_t bank;
        result = cps_readBankHeader(&bank, index - 1);
        if(result != -1)
            snprintf(buf, max_len, "%s", bank.name);
    }
    return result;
}

int _ui_getChannelName(char *buf, uint8_t max_len, uint8_t index)
{
    channel_t channel;
    int result = cps_readChannel(&channel, index);
    if(result != -1)
        snprintf(buf, max_len, "%s", channel.name);
    return result;
}

int _ui_getContactName(char *buf, uint8_t max_len, uint8_t index)
{
    contact_t contact;
    int result = cps_readContact(&contact, index);
    if(result != -1)
        snprintf(buf, max_len, "%s", contact.name);
    return result;
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

void _ui_drawMenuBank(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Bank" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Bank");
    // Print bank entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getBankName);
}

void _ui_drawMenuChannel(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Channel" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Channels");
    // Print channel entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getChannelName);
}

void _ui_drawMenuContacts(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Contacts" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Contacts");
    // Print contact entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getContactName);
}

#ifdef HAS_GPS
void _ui_drawMenuGPS()
{
    char *fix_buf, *type_buf;
    gfx_clearScreen();
    // Print "GPS" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "GPS");
    point_t fix_pos = {layout.line2_pos.x, SCREEN_HEIGHT * 2 / 5};
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
    point_t compass_pos = {layout.horizontal_pad * 2, SCREEN_HEIGHT / 2};
    gfx_drawGPScompass(compass_pos,
                       SCREEN_WIDTH / 9 + 2,
                       last_state.gps_data.tmg_true,
                       last_state.gps_data.fix_quality != 0 &&
                       last_state.gps_data.fix_quality != 6);
    // Draw satellites bar graph
    point_t bar_pos = {layout.line3_pos.x + SCREEN_WIDTH * 1 / 3, SCREEN_HEIGHT / 2};
    gfx_drawGPSgraph(bar_pos,
                     (SCREEN_WIDTH * 2 / 3) - layout.horizontal_pad,
                     SCREEN_HEIGHT / 3,
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

void _ui_drawMenuBackupRestore(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Backup & Restore" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Backup & Restore");
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getBackupRestoreEntryName);
}

void _ui_drawMenuBackup(ui_state_t* ui_state)
{
    (void) ui_state;

    gfx_clearScreen();
    // Print "Flash Backup" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Flash Backup");
    // Print backup message
    point_t line = layout.line2_pos;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, "Connect to RTXTool");
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, "to backup flash and");
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, "press PTT to start.");

    // Shutdown RF stage
    state.rtxShutdown = true;

    if(platform_getPttStatus() == 1)
    {
        platform_ledOn(GREEN);
        #if !defined(PLATFORM_LINUX) && !defined(PLATFORM_MOD17)
        eflash_dump();
        platform_terminate();
        NVIC_SystemReset();
        #endif
    }
}

void _ui_drawMenuRestore(ui_state_t* ui_state)
{
    (void) ui_state;

    gfx_clearScreen();
    // Print "Flash Backup" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Flash Restore");
    // Print backup message
    point_t line = layout.line2_pos;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, "Connect to RTXTool");
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, "to restore flash and");
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, "press PTT to start.");

    // Shutdown RF stage
    state.rtxShutdown = true;

    if(platform_getPttStatus() == 1)
    {
        platform_ledOn(GREEN);
        #if !defined(PLATFORM_LINUX) && !defined(PLATFORM_MOD17)
        eflash_restore();
        platform_terminate();
        NVIC_SystemReset();
        #endif
    }
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

void _ui_drawMenuAbout()
{
    gfx_clearScreen();
    point_t openrtx_pos = {layout.horizontal_pad, layout.line3_h};
    if(SCREEN_HEIGHT >= 100)
        ui_drawSplashScreen(false);
    else
        gfx_print(openrtx_pos, layout.line3_font, TEXT_ALIGN_CENTER,
                  color_white, "OpenRTX");
    uint8_t line_h = layout.menu_h;
    point_t pos = {SCREEN_WIDTH / 7, SCREEN_HEIGHT - (line_h * (author_num - 1)) - 5};
    for(int author = 0; author < author_num; author++)
    {
        gfx_print(pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, "%s", authors[author]);
        pos.y += line_h;
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

#ifdef HAS_GPS
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

#ifdef HAS_RTC
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

void _ui_drawSettingsM17(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "M17 Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "M17 Settings");
    gfx_printLine(1, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.menu_font,
                  TEXT_ALIGN_LEFT, color_white, "Callsign:");
    if(ui_state->edit_mode)
    {
        uint16_t rect_width = SCREEN_WIDTH - (layout.horizontal_pad * 2);
        uint16_t rect_height = (SCREEN_HEIGHT - (layout.top_h + layout.bottom_h))/2;
        point_t rect_origin = {(SCREEN_WIDTH - rect_width) / 2,
                               (SCREEN_HEIGHT - rect_height) / 2};
        gfx_drawRect(rect_origin, rect_width, rect_height, color_white, false);
        // Print M17 callsign being typed
        gfx_printLine(1, 1, layout.top_h, SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.input_font,
                      TEXT_ALIGN_CENTER, color_white, ui_state->new_callsign);
    }
    else
    {
        // Print M17 current callsign
        gfx_printLine(1, 1, layout.top_h, SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.input_font,
                      TEXT_ALIGN_CENTER, color_white, last_state.settings.callsign);
    }
}

void _ui_drawSettingsReset2Defaults(ui_state_t* ui_state)
{
    (void) ui_state;

    static int drawcnt = 0;
    gfx_clearScreen();
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Reset to Defaults");

    //text will flash yellow and white based on update rate of screen
    color_t textcolor = drawcnt % 2 == 0 ? color_white : yellow_fab413;
    gfx_printLine(1, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.top_font,
                  TEXT_ALIGN_CENTER, textcolor, "To reset:");
    gfx_printLine(2, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.top_font,
                  TEXT_ALIGN_CENTER, textcolor, "Press Enter twice");
    drawcnt++;
}

bool _ui_drawMacroMenu()
{
        // Header
        gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, "Macro Menu");
        // First row
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413, "1");
        if (last_state.channel.mode == OPMODE_FM)
        {
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, "  %6.1f",
                      ctcss_tone[last_state.channel.fm.txTone]/10.0f);
        }
        else if (last_state.channel.mode == OPMODE_M17)
        {
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, "          ");
        }
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, "2       ");
        if (last_state.channel.mode == OPMODE_FM)
        {
            char encdec_str[9] = { 0 };
            bool tone_tx_enable = last_state.channel.fm.txToneEn;
            bool tone_rx_enable = last_state.channel.fm.rxToneEn;
            if (tone_tx_enable && tone_rx_enable)
                snprintf(encdec_str, 9, "     E+D");
            else if (tone_tx_enable && !tone_rx_enable)
                snprintf(encdec_str, 9, "      E ");
            else if (!tone_tx_enable && tone_rx_enable)
                snprintf(encdec_str, 9, "      D ");
            else
                snprintf(encdec_str, 9, "        ");
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                      color_white, encdec_str);
        }
        else if (last_state.channel.mode == OPMODE_M17)
        {
            char encdec_str[9] = "        ";
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                      color_white, encdec_str);
        }
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413, "3        ");
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "%.1gW", dBmToWatt(last_state.channel.power));
        // Second row
        // Calculate symmetric second row position, line2_pos is asymmetric like main screen
        point_t pos_2 = {layout.line1_pos.x, layout.line1_pos.y +
                        (layout.line3_pos.y - layout.line1_pos.y)/2};
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413, "4");
        if (last_state.channel.mode == OPMODE_FM)
        {
            char bw_str[8] = { 0 };
            switch (last_state.channel.bandwidth)
            {
                case BW_12_5:
                    snprintf(bw_str, 8, "   12.5");
                    break;
                case BW_20:
                    snprintf(bw_str, 8, "     20");
                    break;
                case BW_25:
                    snprintf(bw_str, 8, "     25");
                    break;
            }
            gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, bw_str);
        }
        else if (last_state.channel.mode == OPMODE_M17)
        {
            gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, "       ");

        }
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, "5       ");
        char mode_str[9] = "";
        switch(last_state.channel.mode)
        {
            case OPMODE_FM:
            snprintf(mode_str, 9,"      FM");
            break;
            case OPMODE_DMR:
            snprintf(mode_str, 9,"     DMR");
            break;
            case OPMODE_M17:
            snprintf(mode_str, 9,"     M17");
            break;
        }
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, mode_str);
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413, "6        ");
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "Lck");
        // Third row
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413, "7");
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, "    B+");
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, "8       ");
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, "     B-");
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413, "9        ");
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "Sav");

        // Draw S-meter bar
        _ui_drawMainBottom();
        return true;
}
