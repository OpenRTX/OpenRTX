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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <utils.h>
#include <ui/ui_default.h>
#include <interfaces/nvmem.h>
#include <interfaces/cps_io.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <memory_profiling.h>
#include <ui/ui_strings.h>
#include <core/voicePromptUtils.h>

#ifdef PLATFORM_TTWRPLUS
#include <SA8x8.h>
#endif

/* UI main screen helper functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainBottom();

static char priorSelectedMenuName[MAX_ENTRY_LEN] = "\0";
static char priorSelectedMenuValue[MAX_ENTRY_LEN] = "\0";
static bool priorEditMode = false;
static uint32_t lastValueUpdate=0;

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
void _ui_reset_menu_anouncement_tracking()
 {
     *priorSelectedMenuName='\0';
     *priorSelectedMenuValue='\0';
 }

static bool DidSelectedMenuItemChange(char* menuName, char* menuValue)
{
    // menu name can't be empty.
    if ((menuName == NULL) || (*menuName == '\0'))
        return false;

    // If value is supplied it can't be empty but it does not have to be supplied.
    if ((menuValue != NULL) && (*menuValue == '\0'))
        return false;

    if (strcmp(menuName, priorSelectedMenuName) != 0)
    {
        strcpy(priorSelectedMenuName, menuName);
        if (menuValue != NULL)
            strcpy(priorSelectedMenuValue, menuValue);
        else
            *priorSelectedMenuValue = '\0'; // reset it since we've changed menu item.

        return true;
    }

    if ((menuValue != NULL) && (strcmp(menuValue, priorSelectedMenuValue) != 0))
    {
        // avoid chatter when value changes rapidly!
    uint32_t now=getTick();
        
        uint32_t interval=now - lastValueUpdate;
        lastValueUpdate = now;
        if (interval < 1000)
            return false;
        strcpy(priorSelectedMenuValue, menuValue);
        return true;
    }

    return false;
}
/*
Normally we determine if we should say the word menu if a menu entry has no 
associated value that can be changed.
There are some menus however with no associated value which are not submenus, 
e.g. the entries under Channels, contacts, Info, 
which are navigable but not modifyable.
*/
static bool ScreenContainsReadOnlyEntries(int menuScreen)
{
    switch (menuScreen)
    {
    case MENU_CHANNEL:
    case MENU_CONTACTS:
    case MENU_INFO:
        return true;
    }
    return false;
}
 
static void announceMenuItemIfNeeded(char* name, char* value, bool editMode)
{
    if (state.settings.vpLevel < vpLow)
        return;

    if ((name == NULL) || (*name == '\0'))
        return;

    if (DidSelectedMenuItemChange(name, value) == false)
        return;

    // Stop any prompt in progress and/or clear the buffer.
    vp_flush();

    vp_announceText(name, vpqDefault);
// We determine if we should say the word Menu as follows:
// The name has no  associated value ,
// i.e. does not represent a modifyable name/value pair.
// We're not in edit mode.
// The screen is navigable but entries  are readonly.
    if (!value && !editMode && !ScreenContainsReadOnlyEntries(state.ui_screen))
        vp_queueStringTableEntry(&currentLanguage->menu);
    
    if (editMode)
        vp_queuePrompt(PROMPT_EDIT);
    if ((value != NULL) && (*value != '\0'))
        vp_announceText(value, vpqDefault);

    vp_play();
}

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
                announceMenuItemIfNeeded(entry_buf, NULL, false);
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
                bool editModeChanged = priorEditMode != ui_state->edit_mode;
                priorEditMode = ui_state->edit_mode;
                // force the menu item to be spoken  when the edit mode changes. 
                // E.g. when pressing Enter on Display Brightness etc.
                if (editModeChanged)
                    priorSelectedMenuName[0]='\0';
                if (!ui_state->edit_mode || editModeChanged)
                {// If in edit mode, only want to speak the char being entered,,
            //not repeat the entire display.
                    announceMenuItemIfNeeded(entry_buf, value_buf, 
                                             ui_state->edit_mode);
                }
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
#ifdef SCREEN_BRIGHTNESS    
        case D_BRIGHTNESS:
            value = last_state.settings.brightness;
            break;
#endif
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

#ifdef GPS_PRESENT
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
            snprintf(buf, max_len, "%s", (last_state.settings.gps_enabled) ?
                                                      currentLanguage->on  :
                                                      currentLanguage->off);
            break;
        case G_SET_TIME:
            snprintf(buf, max_len, "%s", (last_state.gps_set_time) ?
                                               currentLanguage->on :
                                               currentLanguage->off);
            break;
        case G_TIMEZONE:
        {
            int8_t tz_hr = (last_state.settings.utc_timezone / 2);
            int8_t tz_mn = (last_state.settings.utc_timezone % 2) * 5;
            char   sign  = ' ';

            if(last_state.settings.utc_timezone > 0)
            {
                sign = '+';
            }
            else if(last_state.settings.utc_timezone < 0)
            {
                sign   = '-';
                tz_hr *= (-1);
                tz_mn *= (-1);
            }

            snprintf(buf, max_len, "%c%d.%d", sign, tz_hr, tz_mn);
        }
            break;
    }
    return 0;
}
#endif

int _ui_getRadioEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_radio_num) return -1;
    snprintf(buf, max_len, "%s", settings_radio_items[index]);
    return 0;
}

int _ui_getRadioValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_radio_num)
        return -1;

    int32_t offset = 0;
    switch(index)
    {
        case R_OFFSET:
            offset = abs((int32_t)last_state.channel.tx_frequency -
                         (int32_t)last_state.channel.rx_frequency);
            snprintf(buf, max_len, "%gMHz", (float) offset / 1000000.0f);
            break;

        case R_DIRECTION:
            buf[0] = (last_state.channel.tx_frequency >= last_state.channel.rx_frequency) ? '+' : '-';
            buf[1] = '\0';
            break;

        case R_STEP:
            // Print in kHz if it is smaller than 1MHz
            if (freq_steps[last_state.step_index] < 1000000)
                snprintf(buf, max_len, "%gkHz", (float) freq_steps[last_state.step_index] / 1000.0f);
            else
                snprintf(buf, max_len, "%gMHz", (float) freq_steps[last_state.step_index] / 1000000.0f);
            break;
    }

    return 0;
}

int _ui_getM17EntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_m17_num) return -1;
    snprintf(buf, max_len, "%s", settings_m17_items[index]);
    return 0;
}

int _ui_getM17ValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_m17_num)
        return -1;

    switch(index)
    {
        case M17_CALLSIGN:
            snprintf(buf, max_len, "%s", last_state.settings.callsign);
            break;

        case M17_CAN:
            snprintf(buf, max_len, "%d", last_state.settings.m17_can);
            break;
        case M17_CAN_RX:
            snprintf(buf, max_len, "%s", (last_state.settings.m17_can_rx) ?
                                                           currentLanguage->on :
                                                           currentLanguage->off);
            break;
    }

    return 0;
}

int _ui_getAccessibilityEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_accessibility_num) return -1;
    snprintf(buf, max_len, "%s", settings_accessibility_items[index]);
    return 0;
}

int _ui_getAccessibilityValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_accessibility_num) return -1;
    uint8_t value = 0;
    switch(index)
    {
        case A_LEVEL:
        {
            value = last_state.settings.vpLevel;
            switch (value)
            {
                case vpNone:
                    snprintf(buf, max_len, "%s", currentLanguage->off);
                    break;
                case vpBeep:
                    snprintf(buf, max_len, "%s", currentLanguage->beep);
                    break;
                default:
                                    snprintf(buf, max_len, "%d", (value-vpBeep));
                    break;
            }
            break;
        }
        case A_PHONETIC:
            snprintf(buf, max_len, "%s", last_state.settings.vpPhoneticSpell ? currentLanguage->on : currentLanguage->off);
            break;
        case A_MACRO_LATCH:
            snprintf(buf, max_len, "%s", last_state.settings.macroMenuLatch ? currentLanguage->on : currentLanguage->off);
            break;
    }
    return 0;
}

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
        case 4: // Heap usage
            snprintf(buf, max_len, "%dB", getHeapSize() - getCurrentFreeHeap());
            break;
        case 5: // Band
            snprintf(buf, max_len, "%s %s", hwinfo->vhf_band ? currentLanguage->VHF : "", hwinfo->uhf_band ? currentLanguage->UHF : "");
            break;
        case 6: // VHF
            snprintf(buf, max_len, "%d - %d", hwinfo->vhf_minFreq, hwinfo->vhf_maxFreq);
            break;
        case 7: // UHF
            snprintf(buf, max_len, "%d - %d", hwinfo->uhf_minFreq, hwinfo->uhf_maxFreq);
            break;
        case 8: // LCD Type
            snprintf(buf, max_len, "%d", hwinfo->hw_version);
            break;
        #ifdef PLATFORM_TTWRPLUS
        case 9: // Radio model
            strncpy(buf, sa8x8_getModel(), max_len);
            break;
        case 10: // Radio firmware version
        {
            // Get FW version string, skip the first nine chars ("sa8x8-fw/")
            uint8_t major, minor, patch, release;
            const char *fwVer = sa8x8_getFwVersion();

            sscanf(fwVer, "sa8x8-fw/v%hhu.%hhu.%hhu.r%hhu", &major, &minor, &patch, &release);
            snprintf(buf, max_len,"v%hhu.%hhu.%hhu.r%hhu", major, minor, patch, release);
        }
            break;
        #endif
    }
    return 0;
}

int _ui_getBankName(char *buf, uint8_t max_len, uint8_t index)
{
    int result = 0;
    // First bank "All channels" is not read from flash
    if(index == 0)
    {
        strncpy(buf, currentLanguage->allChannels, max_len);
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
              color_white, currentLanguage->menu);
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getMenuTopEntryName);
}

void _ui_drawMenuBank(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Bank" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->banks);
    // Print bank entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getBankName);
}

void _ui_drawMenuChannel(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Channel" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->channels);
    // Print channel entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getChannelName);
}

void _ui_drawMenuContacts(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Contacts" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->contacts);
    // Print contact entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getContactName);
}

#ifdef GPS_PRESENT
void _ui_drawMenuGPS()
{
    char *fix_buf, *type_buf;
    gfx_clearScreen();
    // Print "GPS" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->gps);
    point_t fix_pos = {layout.line2_pos.x, SCREEN_HEIGHT * 2 / 5};
    // Print GPS status, if no fix, hide details
    if(!last_state.settings.gps_enabled)
        gfx_print(fix_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->gpsOff);
    else if (last_state.gps_data.fix_quality == 0)
        gfx_print(fix_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->noFix);
    else if (last_state.gps_data.fix_quality == 6)
        gfx_print(fix_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->fixLost);
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
                fix_buf = (char*)currentLanguage->error;
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
                type_buf = (char*)currentLanguage->error;
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
    point_t bar_pos = {layout.line3_large_pos.x + SCREEN_WIDTH * 1 / 3, SCREEN_HEIGHT / 2};
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
              color_white, currentLanguage->settings);
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getSettingsEntryName);
}

void _ui_drawMenuBackupRestore(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Backup & Restore" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->backupAndRestore);
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getBackupRestoreEntryName);
}

void _ui_drawMenuBackup(ui_state_t* ui_state)
{
    (void) ui_state;

    gfx_clearScreen();
    // Print "Flash Backup" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->flashBackup);
    // Print backup message
    point_t line = layout.line2_pos;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->connectToRTXTool);
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->toBackupFlashAnd);
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->pressPTTToStart);

   if (!platform_getPttStatus())
        return;

    state.devStatus     = DATATRANSFER;
    state.backup_eflash = true;
}

void _ui_drawMenuRestore(ui_state_t* ui_state)
{
    (void) ui_state;

    gfx_clearScreen();
    // Print "Flash Restore" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->flashRestore);
    // Print backup message
    point_t line = layout.line2_pos;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->connectToRTXTool);
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->toRestoreFlashAnd);
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->pressPTTToStart);

    if (!platform_getPttStatus())
        return;

    state.devStatus      = DATATRANSFER;
    state.restore_eflash = true;
}

void _ui_drawMenuInfo(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Info" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->info);
    // Print menu entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getInfoEntryName,
                           _ui_getInfoValueName);
}

void _ui_drawMenuAbout()
{
    gfx_clearScreen();

    point_t logo_pos;
    if(SCREEN_HEIGHT >= 100)
    {
        logo_pos.x = 0;
        logo_pos.y = SCREEN_HEIGHT / 5;
        gfx_print(logo_pos, FONT_SIZE_12PT, TEXT_ALIGN_CENTER, yellow_fab413,
                  "O P N\nR T X");
    }
    else
    {
        logo_pos.x = layout.horizontal_pad;
        logo_pos.y = layout.line3_large_h;
        gfx_print(logo_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, currentLanguage->openRTX);
    }

    uint8_t line_h = layout.menu_h;
    point_t pos = {SCREEN_WIDTH / 7, SCREEN_HEIGHT - (line_h * (author_num - 1)) - 5};
    for(int author = 0; author < author_num; author++)
    {
        gfx_print(pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, "%s", *(&currentLanguage->Niccolo + author));
        pos.y += line_h;
    }
}

void _ui_drawSettingsDisplay(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Display" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->display);
    // Print display settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getDisplayEntryName,
                           _ui_getDisplayValueName);
}

#ifdef GPS_PRESENT
void _ui_drawSettingsGPS(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "GPS Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->gpsSettings);
    // Print display settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected,
                          _ui_getSettingsGPSEntryName,
                          _ui_getSettingsGPSValueName);
}
#endif

#ifdef RTC_PRESENT
void _ui_drawSettingsTimeDate()
{
    gfx_clearScreen();
    datetime_t local_time = utcToLocalTime(last_state.time,
                                           last_state.settings.utc_timezone);
    // Print "Time&Date" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->timeAndDate);
    // Print current time and date
    gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, "%02d/%02d/%02d",
              local_time.date, local_time.month, local_time.year);
    gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, "%02d:%02d:%02d",
              local_time.hour, local_time.minute, local_time.second);
}

void _ui_drawSettingsTimeDateSet(ui_state_t* ui_state)
{
    (void) last_state;

    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->timeAndDate);
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
    gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, ui_state->new_time_buf);
}
#endif

void _ui_drawSettingsM17(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "M17 Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->m17settings);
    gfx_printLine(1, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.menu_font,
                  TEXT_ALIGN_LEFT, color_white, currentLanguage->callsign);
    if((ui_state->edit_mode) && (ui_state->menu_selected == M17_CALLSIGN))
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
        _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getM17EntryName,
                              _ui_getM17ValueName);
    }
}

void _ui_drawSettingsAccessibility(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Accessibility" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->accessibility);
    // Print accessibility settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getAccessibilityEntryName,
                           _ui_getAccessibilityValueName);
}

void _ui_drawSettingsReset2Defaults(ui_state_t* ui_state)
{
    (void) ui_state;

    static int drawcnt = 0;
    static long long lastDraw = 0;

    gfx_clearScreen();
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->resetToDefaults);

    // Make text flash yellow once every 1s
    color_t textcolor = drawcnt % 2 == 0 ? color_white : yellow_fab413;
    gfx_printLine(1, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.top_font,
                  TEXT_ALIGN_CENTER, textcolor, currentLanguage->toReset);
    gfx_printLine(2, 4, layout.top_h, SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.top_font,
                  TEXT_ALIGN_CENTER, textcolor, currentLanguage->pressEnterTwice);

    if((getTick() - lastDraw) > 1000)
    {
        drawcnt++;
        lastDraw = getTick();
    }

    drawcnt++;
}

void _ui_drawSettingsRadio(ui_state_t* ui_state)
{
    gfx_clearScreen();

    // Print "Radio Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->radioSettings);

    // Handle the special case where a frequency is being input
    if ((ui_state->menu_selected == R_OFFSET) && (ui_state->edit_mode))
    {
        char buf[17] = { 0 };
        uint16_t rect_width = SCREEN_WIDTH - (layout.horizontal_pad * 2);
        uint16_t rect_height = (SCREEN_HEIGHT - (layout.top_h + layout.bottom_h))/2;
        point_t rect_origin = {(SCREEN_WIDTH - rect_width) / 2,
                               (SCREEN_HEIGHT - rect_height) / 2};

        gfx_drawRect(rect_origin, rect_width, rect_height, color_white, false);

        // Print frequency with the most sensible unit
        if (ui_state->new_offset < 1000)
            snprintf(buf, 17, "%dHz", ui_state->new_offset);
        else if (ui_state->new_offset < 1000000)
            snprintf(buf, 17, "%gkHz", (float) ui_state->new_offset / 1000.0f);
        else
            snprintf(buf, 17, "%gMHz", (float) ui_state->new_offset / 1000000.0f);

        gfx_printLine(1, 1, layout.top_h, SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.input_font,
                      TEXT_ALIGN_CENTER, color_white, buf);
    }
    else
    {
        // Print radio settings entries
        _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getRadioEntryName,
                               _ui_getRadioValueName);
    }
}

void _ui_drawMacroTop()
{
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
                color_white, currentLanguage->macroMenu);
    if (macro_latched)
    {
        gfx_drawSymbol(layout.top_pos, layout.top_symbol_size, TEXT_ALIGN_LEFT,
                       color_white, SYMBOL_ALPHA_M_BOX_OUTLINE);
    }
    if(last_state.settings.gps_enabled)
    {
        if(last_state.gps_data.fix_quality > 0)
        {
            gfx_drawSymbol(layout.top_pos, layout.top_symbol_size, TEXT_ALIGN_RIGHT,
                           color_white, SYMBOL_CROSSHAIRS_GPS);
        } 
        else
        {
            gfx_drawSymbol(layout.top_pos, layout.top_symbol_size, TEXT_ALIGN_RIGHT,
                           color_white, SYMBOL_CROSSHAIRS);
        }
    }
}

bool _ui_drawMacroMenu(ui_state_t* ui_state)
{
        // Header
        _ui_drawMacroTop();
        // First row
        if (last_state.channel.mode == OPMODE_FM)
        {
/*
 * If we have a keyboard installed draw all numbers, otherwise draw only the
 * currently selected number.
 */
#if defined(UI_NO_KEYBOARD)
            if (ui_state->macro_menu_selected == 0)
#endif // UI_NO_KEYBOARD
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      yellow_fab413, "1");
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, "   T-");
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, "     %7.1f",
                      ctcss_tone[last_state.channel.fm.txTone]/10.0f);
#if defined(UI_NO_KEYBOARD)
            if (ui_state->macro_menu_selected == 1)
#endif // UI_NO_KEYBOARD
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                      yellow_fab413, "2");
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                      color_white,   "       T+");
        }
        else if (last_state.channel.mode == OPMODE_M17)
        {
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      yellow_fab413, "1");
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, "          ");
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                      yellow_fab413, "2");
        }
#if defined(UI_NO_KEYBOARD)
            if (ui_state->macro_menu_selected == 2)
#endif // UI_NO_KEYBOARD
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413, "3        ");
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
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                      color_white, encdec_str);
        }
        else if (last_state.channel.mode == OPMODE_M17)
        {
            char encdec_str[9] = "        ";
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                      color_white, encdec_str);
        }
        // Second row
        // Calculate symmetric second row position, line2_pos is asymmetric like main screen
        point_t pos_2 = {layout.line1_pos.x, layout.line1_pos.y +
                        (layout.line3_large_pos.y - layout.line1_pos.y)/2};
#if defined(UI_NO_KEYBOARD)
            if (ui_state->macro_menu_selected == 3)
#endif // UI_NO_KEYBOARD
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413, "4");
        if (last_state.channel.mode == OPMODE_FM)
        {
            char bw_str[12] = { 0 };
            switch (last_state.channel.bandwidth)
            {
                case BW_12_5:
                    snprintf(bw_str, 12, "   BW 12.5");
                    break;
                case BW_20:
                    snprintf(bw_str, 12, "   BW  20 ");
                    break;
                case BW_25:
                    snprintf(bw_str, 12, "   BW  25 ");
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
#if defined(UI_NO_KEYBOARD)
            if (ui_state->macro_menu_selected == 4)
#endif // UI_NO_KEYBOARD
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, "5");
        char mode_str[12] = "";
        switch(last_state.channel.mode)
        {
            case OPMODE_FM:
            snprintf(mode_str, 12,"         FM");
            break;
            case OPMODE_DMR:
            snprintf(mode_str, 12,"        DMR");
            break;
            case OPMODE_M17:
            snprintf(mode_str, 12,"        M17");
            break;
        }
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, mode_str);
#if defined(UI_NO_KEYBOARD)
            if (ui_state->macro_menu_selected == 5)
#endif // UI_NO_KEYBOARD
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413, "6        ");
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "%.1gW", dBmToWatt(last_state.channel.power));
        // Third row
#if defined(UI_NO_KEYBOARD)
            if (ui_state->macro_menu_selected == 6)
#endif // UI_NO_KEYBOARD
        gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413, "7");
#ifdef SCREEN_BRIGHTNESS                  
        gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, "   B-");
        gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, "       %5d",
                  state.settings.brightness);
#endif
#if defined(UI_NO_KEYBOARD)
            if (ui_state->macro_menu_selected == 7)
#endif // UI_NO_KEYBOARD
        gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, "8");
#ifdef SCREEN_BRIGHTNESS                  
        gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white,   "       B+");
#endif
#if defined(UI_NO_KEYBOARD)
            if (ui_state->macro_menu_selected == 8)
#endif // UI_NO_KEYBOARD
        gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413, "9        ");
        if( ui_state->input_locked == true )
           gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                     color_white, "Unlk");
        else
           gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                     color_white, "Lck");

        // Draw S-meter bar
        _ui_drawMainBottom();
        return true;
}
