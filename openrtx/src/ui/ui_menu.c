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
#include <ui.h>
#include <interfaces/nvmem.h>
#include <interfaces/platform.h>

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
            gfx_print(pos, entry_buf, layout.menu_font, TEXT_ALIGN_LEFT, text_color);
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
            gfx_print(pos, entry_buf, layout.menu_font, TEXT_ALIGN_LEFT, text_color);
            gfx_print(pos, value_buf, layout.menu_font, TEXT_ALIGN_RIGHT, text_color);
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
            snprintf(buf, max_len, "%s", (last_state.settings.gps_set_time) ? "ON" : "OFF");
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
        case 0: // Battery voltage
            snprintf(buf, max_len, "%.1fV", last_state.v_bat);
            break;
        case 1: // Battery charge
            snprintf(buf, max_len, "%.1f%%", last_state.charge * 100);
            break;
        case 2: // RSSI
            snprintf(buf, max_len, "%.1fdBm", last_state.rssi);
            break;
        case 3: // Model
            snprintf(buf, max_len, "%s", hwinfo->name);
            break;
        case 4: // Band
            snprintf(buf, max_len, "%s %s", hwinfo->vhf_band ? "VHF" : "", hwinfo->uhf_band ? "UHF" : "");
            break;
        case 5: // VHF
            snprintf(buf, max_len, "%d - %d", hwinfo->vhf_minFreq, hwinfo->vhf_maxFreq);
            break;
        case 6: // UHF
            snprintf(buf, max_len, "%d - %d", hwinfo->uhf_minFreq, hwinfo->uhf_maxFreq);
            break;
        case 7: // LCD Type
            snprintf(buf, max_len, "%d", hwinfo->lcd_type);
            break;
    }
    return 0;
}

int _ui_getZoneName(char *buf, uint8_t max_len, uint8_t index)
{
    int result = 0;
    // First zone "All channels" is not read from flash
    if(index == 0)
    {
        snprintf(buf, max_len, "All channels");
    }
    else
    {
        zone_t zone;
        result = nvm_readZoneData(&zone, index);
        if(result != -1)
            snprintf(buf, max_len, "%s", zone.name);
    }
    return result;
}

int _ui_getChannelName(char *buf, uint8_t max_len, uint8_t index)
{
    channel_t channel;
    int result = nvm_readChannelData(&channel, index + 1);
    if(result != -1)
        snprintf(buf, max_len, "%s", channel.name);
    return result;
}

int _ui_getContactName(char *buf, uint8_t max_len, uint8_t index)
{
    contact_t contact;
    int result = nvm_readContactData(&contact, index + 1);
    if(result != -1)
        snprintf(buf, max_len, "%s", contact.name);
    return result;
}

void _ui_drawMenuTop(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Menu" on top bar
    gfx_print(layout.top_pos, "Menu", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getMenuTopEntryName);
}

void _ui_drawMenuZone(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Zone" on top bar
    gfx_print(layout.top_pos, "Zone", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print zone entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getZoneName);
}

void _ui_drawMenuChannel(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Channel" on top bar
    gfx_print(layout.top_pos, "Channels", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print channel entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getChannelName);
}

void _ui_drawMenuContacts(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Contacts" on top bar
    gfx_print(layout.top_pos, "Contacts", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print contact entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getContactName);
}

#ifdef HAS_GPS
void _ui_drawMenuGPS()
{
    char *fix_buf, *type_buf;
    char lat_buf[12] = { 0 };
    char lon_buf[12] = { 0 };
    char data_buf[25] = { 0 };
    gfx_clearScreen();
    // Print "GPS" on top bar
    gfx_print(layout.top_pos, "GPS", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    point_t fix_pos = {layout.line2_pos.x, SCREEN_HEIGHT * 2 / 5};
    // Print GPS status, if no fix, hide details
    if(!last_state.settings.gps_enabled)
        gfx_print(fix_pos, "GPS OFF", layout.line3_font, TEXT_ALIGN_CENTER, color_white);
    else if (last_state.gps_data.fix_quality == 0)
        gfx_print(fix_pos, "No Fix", layout.line3_font, TEXT_ALIGN_CENTER, color_white);
    else if (last_state.gps_data.fix_quality == 6)
        gfx_print(fix_pos, "Fix Lost", layout.line3_font, TEXT_ALIGN_CENTER, color_white);
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
        gfx_print(layout.line1_pos, fix_buf, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white);
        gfx_print(layout.line1_pos, "N     ", layout.top_font, TEXT_ALIGN_CENTER,
                  color_white);
        snprintf(lat_buf, 12, "%8.6f", last_state.gps_data.latitude);
        gfx_print(layout.line1_pos, lat_buf, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white);
        gfx_print(layout.line2_pos, type_buf, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white);
        // Convert from signed longitude, to unsigned + direction
        float longitude = last_state.gps_data.longitude;
        char *direction = (longitude < 0) ? "W     " : "E     ";
        longitude = (longitude < 0) ? -longitude : longitude;
        gfx_print(layout.line2_pos, direction, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white);
        snprintf(lon_buf, 12, "%8.6f", longitude);
        gfx_print(layout.line2_pos, lon_buf, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white);
        snprintf(data_buf, 25, "S %4.1fkm/h  A %4.1fm",
                 last_state.gps_data.speed,
                 last_state.gps_data.altitude);
        gfx_print(layout.bottom_pos, data_buf, layout.bottom_font, TEXT_ALIGN_CENTER,
                  color_white);
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
    gfx_print(layout.top_pos, "Settings", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getSettingsEntryName);
}

void _ui_drawMenuInfo(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Info" on top bar
    gfx_print(layout.top_pos, "Info", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
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
        gfx_print(openrtx_pos, "OpenRTX", layout.line3_font, TEXT_ALIGN_CENTER, color_white);
    char author_buf[MAX_ENTRY_LEN] = "";
    uint8_t line_h = layout.menu_h;
    point_t pos = {SCREEN_WIDTH / 7, SCREEN_HEIGHT - (line_h * (author_num - 1)) - 5};
    for(int author = 0; author < author_num; author++)
    {
        snprintf(author_buf, MAX_ENTRY_LEN, "%s", authors[author]);
        gfx_print(pos, author_buf, layout.top_font, TEXT_ALIGN_LEFT, color_white);
        pos.y += line_h;
    }
}

void _ui_drawSettingsDisplay(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Display" on top bar
    gfx_print(layout.top_pos, "Display", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print display settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getDisplayEntryName,
                           _ui_getDisplayValueName);
}

#ifdef HAS_GPS
void _ui_drawSettingsGPS(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "GPS Settings" on top bar
    gfx_print(layout.top_pos, "GPS Settings", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
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
    // Print "Time&Date" on top bar
    gfx_print(layout.top_pos, "Time&Date", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print current time and date
    char date_buf[10] = "";
    char time_buf[10] = "";
    snprintf(date_buf, sizeof(date_buf), "%02d/%02d/%02d", 
             last_state.time.date, last_state.time.month, last_state.time.year);
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", 
             last_state.time.hour, last_state.time.minute, last_state.time.second);
    gfx_print(layout.line2_pos, date_buf, layout.input_font, TEXT_ALIGN_CENTER,
              color_white);
    gfx_print(layout.line3_pos, time_buf, layout.input_font, TEXT_ALIGN_CENTER,
              color_white);
}

void _ui_drawSettingsTimeDateSet(ui_state_t* ui_state)
{
    (void) last_state;

    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print(layout.top_pos, "Time&Date", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
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
    gfx_print(layout.line2_pos, ui_state->new_date_buf, layout.input_font, TEXT_ALIGN_CENTER,
              color_white);
    gfx_print(layout.line3_pos, ui_state->new_time_buf, layout.input_font, TEXT_ALIGN_CENTER,
              color_white);
}
#endif

bool _ui_drawMacroMenu() {
        // Header
        gfx_print(layout.top_pos, "Macro Menu", layout.top_font, TEXT_ALIGN_CENTER,
                  color_white);
        // First row
        gfx_print(layout.line1_pos, "1", layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413);
        char code_str[11] = { 0 };
        snprintf(code_str, 11, "  %6.1f",
                 ctcss_tone[last_state.channel.fm.txTone]/10.0f);
        gfx_print(layout.line1_pos, code_str, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white);
        gfx_print(layout.line1_pos, "2       ", layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413);
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
        gfx_print(layout.line1_pos, encdec_str, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white);
        gfx_print(layout.line1_pos, "3        ", layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413);
        char pow_str[9] = { 0 };
        snprintf(pow_str, 9, "%.1gW", last_state.channel.power);
        gfx_print(layout.line1_pos, pow_str, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white);
        // Second row
        // Calculate symmetric second row position, line2_pos is asymmetric like main screen
        point_t pos_2 = {layout.line1_pos.x, layout.line1_pos.y + (layout.line3_pos.y - layout.line1_pos.y)/2};
        gfx_print(pos_2, "4", layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413);
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
        gfx_print(pos_2, bw_str, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white);
        gfx_print(pos_2, "5       ", layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413);
        char mode_str[9] = "";
        switch(last_state.channel.mode)
        {
            case FM:
            snprintf(mode_str, 9,"      FM");
            break;
            case DMR:
            snprintf(mode_str, 9,"     DMR");
            break;
        }
        gfx_print(pos_2, mode_str, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white);
        gfx_print(pos_2, "6        ", layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413);
        gfx_print(pos_2, "Lck", layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white);
        // Third row
        gfx_print(layout.line3_pos, "7", layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413);
        gfx_print(layout.line3_pos, "    B+", layout.top_font, TEXT_ALIGN_LEFT,
                  color_white);
        gfx_print(layout.line3_pos, "8       ", layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413);
        gfx_print(layout.line3_pos, "     B-", layout.top_font, TEXT_ALIGN_CENTER,
                  color_white);
        gfx_print(layout.line3_pos, "9        ", layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413);
        gfx_print(layout.line3_pos, "Sav", layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white);
        // Smeter bar
        float rssi = last_state.rssi;
        float squelch = last_state.sqlLevel / 16.0f;
        point_t smeter_pos = { layout.horizontal_pad,
                               layout.bottom_pos.y +
                               layout.status_v_pad +
                               layout.text_v_offset -
                               layout.bottom_h };
        gfx_drawSmeter(smeter_pos,
                       SCREEN_WIDTH - 2 * layout.horizontal_pad,
                       layout.bottom_h - 1,
                       rssi,
                       squelch,
                       yellow_fab413);

        return true;
}
