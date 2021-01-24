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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ui.h>
#include <interfaces/nvmem.h>

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
            point_t rect_pos = {0, pos.y - layout.top_h + (layout.text_v_offset*2)};
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

int _ui_getZoneName(char *buf, uint8_t max_len, uint8_t index)
{
    zone_t zone;
    int result = nvm_readZoneData(&zone, index);
    if(result != -1)
        snprintf(buf, max_len, "%s", zone.name);
    return result;
}

int _ui_getChannelName(char *buf, uint8_t max_len, uint8_t index)
{
    channel_t channel;
    int result = nvm_readChannelData(&channel, index);
    if(result != -1)
        snprintf(buf, max_len, "%s", channel.name);
    return result;
}

int _ui_getContactName(char *buf, uint8_t max_len, uint8_t index)
{
    contact_t contact;
    int result = nvm_readContactData(&contact, index);
    if(result != -1)
        snprintf(buf, max_len, "%s", contact.name);
    return result;
}

void _ui_drawCPSList(point_t pos, uint8_t selected, int (*f)(char *buf, uint8_t max_len, uint8_t index))
{
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen = ((SCREEN_HEIGHT - pos.y) / layout.top_h) + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    int result = 0;
    for(int item=0; (result == 0) && (pos.y < SCREEN_HEIGHT); item++)
    {
        // If selection is off the screen, scroll screen
        if(selected >= entries_in_screen)
            scroll = selected - entries_in_screen + 1;
        // Call function pointer to get CPS element name
        result = (*f)(entry_buf, MAX_ENTRY_LEN, item+scroll);
        if(result != -1)
        {
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
}

void _ui_drawMenuTop(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Menu" on top bar
    gfx_print(layout.top_left, "Menu", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print menu entries
    _ui_drawMenuList(layout.line1_left, menu_items, menu_num, ui_state->menu_selected);
}

void _ui_drawMenuZone(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Zone" on top bar
    gfx_print(layout.top_left, "Zone", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print zone entries
    _ui_drawCPSList(layout.line1_left, ui_state->menu_selected, _ui_getZoneName);
}

void _ui_drawMenuChannel(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Channel" on top bar
    gfx_print(layout.top_left, "Channels", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print channel entries
    _ui_drawCPSList(layout.line1_left, ui_state->menu_selected, _ui_getChannelName);
}

void _ui_drawMenuContacts(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Contacts" on top bar
    gfx_print(layout.top_left, "Contacts", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print contact entries
    _ui_drawCPSList(layout.line1_left, ui_state->menu_selected, _ui_getContactName);
}

void _ui_drawMenuSettings(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Settings" on top bar
    gfx_print(layout.top_left, "Settings", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print menu entries
    _ui_drawMenuList(layout.line1_left, settings_items, settings_num, ui_state->menu_selected);
}

#ifdef HAS_RTC
void _ui_drawSettingsTimeDate(state_t* last_state)
{
    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print(layout.top_left, "Time&Date", layout.top_font,
              TEXT_ALIGN_CENTER, color_white);
    // Print current time and date
    char date_buf[10] = "";
    char time_buf[10] = "";
    snprintf(date_buf, sizeof(date_buf), "%02d/%02d/%02d", 
             last_state->time.date, last_state->time.month, last_state->time.year);
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", 
             last_state->time.hour, last_state->time.minute, last_state->time.second);
    gfx_print(layout.line2_left, date_buf, layout.line2_font, TEXT_ALIGN_CENTER,
              color_white);
    gfx_print(layout.line3_left, time_buf, layout.line3_font, TEXT_ALIGN_CENTER,
              color_white);
}

void _ui_drawSettingsTimeDateSet(state_t* last_state, ui_state_t* ui_state)
{
    (void) last_state;

    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print(layout.top_left, "Time&Date", layout.top_font,
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
    gfx_print(layout.line2_left, ui_state->new_date_buf, layout.line2_font, TEXT_ALIGN_CENTER,
              color_white);
    gfx_print(layout.line3_left, ui_state->new_time_buf, layout.line3_font, TEXT_ALIGN_CENTER,
              color_white);
}
#endif

bool _ui_drawMenuMacro(state_t* last_state) {
        // Header
        gfx_print(layout.top_left, "Macro Menu", layout.top_font, TEXT_ALIGN_CENTER,
                  color_white);
        // First row
        gfx_print(layout.line1_left, "1", layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413);
        char code_str[11] = { 0 };
        snprintf(code_str, 11, "  %6.1f",
                 ctcss_tone[last_state->channel.fm.txTone]/10.0f);
        gfx_print(layout.line1_left, code_str, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white);
        gfx_print(layout.line1_left, "2       ", layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413);
        char encdec_str[9] = { 0 };
        bool tone_tx_enable = last_state->channel.fm.txToneEn;
        bool tone_rx_enable = last_state->channel.fm.rxToneEn;
        if (tone_tx_enable && tone_rx_enable)
            snprintf(encdec_str, 9, "     E+D");
        else if (tone_tx_enable && !tone_rx_enable)
            snprintf(encdec_str, 9, "      E ");
        else if (!tone_tx_enable && tone_rx_enable)
            snprintf(encdec_str, 9, "      D ");
        else
            snprintf(encdec_str, 9, "        ");
        gfx_print(layout.line1_left, encdec_str, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white);
        gfx_print(layout.line1_right, "3        ", layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413);
        char pow_str[9] = { 0 };
        snprintf(pow_str, 9, "%.1gW", last_state->channel.power);
        gfx_print(layout.line1_right, pow_str, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white);
        // Second row
        gfx_print(layout.line2_left, "4", layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413);
        char bw_str[8] = { 0 };
        switch (last_state->channel.bandwidth)
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
        gfx_print(layout.line2_left, bw_str, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white);
        gfx_print(layout.line2_left, "5       ", layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413);
        gfx_print(layout.line2_left, "     GPS", layout.top_font, TEXT_ALIGN_CENTER,
                  color_white);
        gfx_print(layout.line2_right, "6        ", layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413);
        gfx_print(layout.line2_right, "Lck", layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white);
        // Third row
        gfx_print(layout.line3_left, "7", layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413);
        gfx_print(layout.line3_left, "    B+", layout.top_font, TEXT_ALIGN_LEFT,
                  color_white);
        gfx_print(layout.line3_left, "8       ", layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413);
        gfx_print(layout.line3_left, "     B-", layout.top_font, TEXT_ALIGN_CENTER,
                  color_white);
        gfx_print(layout.line3_right, "9        ", layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413);
        gfx_print(layout.line3_right, "Sav", layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white);
    return true;
}
