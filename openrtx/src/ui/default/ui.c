/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
#include "ui/ui_default.h"
#include "ui_vfo.h"
#include "ui_input.h"
#include "ui_fsm.h"
#include "ui_layout_config.h"
#include "ui_macro_menu.h"
#include "ui_menu.h"
#include "ui/ui_menu_list.h"
#include "ui_settings.h"
#include "rtx/rtx.h"
#include "interfaces/platform.h"
#include "interfaces/display.h"
#include "interfaces/cps_io.h"
#include "interfaces/nvmem.h"
#include "interfaces/delays.h"
#include <string.h>
#include "core/battery.h"
#include "core/input.h"
#include "core/utils.h"
#include "hwconfig.h"
#include "core/voicePromptUtils.h"
#include "core/beeps.h"

/* UI main screen functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainBackground();
extern void _ui_drawMainTop(ui_state_t *ui_state);
extern void _ui_drawMEMMiddle();
extern void _ui_drawMEMBottom();
extern void _ui_drawMainMEM(ui_state_t *ui_state);
// TODO: get these from ui strings / currentLanguage
const char *menu_items[] = { "Banks",    "Channels", "Contacts",
#ifdef CONFIG_GPS
                             "GPS",
#endif
                             "Settings", "Info",     "About" };

const char *settings_items[] = { "Display",
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
                                 "FM",
                                 "Accessibility",
                                 "Default Settings" };

const char *display_items[] = {
#ifdef CONFIG_SCREEN_BRIGHTNESS
    "Brightness",
#endif
#ifdef CONFIG_SCREEN_CONTRAST
    "Contrast",
#endif
    "Timer", "Battery Icon"
};

#ifdef CONFIG_GPS
const char *settings_gps_items[] = { "GPS Enabled",
#ifdef CONFIG_RTC
                                     "GPS Set Time", "UTC Timezone"
#endif
};
#endif

const char *settings_radio_items[] = {
    "Offset",
    "Direction",
    "Step",
};

const char *settings_m17_items[] = { "Callsign", "Meta Txt", "CAN",
                                     "CAN RX Check" };

const char *settings_fm_items[] = { "CTCSS Tone", "CTCSS En." };

const char *settings_accessibility_items[] = { "Macro Latch", "Voice",
                                               "Phonetic" };

const char *backup_restore_items[] = { "Backup", "Restore" };

const char *info_items[] = {
    "",      "Bat. Voltage", "Bat. Charge", "RSSI",       "Used heap",
    "Band",  "VHF",          "UHF",         "Hw Version",
#ifdef PLATFORM_TTWRPLUS
    "Radio", "Radio FW",
#endif
};

const char *authors[] = { "Niccolo' IU2KIN", "Silvano IU2KWO",
                          "Federico IU2NUO", "Fred IU2NRO",
                          "Joseph VK7JS",    "Morgan ON4MOD",
                          "Marco DM4RCO" };

// Calculate number of menu entries
const uint8_t menu_num = sizeof(menu_items) / sizeof(menu_items[0]);
const uint8_t settings_num = sizeof(settings_items) / sizeof(settings_items[0]);
const uint8_t display_num = sizeof(display_items) / sizeof(display_items[0]);
#ifdef CONFIG_GPS
const uint8_t settings_gps_num = sizeof(settings_gps_items)
                               / sizeof(settings_gps_items[0]);
#endif
const uint8_t settings_radio_num = sizeof(settings_radio_items)
                                 / sizeof(settings_radio_items[0]);
#ifdef CONFIG_M17
const uint8_t settings_m17_num = sizeof(settings_m17_items)
                               / sizeof(settings_m17_items[0]);
#endif
const uint8_t settings_fm_num = sizeof(settings_fm_items)
                              / sizeof(settings_fm_items[0]);
const uint8_t settings_accessibility_num =
    sizeof(settings_accessibility_items)
    / sizeof(settings_accessibility_items[0]);
const uint8_t backup_restore_num = sizeof(backup_restore_items)
                                 / sizeof(backup_restore_items[0]);
const uint8_t info_num = sizeof(info_items) / sizeof(info_items[0]);
const uint8_t author_num = sizeof(authors) / sizeof(authors[0]);

const color_t color_black = { 0, 0, 0, 255 };
const color_t color_grey = { 60, 60, 60, 255 };
const color_t color_white = { 255, 255, 255, 255 };
const color_t yellow_fab413 = { 250, 180, 19, 255 };

state_t last_state;
static bool layout_ready = false;

static void _ui_drawLowBatteryScreen()
{
    gfx_clearScreen();
    uint16_t bat_width = CONFIG_SCREEN_WIDTH / 2;
    uint16_t bat_height = CONFIG_SCREEN_HEIGHT / 3;
    point_t bat_pos = { CONFIG_SCREEN_WIDTH / 4, CONFIG_SCREEN_HEIGHT / 8 };
    gfx_drawBattery(bat_pos, bat_width, bat_height, 10);
    point_t text_pos_1 = { 0, CONFIG_SCREEN_HEIGHT * 2 / 3 };
    point_t text_pos_2 = { 0, CONFIG_SCREEN_HEIGHT * 2 / 3 + 16 };

    gfx_print(text_pos_1, FONT_SIZE_6PT, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->forEmergencyUse);
    gfx_print(text_pos_2, FONT_SIZE_6PT, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->pressAnyButton);
}

static void _ui_drawDarkOverlay(void)
{
    color_t alpha_grey = { 0, 0, 0, 255 };
    point_t origin = { 0, 0 };

    gfx_drawRect(origin, CONFIG_SCREEN_WIDTH, CONFIG_SCREEN_HEIGHT, alpha_grey,
                 true);
}

void ui_init()
{
    ui_fsm_init();
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
    static const point_t logo_orig = { 0, (CONFIG_SCREEN_HEIGHT / 2) - 6 };
    static const point_t call_orig = { 0, CONFIG_SCREEN_HEIGHT - 8 };
    static const fontSize_t logo_font = FONT_SIZE_12PT;
    static const fontSize_t call_font = FONT_SIZE_8PT;
#else
    static const point_t logo_orig = { 0, 19 };
    static const point_t call_orig = { 0, CONFIG_SCREEN_HEIGHT - 8 };
    static const fontSize_t logo_font = FONT_SIZE_8PT;
    static const fontSize_t call_font = FONT_SIZE_6PT;
#endif

    gfx_print(logo_orig, logo_font, TEXT_ALIGN_CENTER, yellow_fab413,
              "O P N\nR T X");
    gfx_print(call_orig, call_font, TEXT_ALIGN_CENTER, color_white,
              state.settings.callsign);

    vp_announceSplashScreen();
}

void ui_saveState()
{
    last_state = state;
}

bool ui_updateGUI()
{
    if (!ui_fsm_needRedraw())
        return false;

    if (!layout_ready) {
        _ui_calculateLayout(&layout);
        layout_ready = true;
    }
    // Draw current GUI page
    switch (last_state.ui_screen) {
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
        // FM settings screen
        case SETTINGS_FM:
            _ui_drawSettingsFM(&ui_state);
            break;
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
    if (ui_fsm_is_macro_menu_visible()) {
        _ui_drawDarkOverlay();
        _ui_drawMacroMenu(&ui_state);
    }

    ui_fsm_clearRedraw();
    return true;
}

void ui_terminate()
{
}
