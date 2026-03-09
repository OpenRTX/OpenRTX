/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "core/utils.h"
#include "ui/ui_default.h"
#include "interfaces/nvmem.h"
#include "interfaces/cps_io.h"
#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "core/memory_profiling.h"
#include "ui/ui_strings.h"
#include "core/voicePromptUtils.h"

#ifdef PLATFORM_TTWRPLUS
#include "drivers/baseband/SA8x8.h"
#endif

/* UI main screen helper functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainBottom();
extern const char* _ui_getToneEnabledString(bool tone_tx_enable,
                            bool tone_rx_enable, bool use_abbreviation);

static char priorSelectedMenuName[MAX_ENTRY_LEN] = "\0";
static char priorSelectedMenuValue[MAX_ENTRY_LEN] = "\0";
static bool priorEditMode = false;
static uint32_t lastValueUpdate=0;

const char *display_timer_values[] =
{
    "OFF",
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
    sniprintf(buf, max_len, "%s", menu_items[index]);
    return 0;
}

int _ui_getSettingsEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_num) return -1;
    sniprintf(buf, max_len, "%s", settings_items[index]);
    return 0;
}

int _ui_getDisplayEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= display_num) return -1;
    sniprintf(buf, max_len, "%s", display_items[index]);
    return 0;
}

int _ui_getDisplayValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= display_num) return -1;
    uint8_t value = 0;
    switch(index)
    {
#ifdef CONFIG_SCREEN_BRIGHTNESS
        case D_BRIGHTNESS:
            value = last_state.settings.brightness;
            break;
#endif
#ifdef CONFIG_SCREEN_CONTRAST
        case D_CONTRAST:
            value = last_state.settings.contrast;
            break;
#endif
        case D_TIMER:
            sniprintf(buf, max_len, "%s",
                     display_timer_values[last_state.settings.display_timer]);
            return 0;
        case D_BATTERY:
            sniprintf(buf, max_len, "%s",
                     (last_state.settings.showBatteryIcon) ?
                     currentLanguage->on :
                     currentLanguage->off);
	    return 0;
    }
    sniprintf(buf, max_len, "%d", value);
    return 0;
}

#ifdef CONFIG_GPS
int _ui_getSettingsGPSEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_gps_num) return -1;
    sniprintf(buf, max_len, "%s", settings_gps_items[index]);
    return 0;
}

int _ui_getSettingsGPSValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_gps_num) return -1;
    switch(index)
    {
        case G_ENABLED:
            sniprintf(buf, max_len, "%s", (last_state.settings.gps_enabled) ?
                                                      currentLanguage->on  :
                                                      currentLanguage->off);
            break;
#ifdef CONFIG_RTC
        case G_SET_TIME:
            sniprintf(buf, max_len, "%s", (last_state.settings.gpsSetTime) ?
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

            sniprintf(buf, max_len, "%c%d.%d", sign, tz_hr, tz_mn);
        }
            break;
#endif
    }
    return 0;
}
#endif

int _ui_getRadioEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_radio_num) return -1;
    sniprintf(buf, max_len, "%s", settings_radio_items[index]);
    return 0;
}

int _ui_getRadioValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_radio_num)
        return -1;

    // Only returning the sign
    if(index == R_DIRECTION)
    {
        buf[0] = (last_state.channel.tx_frequency >= last_state.channel.rx_frequency) ? '+' : '-';
        buf[1] = '\0';

        return 0;
    }

    // Return an x.y string
    uint32_t value  = 0;
    switch(index)
    {
        case R_OFFSET:
        {
            uint32_t txFreq = last_state.channel.tx_frequency;
            uint32_t rxFreq = last_state.channel.rx_frequency;

            // Yes, we're basically reinventing the abs() here. The problem is
            // that abs() works on signed integers and using it would mean
            // casting values back and forth between signed and unsigned.
            if(txFreq > rxFreq)
                value = txFreq - rxFreq;
            else
                value = rxFreq - txFreq;
        }
            break;

        case R_STEP:
            value = freq_steps[last_state.step_index];
            break;
    }

    uint32_t div    = 1;
    char     prefix = ' ';

    if(value >= 1000000)
    {
        prefix = 'M';
        div    = 1000000;
    }
    else if(value >= 1000)
    {
        prefix = 'k';
        div    = 1000;
    }

    // NOTE: casts are there only to squelch -Wformat warnings on the
    // sniprintf.
    char str[16];
    sniprintf(str, sizeof(str), "%u.%u", (unsigned int)(value / div),
                                        (unsigned int)(value % div));
    stripTrailingZeroes(str);
    sniprintf(buf, max_len, "%s%cHz", str, prefix);

    return 0;
}

#ifdef CONFIG_M17
int _ui_getM17EntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_m17_num) return -1;
    sniprintf(buf, max_len, "%s", settings_m17_items[index]);
    return 0;
}

int _ui_getM17ValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_m17_num)
        return -1;

    switch(index)
    {
        case M17_CALLSIGN:
            sniprintf(buf, max_len, "%s", last_state.settings.callsign);
            break;
        case M17_RX_CAN:
            sniprintf(buf, max_len, "%d", last_state.channel.rx_can);
            break;
        case M17_TX_CAN:
            sniprintf(buf, max_len, "%d", last_state.channel.tx_can);
            break;
        case M17_CAN_RX:
            sniprintf(buf, max_len, "%s", (last_state.settings.m17_can_rx) ?
                                                           currentLanguage->on :
                                                           currentLanguage->off);
            break;
    }

    return 0;
}
#endif

int _ui_getFMEntryName(char* buf, uint8_t max_len, uint8_t index)
{
    if (index >= settings_fm_num)
        return -1;

    sniprintf(buf, max_len, "%s", settings_fm_items[index]);
    return 0;
}

int _ui_getFMValueName(char* buf, uint8_t max_len, uint8_t index)
{
    if (index >= settings_fm_num)
        return -1;

    switch (index) {
        case CTCSS_Tone: {
            uint16_t tone = ctcss_tone[last_state.channel.fm.txTone];
            sniprintf(buf, max_len, "%d.%d", (tone / 10), (tone % 10));
            break;
        }

        case CTCSS_Enabled:
            sniprintf(buf, max_len, "%s",
                    _ui_getToneEnabledString(last_state.channel.fm.txToneEn,
                                             last_state.channel.fm.rxToneEn,
                                             false));
            break;
    }

    return 0;
}

int _ui_getAccessibilityEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_accessibility_num) return -1;
    sniprintf(buf, max_len, "%s", settings_accessibility_items[index]);
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
                    sniprintf(buf, max_len, "%s", currentLanguage->off);
                    break;
                case vpBeep:
                    sniprintf(buf, max_len, "%s", currentLanguage->beep);
                    break;
                default:
                                    sniprintf(buf, max_len, "%d", (value-vpBeep));
                    break;
            }
            break;
        }
        case A_PHONETIC:
            sniprintf(buf, max_len, "%s", last_state.settings.vpPhoneticSpell ? currentLanguage->on : currentLanguage->off);
            break;
        case A_MACRO_LATCH:
            sniprintf(buf, max_len, "%s", last_state.settings.macroMenuLatch ? currentLanguage->on : currentLanguage->off);
            break;
    }
    return 0;
}

int _ui_getBackupRestoreEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= backup_restore_num) return -1;
    sniprintf(buf, max_len, "%s", backup_restore_items[index]);
    return 0;
}

int _ui_getInfoEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= info_num) return -1;
    sniprintf(buf, max_len, "%s", info_items[index]);
    return 0;
}

int _ui_getInfoValueName(char *buf, uint8_t max_len, uint8_t index)
{
    const hwInfo_t* hwinfo = platform_getHwInfo();
    if(index >= info_num) return -1;
    switch(index)
    {
        case 0: // Git Version
            sniprintf(buf, max_len, "%s", GIT_VERSION);
            break;
        case 1: // Battery voltage
        {
            // Compute integer part and mantissa of voltage value, adding 50mV
            // to mantissa for rounding to nearest integer
            uint16_t volt  = (last_state.v_bat + 50) / 1000;
            uint16_t mvolt = ((last_state.v_bat - volt * 1000) + 50) / 100;
            sniprintf(buf, max_len, "%d.%dV", volt, mvolt);
        }
            break;
        case 2: // Battery charge
            sniprintf(buf, max_len, "%d%%", last_state.charge);
            break;
        case 3: // RSSI
            sniprintf(buf, max_len, "%"PRIi32"dBm", last_state.rssi);
            break;
        case 4: // Heap usage
            sniprintf(buf, max_len, "%dB", getHeapSize() - getCurrentFreeHeap());
            break;
        case 5: // Band
            sniprintf(buf, max_len, "%s %s", hwinfo->vhf_band ? currentLanguage->VHF : "", hwinfo->uhf_band ? currentLanguage->UHF : "");
            break;
        case 6: // VHF
            sniprintf(buf, max_len, "%d - %d", hwinfo->vhf_minFreq, hwinfo->vhf_maxFreq);
            break;
        case 7: // UHF
            sniprintf(buf, max_len, "%d - %d", hwinfo->uhf_minFreq, hwinfo->uhf_maxFreq);
            break;
        case 8: // LCD Type
            sniprintf(buf, max_len, "%d", hwinfo->hw_version);
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
            sniprintf(buf, max_len,"v%hhu.%hhu.%hhu.r%hhu", major, minor, patch, release);
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
            sniprintf(buf, max_len, "%s", bank.name);
    }
    return result;
}

int _ui_getChannelName(char *buf, uint8_t max_len, uint8_t index)
{
    channel_t channel;
    int result = cps_readChannel(&channel, index);
    if(result != -1)
        sniprintf(buf, max_len, "%s", channel.name);
    return result;
}

int _ui_getContactName(char *buf, uint8_t max_len, uint8_t index)
{
    contact_t contact;
    int result = cps_readContact(&contact, index);
    if(result != -1)
        sniprintf(buf, max_len, "%s", contact.name);
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

#ifdef CONFIG_GPS
void _ui_drawMenuGPS()
{
    char *fix_buf, *type_buf;
    gfx_clearScreen();
    // Print "GPS" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->gps);
    point_t fix_pos = {layout.line2_pos.x, CONFIG_SCREEN_HEIGHT * 2 / 5};
    // Print GPS status, if no fix, hide details
    if(!last_state.gpsDetected)
        gfx_print(fix_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->noGps);
    else if(!last_state.settings.gps_enabled)
        gfx_print(fix_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->gpsOff);
    else if (last_state.gps_data.fix_quality == FIX_QUALITY_NO_FIX)
        gfx_print(fix_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->noFix);
    else if (last_state.gps_data.fix_quality == FIX_QUALITY_ESTIMATED)
        gfx_print(fix_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->fixLost);
    else
    {
        switch(last_state.gps_data.fix_quality)
        {
            case FIX_QUALITY_GPS:
                fix_buf = "GPS";
                break;
            case FIX_QUALITY_DGPS:
                fix_buf = "DGPS";
                break;
            case FIX_QUALITY_PPS:
                fix_buf = "PPS";
                break;
            case FIX_QUALITY_RTK:
            case FIX_QUALITY_RTK_FLOAT:
                fix_buf = "RTK";
                break;
            default:
                fix_buf = (char*)currentLanguage->error;
                break;
        }

        switch(last_state.gps_data.fix_type)
        {
            case FIX_TYPE_NOT_AVAIL:
                type_buf = "";
                break;
            case FIX_TYPE_2D:
                type_buf = "2D";
                break;
            case FIX_TYPE_3D:
                type_buf = "3D";
                break;
            default:
                type_buf = (char*)currentLanguage->error;
                break;
        }
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, fix_buf);

        // Convert from signed longitude, to unsigned + direction
        int32_t latitude     = abs(last_state.gps_data.latitude);
        uint8_t latitude_int = latitude / 1000000;
        int32_t latitude_dec = latitude % 1000000;
        char *direction_lat  = (last_state.gps_data.latitude < 0) ? "S     " : "N     ";

        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, direction_lat);
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "%d.%.6d", latitude_int, latitude_dec);
        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, type_buf);

        // Convert from signed longitude, to unsigned + direction
        int32_t longitude     = abs(last_state.gps_data.longitude);
        uint8_t longitude_int = longitude / 1000000;
        int32_t longitude_dec = longitude % 1000000;
        char *direction_lon   = (last_state.gps_data.longitude < 0) ? "W     " : "E     ";

        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, direction_lon);
        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "%d.%.6d", longitude_int, longitude_dec);
        gfx_print(layout.bottom_pos, layout.bottom_font, TEXT_ALIGN_CENTER,
                  color_white, "S %dkm/h  A %dm",
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
    point_t bar_pos = {layout.line3_large_pos.x + CONFIG_SCREEN_WIDTH * 1 / 3, CONFIG_SCREEN_HEIGHT / 2};
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

void _ui_drawMenuAbout(ui_state_t* ui_state)
{
    gfx_clearScreen();

    point_t logo_pos;
    if(CONFIG_SCREEN_HEIGHT >= 100)
    {
        logo_pos.x = 0;
        logo_pos.y = CONFIG_SCREEN_HEIGHT / 5;
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
              color_white, currentLanguage->display);
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
              color_white, currentLanguage->gpsSettings);
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

#ifdef CONFIG_M17
void _ui_drawSettingsM17(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "M17 Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, currentLanguage->m17settings);
    gfx_printLine(1, 4, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.menu_font,
                  TEXT_ALIGN_LEFT, color_white, currentLanguage->callsign);
    if((ui_state->edit_mode) && (ui_state->menu_selected == M17_CALLSIGN))
    {
        uint16_t rect_width = CONFIG_SCREEN_WIDTH - (layout.horizontal_pad * 2);
        uint16_t rect_height = (CONFIG_SCREEN_HEIGHT - (layout.top_h + layout.bottom_h))/2;
        point_t rect_origin = {(CONFIG_SCREEN_WIDTH - rect_width) / 2,
                               (CONFIG_SCREEN_HEIGHT - rect_height) / 2};
        gfx_drawRect(rect_origin, rect_width, rect_height, color_white, false);
        // Print M17 callsign being typed
        gfx_printLine(1, 1, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.input_font,
                      TEXT_ALIGN_CENTER, color_white, ui_state->new_callsign);
    }
    else
    {
        _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getM17EntryName,
                              _ui_getM17ValueName);
    }
}
#endif

void _ui_drawSettingsFM(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "FM Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->fm);
    // Print FM settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getFMEntryName,
                          _ui_getFMValueName);
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
    gfx_printLine(1, 4, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.top_font,
                  TEXT_ALIGN_CENTER, textcolor, currentLanguage->toReset);
    gfx_printLine(2, 4, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
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
        uint16_t rect_width = CONFIG_SCREEN_WIDTH - (layout.horizontal_pad * 2);
        uint16_t rect_height = (CONFIG_SCREEN_HEIGHT - (layout.top_h + layout.bottom_h))/2;
        point_t rect_origin = {(CONFIG_SCREEN_WIDTH - rect_width) / 2,
                               (CONFIG_SCREEN_HEIGHT - rect_height) / 2};

        gfx_drawRect(rect_origin, rect_width, rect_height, color_white, false);

        // Print frequency with the most sensible unit
        char     prefix = ' ';
        uint32_t div    = 1;
        if(ui_state->new_offset >= 1000000)
        {
            prefix = 'M';
            div    = 1000000;
        }
        else if(ui_state->new_offset >= 1000)
        {
            prefix = 'k';
            div    = 1000;
        }

        // NOTE: casts are there only to squelch -Wformat warnings on the
        // sniprintf.
        sniprintf(buf, sizeof(buf), "%u.%u", (unsigned int)(ui_state->new_offset / div),
                                            (unsigned int)(ui_state->new_offset % div));
        stripTrailingZeroes(buf);

        gfx_printLine(1, 1, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.input_font,
                      TEXT_ALIGN_CENTER, color_white, "%s%cHz", buf, prefix);
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
#if defined(CONFIG_UI_NO_KEYBOARD)
        if (ui_state->macro_menu_selected == 0)
#endif  // CONFIG_UI_NO_KEYBOARD
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      yellow_fab413, "1");
        if (last_state.channel.mode == OPMODE_FM)
        {
            char encdec_str[9]  = {0};
            sniprintf(encdec_str, 9, "  %s",
                    _ui_getToneEnabledString(last_state.channel.fm.txToneEn,
                                             last_state.channel.fm.rxToneEn,
                                             true));

            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, encdec_str);
            uint16_t tone = ctcss_tone[last_state.channel.fm.txTone];
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, "       %d.%d", (tone / 10), (tone % 10));
#if defined(CONFIG_UI_NO_KEYBOARD)
        if (ui_state->macro_menu_selected == 1)
#endif // CONFIG_UI_NO_KEYBOARD
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, "2");
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white,   "       T-");
    }
#ifdef CONFIG_M17
    else if (last_state.channel.mode == OPMODE_M17)
    {
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413, "1");
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, "          ");
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, "2");
    }
#endif
#if defined(CONFIG_UI_NO_KEYBOARD)
        if (ui_state->macro_menu_selected == 2)
#endif  // CONFIG_UI_NO_KEYBOARD
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                      yellow_fab413, "3        ");
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, " T+");
    }
#ifdef CONFIG_M17
    else if (last_state.channel.mode == OPMODE_M17)
    {
        char encdec_str[9] = "        ";
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, encdec_str);
    }
#endif

    // Second row
    // Calculate symmetric second row position, line2_pos is asymmetric like main screen
    point_t pos_2 = {layout.line1_pos.x, layout.line1_pos.y +
                    (layout.line3_large_pos.y - layout.line1_pos.y)/2};

#if defined(CONFIG_UI_NO_KEYBOARD)
        if (ui_state->macro_menu_selected == 3)
#endif // CONFIG_UI_NO_KEYBOARD
    gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT,
              yellow_fab413, "4");

    if (last_state.channel.mode == OPMODE_FM)
    {
        char bw_str[12] = { 0 };
        switch (last_state.channel.bandwidth)
        {
            case BW_12_5:
                sniprintf(bw_str, 12, "  BW12.5");
                break;
            case BW_25:
                sniprintf(bw_str, 12, "  BW   25");
                break;
        }

        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, bw_str);
    }
#ifdef CONFIG_M17
    else if (last_state.channel.mode == OPMODE_M17)
    {
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, "       ");

    }
#endif

#if defined(CONFIG_UI_NO_KEYBOARD)
        if (ui_state->macro_menu_selected == 4)
#endif // CONFIG_UI_NO_KEYBOARD
    gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER,
              yellow_fab413, "5");

    char mode_str[12] = "";
    switch(last_state.channel.mode)
    {
        case OPMODE_FM:
            gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER, color_white, "         FM");
            break;
        case OPMODE_DMR:
            gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER, color_white, "          DMR");
            break;
#ifdef CONFIG_M17
        case OPMODE_M17:
            gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER, color_white, "          M17");
            break;
#endif
    }

    gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, mode_str);

#if defined(CONFIG_UI_NO_KEYBOARD)
        if (ui_state->macro_menu_selected == 5)
#endif // CONFIG_UI_NO_KEYBOARD
    gfx_print(pos_2, layout.top_font, TEXT_ALIGN_RIGHT,
              yellow_fab413, "6        ");

    // Compute x.y format for TX power avoiding to pull in floating point math.
    // Remember that power is expressed in mW!
    unsigned int power_int = (last_state.channel.power / 1000);
    unsigned int power_dec = (last_state.channel.power % 1000) / 100;
    gfx_print(pos_2, layout.top_font, TEXT_ALIGN_RIGHT,
              color_white, "%d.%dW", power_int, power_dec);

    // Third row
#if defined(CONFIG_UI_NO_KEYBOARD)
        if (ui_state->macro_menu_selected == 6)
#endif // CONFIG_UI_NO_KEYBOARD
    gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_LEFT,
              yellow_fab413, "7");
#ifdef CONFIG_SCREEN_BRIGHTNESS
    gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_LEFT,
              color_white, "  B-");
    gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_LEFT,
              color_white, "       %5d",
              state.settings.brightness);
#endif

#if defined(CONFIG_UI_NO_KEYBOARD)
        if (ui_state->macro_menu_selected == 7)
#endif // CONFIG_UI_NO_KEYBOARD
    gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_CENTER,
              yellow_fab413, "8");
#ifdef CONFIG_SCREEN_BRIGHTNESS
    gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "        B+");
#endif

#if defined(CONFIG_UI_NO_KEYBOARD)
        if (ui_state->macro_menu_selected == 8)
#endif // CONFIG_UI_NO_KEYBOARD
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
