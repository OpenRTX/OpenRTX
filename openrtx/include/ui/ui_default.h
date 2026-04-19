/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UI_DEFAULT_H
#define UI_DEFAULT_H

#include <stdbool.h>
#include "core/state.h"
#include "core/graphics.h"
#include "interfaces/keyboard.h"
#include <stdint.h>
#include "core/event.h"
#include "hwconfig.h"
#include "core/ui.h"
#include "ui/ui_limits.h"
#include "ui/ui_layout.h"

enum uiScreen {
    MAIN_VFO = 0,
    MAIN_VFO_INPUT,
    MAIN_MEM,
    MODE_VFO,
    MODE_MEM,
    MENU_TOP,
    MENU_BANK,
    MENU_CHANNEL,
    MENU_CONTACTS,
    MENU_GPS,
    MENU_SETTINGS,
    MENU_BACKUP_RESTORE,
    MENU_BACKUP,
    MENU_RESTORE,
    MENU_INFO,
    MENU_ABOUT,
    SETTINGS_TIMEDATE,
    SETTINGS_TIMEDATE_SET,
    SETTINGS_DISPLAY,
    SETTINGS_GPS,
    SETTINGS_RADIO,
    SETTINGS_M17,
    SETTINGS_FM,
    SETTINGS_ACCESSIBILITY,
    SETTINGS_RESET2DEFAULTS,
    LOW_BAT
};

enum SetRxTx { SET_RX = 0, SET_TX };

// This enum is needed to have item numbers that match
// menu elements even if some elements may be missing (GPS)
enum menuItems {
    M_BANK = 0,
    M_CHANNEL,
    M_CONTACTS,
#ifdef CONFIG_GPS
    M_GPS,
#endif
    M_SETTINGS,
    M_INFO,
    M_ABOUT
};

enum settingsItems {
    S_DISPLAY = 0,
#ifdef CONFIG_RTC
    S_TIMEDATE,
#endif
#ifdef CONFIG_GPS
    S_GPS,
#endif
    S_RADIO,
#ifdef CONFIG_M17
    S_M17,
#endif
    S_FM,
    S_ACCESSIBILITY,
    S_RESET2DEFAULTS,
};

enum backupRestoreItems { BR_BACKUP = 0, BR_RESTORE };

enum displayItems {
#ifdef CONFIG_SCREEN_BRIGHTNESS
    D_BRIGHTNESS = 0,
#endif
#ifdef CONFIG_SCREEN_CONTRAST
    D_CONTRAST,
#endif
    D_TIMER,
    D_BATTERY
};

#ifdef CONFIG_GPS
enum settingsGPSItems {
    G_ENABLED = 0,
#ifdef CONFIG_RTC
    G_SET_TIME,
    G_TIMEZONE
#endif
};
#endif

enum settingsAccessibilityItems {
    A_MACRO_LATCH = 0,
    A_LEVEL,
    A_PHONETIC,
};

enum settingsRadioItems {
    R_OFFSET,
    R_DIRECTION,
    R_STEP,
};

enum settingsM17Items { M17_CALLSIGN = 0, M17_METATEXT, M17_CAN, M17_CAN_RX };

enum settingsFMItems { CTCSS_Tone, CTCSS_Enabled };

/**
 * This structs contains state variables internal to the
 * UI that need to be kept between executions of the UI
 * This state does not need to be saved on device poweroff
 */
typedef struct ui_state_t {
    // Index of the currently selected menu entry
    uint8_t menu_selected;
    // If true we can change a menu entry value with UP/DOWN
    bool edit_mode;
    bool input_locked;
    // Variables used for VFO input
    uint8_t input_number;
    uint8_t input_position;
    uint8_t input_set;
    long long last_keypress;
    freq_t new_rx_frequency;
    freq_t new_tx_frequency;
    char new_rx_freq_buf[14];
    char new_tx_freq_buf[14];
    size_t m17_meta_text_scroll_position;
    long long m17_meta_text_last_scroll_tick;
    char new_message[53];
    bool edit_message;
#ifdef CONFIG_RTC
    // Variables used for Time & Date input
    datetime_t new_timedate;
    char new_date_buf[9];
    char new_time_buf[9];
#endif
    char new_callsign[10];
    freq_t new_offset;
    // Which state to return to when we exit menu
    uint8_t last_main_state;
#if defined(CONFIG_UI_NO_KEYBOARD)
    uint8_t macro_menu_selected;
#endif // UI_NO_KEYBOARD
} ui_state_t;

extern ui_state_t ui_state;

extern layout_t layout;
extern state_t last_state;
extern bool macro_latched;
extern const char *menu_items[];
extern const char *settings_items[];
extern const char *display_items[];
extern const char *settings_gps_items[];
extern const char *settings_radio_items[];
extern const char *settings_m17_items[];
extern const char *settings_fm_items[];
extern const char *settings_accessibility_items[];
extern const char *backup_restore_items[];
extern const char *info_items[];
extern const char *authors[];
extern const uint8_t menu_num;
extern const uint8_t settings_num;
extern const uint8_t display_num;
extern const uint8_t settings_gps_num;
extern const uint8_t settings_radio_num;
extern const uint8_t settings_m17_num;
extern const uint8_t settings_fm_num;
extern const uint8_t settings_accessibility_num;
extern const uint8_t backup_restore_num;
extern const uint8_t info_num;
extern const uint8_t author_num;
extern const color_t color_black;
extern const color_t color_grey;
extern const color_t color_white;
extern const color_t yellow_fab413;

#endif /* UI_DEFAULT_H */
