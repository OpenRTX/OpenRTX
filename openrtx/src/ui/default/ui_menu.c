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
#include "ui_menu.h"
#include "ui/ui_menu_list.h"
#include "interfaces/nvmem.h"
#include "interfaces/cps_io.h"
#include "interfaces/platform.h"
#include "core/memory_profiling.h"
#include "core/voicePromptUtils.h"
#include "ui/ui_strings.h"
#include "ui_draw_private.h"

#ifdef PLATFORM_TTWRPLUS
#include "drivers/baseband/SA8x8.h"
#endif

const char *display_timer_values[] = { "OFF",    "5 s",    "10 s",   "15 s",
                                       "20 s",   "25 s",   "30 s",   "1 min",
                                       "2 min",  "3 min",  "4 min",  "5 min",
                                       "15 min", "30 min", "45 min", "1 hour" };

int _ui_getMenuTopEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= menu_num)
        return -1;
    sniprintf(buf, max_len, "%s", menu_items[index]);
    return 0;
}

int _ui_getSettingsEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= settings_num)
        return -1;
    sniprintf(buf, max_len, "%s", settings_items[index]);
    return 0;
}

int _ui_getDisplayEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= display_num)
        return -1;
    sniprintf(buf, max_len, "%s", display_items[index]);
    return 0;
}

int _ui_getDisplayValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= display_num)
        return -1;
    uint8_t value = 0;
    switch (index) {
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
    if (index >= settings_gps_num)
        return -1;
    sniprintf(buf, max_len, "%s", settings_gps_items[index]);
    return 0;
}

int _ui_getSettingsGPSValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= settings_gps_num)
        return -1;
    switch (index) {
        case G_ENABLED:
            sniprintf(buf, max_len, "%s",
                      (last_state.settings.gps_enabled) ? currentLanguage->on :
                                                          currentLanguage->off);
            break;
#ifdef CONFIG_RTC
        case G_SET_TIME:
            sniprintf(buf, max_len, "%s",
                      (last_state.settings.gpsSetTime) ? currentLanguage->on :
                                                         currentLanguage->off);
            break;
        case G_TIMEZONE: {
            int8_t tz_hr = (last_state.settings.utc_timezone / 2);
            int8_t tz_mn = (last_state.settings.utc_timezone % 2) * 5;
            char sign = ' ';

            if (last_state.settings.utc_timezone > 0) {
                sign = '+';
            } else if (last_state.settings.utc_timezone < 0) {
                sign = '-';
                tz_hr *= (-1);
                tz_mn *= (-1);
            }

            sniprintf(buf, max_len, "%c%d.%d", sign, tz_hr, tz_mn);
        } break;
#endif
    }
    return 0;
}
#endif

int _ui_getRadioEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= settings_radio_num)
        return -1;
    sniprintf(buf, max_len, "%s", settings_radio_items[index]);
    return 0;
}

int _ui_getRadioValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= settings_radio_num)
        return -1;

    // Only returning the sign
    if (index == R_DIRECTION) {
        buf[0] = (last_state.channel.tx_frequency
                  >= last_state.channel.rx_frequency) ?
                     '+' :
                     '-';
        buf[1] = '\0';

        return 0;
    }

    // Return an x.y string
    uint32_t value = 0;
    switch (index) {
        case R_OFFSET: {
            uint32_t txFreq = last_state.channel.tx_frequency;
            uint32_t rxFreq = last_state.channel.rx_frequency;

            // Yes, we're basically reinventing the abs() here. The problem is
            // that abs() works on signed integers and using it would mean
            // casting values back and forth between signed and unsigned.
            if (txFreq > rxFreq)
                value = txFreq - rxFreq;
            else
                value = rxFreq - txFreq;
        } break;

        case R_STEP:
            value = freq_steps[last_state.step_index];
            break;
    }

    uint32_t div = 1;
    char prefix = ' ';

    if (value >= 1000000) {
        prefix = 'M';
        div = 1000000;
    } else if (value >= 1000) {
        prefix = 'k';
        div = 1000;
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
    if (index >= settings_m17_num)
        return -1;
    sniprintf(buf, max_len, "%s", settings_m17_items[index]);
    return 0;
}

int _ui_getM17ValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= settings_m17_num)
        return -1;

    switch (index) {
        case M17_CALLSIGN:
            sniprintf(buf, max_len, "%s", last_state.settings.callsign);
            break;

        case M17_METATEXT:
            // limit display to 8 characters
            if (strlen(last_state.settings.M17_meta_text) > 7) {
                char tmp[9];
                memcpy(tmp, last_state.settings.M17_meta_text, 7);
                tmp[8] = 0;
                // append asterisk to indicate more characters than displayed
                sniprintf(buf, max_len, "%s*", tmp);
            } else
                sniprintf(buf, max_len, "%s",
                          last_state.settings.M17_meta_text);
            break;

        case M17_CAN:
            sniprintf(buf, max_len, "%d", last_state.settings.m17_can);
            break;
        case M17_CAN_RX:
            sniprintf(buf, max_len, "%s",
                      (last_state.settings.m17_can_rx) ? currentLanguage->on :
                                                         currentLanguage->off);
            break;
    }

    return 0;
}
#endif

int _ui_getFMEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= settings_fm_num)
        return -1;

    sniprintf(buf, max_len, "%s", settings_fm_items[index]);
    return 0;
}

int _ui_getFMValueName(char *buf, uint8_t max_len, uint8_t index)
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
    if (index >= settings_accessibility_num)
        return -1;
    sniprintf(buf, max_len, "%s", settings_accessibility_items[index]);
    return 0;
}

int _ui_getAccessibilityValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= settings_accessibility_num)
        return -1;
    uint8_t value = 0;
    switch (index) {
        case A_LEVEL: {
            value = last_state.settings.vpLevel;
            switch (value) {
                case vpNone:
                    sniprintf(buf, max_len, "%s", currentLanguage->off);
                    break;
                case vpBeep:
                    sniprintf(buf, max_len, "%s", currentLanguage->beep);
                    break;
                default:
                    sniprintf(buf, max_len, "%d", (value - vpBeep));
                    break;
            }
            break;
        }
        case A_PHONETIC:
            sniprintf(buf, max_len, "%s",
                      last_state.settings.vpPhoneticSpell ?
                          currentLanguage->on :
                          currentLanguage->off);
            break;
        case A_MACRO_LATCH:
            sniprintf(buf, max_len, "%s",
                      last_state.settings.macroMenuLatch ?
                          currentLanguage->on :
                          currentLanguage->off);
            break;
    }
    return 0;
}

int _ui_getBackupRestoreEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= backup_restore_num)
        return -1;
    sniprintf(buf, max_len, "%s", backup_restore_items[index]);
    return 0;
}

int _ui_getInfoEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if (index >= info_num)
        return -1;
    sniprintf(buf, max_len, "%s", info_items[index]);
    return 0;
}

int _ui_getInfoValueName(char *buf, uint8_t max_len, uint8_t index)
{
    const hwInfo_t *hwinfo = platform_getHwInfo();
    if (index >= info_num)
        return -1;
    switch (index) {
        case 0: // Git Version
            sniprintf(buf, max_len, "%s", GIT_VERSION);
            break;
        case 1: // Battery voltage
        {
            // Compute integer part and mantissa of voltage value, adding 50mV
            // to mantissa for rounding to nearest integer
            uint16_t volt = (last_state.v_bat + 50) / 1000;
            uint16_t mvolt = ((last_state.v_bat - volt * 1000) + 50) / 100;
            sniprintf(buf, max_len, "%d.%dV", volt, mvolt);
        } break;
        case 2: // Battery charge
            sniprintf(buf, max_len, "%d%%", last_state.charge);
            break;
        case 3: // RSSI
            sniprintf(buf, max_len, "%" PRIi32 "dBm", last_state.rssi);
            break;
        case 4: // Heap usage
            sniprintf(buf, max_len, "%dB",
                      getHeapSize() - getCurrentFreeHeap());
            break;
        case 5: // Band
            sniprintf(buf, max_len, "%s %s",
                      hwinfo->vhf_band ? currentLanguage->VHF : "",
                      hwinfo->uhf_band ? currentLanguage->UHF : "");
            break;
        case 6: // VHF
            sniprintf(buf, max_len, "%d - %d", hwinfo->vhf_minFreq,
                      hwinfo->vhf_maxFreq);
            break;
        case 7: // UHF
            sniprintf(buf, max_len, "%d - %d", hwinfo->uhf_minFreq,
                      hwinfo->uhf_maxFreq);
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

            sscanf(fwVer, "sa8x8-fw/v%hhu.%hhu.%hhu.r%hhu", &major, &minor,
                   &patch, &release);
            sniprintf(buf, max_len, "v%hhu.%hhu.%hhu.r%hhu", major, minor,
                      patch, release);
        } break;
#endif
    }
    return 0;
}

int _ui_getBankName(char *buf, uint8_t max_len, uint8_t index)
{
    int result = 0;
    // First bank "All channels" is not read from flash
    if (index == 0) {
        strncpy(buf, currentLanguage->allChannels, max_len);
    } else {
        bankHdr_t bank;
        result = cps_readBankHeader(&bank, index - 1);
        if (result != -1)
            sniprintf(buf, max_len, "%s", bank.name);
    }
    return result;
}

int _ui_getChannelName(char *buf, uint8_t max_len, uint8_t index)
{
    channel_t channel;
    int result = cps_readChannel(&channel, index);
    if (result != -1)
        sniprintf(buf, max_len, "%s", channel.name);
    return result;
}

int _ui_getContactName(char *buf, uint8_t max_len, uint8_t index)
{
    contact_t contact;
    int result = cps_readContact(&contact, index);
    if (result != -1)
        sniprintf(buf, max_len, "%s", contact.name);
    return result;
}

void _ui_drawMenuTop(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "Menu" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->menu);
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getMenuTopEntryName);
}

void _ui_drawMenuBank(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "Bank" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->banks);
    // Print bank entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getBankName);
}

void _ui_drawMenuChannel(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "Channel" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->channels);
    // Print channel entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getChannelName);
}

void _ui_drawMenuContacts(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "Contacts" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->contacts);
    // Print contact entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getContactName);
}

#ifdef CONFIG_GPS
void _ui_drawMenuGPS()
{
    char *fix_buf, *type_buf;
    gfx_clearScreen();
    // Print "GPS" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->gps);
    point_t fix_pos = { layout.line2_pos.x, CONFIG_SCREEN_HEIGHT * 2 / 5 };
    // Print GPS status, if no fix, hide details
    if (!last_state.gpsDetected)
        gfx_print(fix_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->noGps);
    else if (!last_state.settings.gps_enabled)
        gfx_print(fix_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->gpsOff);
    else if (last_state.gps_data.fix_quality == FIX_QUALITY_NO_FIX)
        gfx_print(fix_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->noFix);
    else if (last_state.gps_data.fix_quality == FIX_QUALITY_ESTIMATED)
        gfx_print(fix_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->fixLost);
    else {
        switch (last_state.gps_data.fix_quality) {
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
                fix_buf = (char *)currentLanguage->error;
                break;
        }

        switch (last_state.gps_data.fix_type) {
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
                type_buf = (char *)currentLanguage->error;
                break;
        }
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, fix_buf);

        // Convert from signed longitude, to unsigned + direction
        int32_t latitude = abs(last_state.gps_data.latitude);
        uint8_t latitude_int = latitude / 1000000;
        int32_t latitude_dec = latitude % 1000000;
        char *direction_lat = (last_state.gps_data.latitude < 0) ? "S     " :
                                                                   "N     ";

        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, direction_lat);
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "%d.%.6d", latitude_int, latitude_dec);
        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, type_buf);

        // Convert from signed longitude, to unsigned + direction
        int32_t longitude = abs(last_state.gps_data.longitude);
        uint8_t longitude_int = longitude / 1000000;
        int32_t longitude_dec = longitude % 1000000;
        char *direction_lon = (last_state.gps_data.longitude < 0) ? "W     " :
                                                                    "E     ";

        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, direction_lon);
        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "%d.%.6d", longitude_int, longitude_dec);
        gfx_print(layout.bottom_pos, layout.bottom_font, TEXT_ALIGN_CENTER,
                  color_white, "S %dkm/h  A %dm", last_state.gps_data.speed,
                  last_state.gps_data.altitude);
    }
    // Draw compass
    point_t compass_pos = { layout.horizontal_pad * 2,
                            CONFIG_SCREEN_HEIGHT / 2 };
    gfx_drawGPScompass(compass_pos, CONFIG_SCREEN_WIDTH / 9 + 2,
                       last_state.gps_data.tmg_true,
                       last_state.gps_data.fix_quality != 0
                           && last_state.gps_data.fix_quality != 6);
    // Draw satellites bar graph
    point_t bar_pos = { layout.line3_large_pos.x + CONFIG_SCREEN_WIDTH * 1 / 3,
                        CONFIG_SCREEN_HEIGHT / 2 };
    gfx_drawGPSgraph(bar_pos,
                     (CONFIG_SCREEN_WIDTH * 2 / 3) - layout.horizontal_pad,
                     CONFIG_SCREEN_HEIGHT / 3, last_state.gps_data.satellites,
                     last_state.gps_data.active_sats);
}
#endif

void _ui_drawMenuSettings(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->settings);
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getSettingsEntryName);
}

void _ui_drawMenuBackupRestore(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "Backup & Restore" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->backupAndRestore);
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getBackupRestoreEntryName);
}

void _ui_drawMenuBackup(ui_state_t *ui_state)
{
    (void)ui_state;

    gfx_clearScreen();
    // Print "Flash Backup" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->flashBackup);
    // Print backup message
    point_t line = layout.line2_pos;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->connectToRTXTool);
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->toBackupFlashAnd);
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->pressPTTToStart);

    if (!platform_getPttStatus())
        return;

    state.devStatus = DATATRANSFER;
    state.backup_eflash = true;
}

void _ui_drawMenuRestore(ui_state_t *ui_state)
{
    (void)ui_state;

    gfx_clearScreen();
    // Print "Flash Restore" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->flashRestore);
    // Print backup message
    point_t line = layout.line2_pos;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->connectToRTXTool);
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->toRestoreFlashAnd);
    line.y += 18;
    gfx_print(line, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->pressPTTToStart);

    if (!platform_getPttStatus())
        return;

    state.devStatus = DATATRANSFER;
    state.restore_eflash = true;
}

void _ui_drawMenuInfo(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "Info" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->info);
    // Print menu entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected,
                          _ui_getInfoEntryName, _ui_getInfoValueName);
}

void _ui_drawMenuAbout(ui_state_t *ui_state)
{
    gfx_clearScreen();

    point_t logo_pos;
    if (CONFIG_SCREEN_HEIGHT >= 100) {
        logo_pos.x = 0;
        logo_pos.y = CONFIG_SCREEN_HEIGHT / 5;
        gfx_print(logo_pos, FONT_SIZE_12PT, TEXT_ALIGN_CENTER, yellow_fab413,
                  "O P N\nR T X");
    } else {
        logo_pos.x = layout.horizontal_pad;
        logo_pos.y = layout.line3_large_h;
        gfx_print(logo_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, currentLanguage->openRTX);
    }

    point_t pos = { CONFIG_SCREEN_WIDTH / 7,
                    CONFIG_SCREEN_HEIGHT - (layout.menu_h * 3) - 5 };
    uint8_t entries_in_screen =
        (CONFIG_SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t max_scroll = author_num - entries_in_screen;

    if (ui_state->menu_selected >= max_scroll)
        ui_state->menu_selected = max_scroll;

    for (uint8_t item = 0; item < entries_in_screen; item++) {
        uint8_t elem = ui_state->menu_selected + item;
        gfx_print(pos, layout.menu_font, TEXT_ALIGN_LEFT, color_white,
                  authors[elem]);
        pos.y += layout.menu_h;
    }
}
