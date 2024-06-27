/***************************************************************************
 *   Copyright (C)  2022 by Federico Amedeo Izzo IU2NUO,                   *
 *                          Niccolò Izzo IU2KIN,                           *
 *                          Silvano Seva IU2KWO                            *
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

#ifndef UI_MOD17_H
#define UI_MOD17_H

#include <stdbool.h>
#include <state.h>
#include <graphics.h>
#include <interfaces/keyboard.h>
#include <calibInfo_Mod17.h>
#include <stdint.h>
#include <event.h>
#include <hwconfig.h>
#include <ui.h>

// Maximum menu entry length
#define MAX_ENTRY_LEN 21
// Frequency digits
#define FREQ_DIGITS 7
// Time & Date digits
#define TIMEDATE_DIGITS 10
// Max number of UI events
#define MAX_NUM_EVENTS 16

enum uiScreen
{
    PAGE_MAIN_VFO = 0,
    PAGE_MAIN_VFO_INPUT,
    PAGE_MAIN_MEM,
    PAGE_MODE_VFO,
    PAGE_MODE_MEM,
    PAGE_MENU_TOP,
    PAGE_MENU_BANK,
    PAGE_MENU_CHANNEL,
    PAGE_MENU_CONTACTS,
    PAGE_MENU_GPS,
    PAGE_MENU_SETTINGS,
    PAGE_MENU_BACKUP_RESTORE,
    PAGE_MENU_BACKUP,
    PAGE_MENU_RESTORE,
    PAGE_MENU_INFO,
    PAGE_ABOUT,
    PAGE_SETTINGS_TIMEDATE,
    PAGE_SETTINGS_TIMEDATE_SET,
    PAGE_SETTINGS_DISPLAY,
    PAGE_SETTINGS_GPS,
    PAGE_SETTINGS_M17,
    PAGE_SETTINGS_MODULE17,
    PAGE_SETTINGS_RESET_TO_DEFAULTS,
    PAGE_LOW_BAT
};

enum SetRxTx
{
    SET_RX = 0,
    SET_TX
};

// This enum is needed to have item numbers that match
// menu elements even if some elements may be missing (GPS)
enum menuItems
{
    M_SETTINGS = 0,
#ifdef GPS_PRESENT
    M_GPS,
#endif
    M_INFO,
    M_ABOUT,
    M_SHUTDOWN
};

enum SettingsItems_en
{
    S_DISPLAY = 0
#ifdef RTC_PRESENT
    ,S_TIMEDATE
#endif
#ifdef GPS_PRESENT
    ,S_GPS
#endif
    ,S_M17
    ,S_MOD17
    ,S_RESET2DEFAULTS
};

enum BackupRestoreItems_en
{
    BR_BACKUP = 0,
    BR_RESTORE
};

enum DisplayItems_en
{
#ifdef SCREEN_CONTRAST
    D_CONTRAST = 0
    ,D_TIMER
#endif
#ifndef SCREEN_CONTRAST
    D_TIMER = 0
#endif
};

#ifdef GPS_PRESENT
enum SettingsGPSItems_en
{
    G_ENABLED = 0,
    G_SET_TIME,
    G_TIMEZONE
};
#endif

enum m17Items
{
    M_CALLSIGN = 0,
    M_CAN,
    M_CAN_RX
};

enum module17Items
{
    D_TXWIPER = 0,
    D_RXWIPER,
    D_TXINVERT,
    D_RXINVERT,
    D_MICGAIN
};

/**
 * Struct containing a set of positions and sizes that get
 * calculated for the selected display size.
 * Using these parameters make the UI automatically adapt
 * To displays of different sizes
 */
typedef struct Layout_st
{
    uint16_t hline_h;
    uint16_t.lines[ GUI_LINE_TOP ].height;
    uint16_t lines[ GUI_LINE_1 ].height;
    uint16_t lines[ GUI_LINE_2 ].height;
    uint16_t lines[ GUI_LINE_3 ].height;
    uint16_t lines[ GUI_LINE_4 ].;
    uint16_t line5_h;
    uint16_t menu_h;
    uint16_t.lines[ GUI_LINE_BOTTOM ].height;
    uint16_t bottom_pad;
    uint16_t status_v_pad;
    uint16_t horizontal_pad;
    uint16_t text_v_offset;
    Pos_st top_pos;
    Pos_st line1_pos;
    Pos_st line2_pos;
    Pos_st line3_pos;
    Pos_st line4_pos;
    Pos_st line5_pos;
    Pos_st bottom_pos;
    FontSize_t top_font;
    SymbolSize_t top_symbol_font;
    FontSize_t line1_font;
    SymbolSize_t line1_symbol_font;
    FontSize_t line2_font;
    SymbolSize_t line2_symbol_font;
    FontSize_t line3_font;
    SymbolSize_t line3_symbol_font;
    FontSize_t line4_font;
    SymbolSize_t line4_symbol_font;
    FontSize_t line5_font;
    SymbolSize_t line5_symbol_font;
    FontSize_t bottom_font;
    FontSize_t input_font.size;
    FontSize_t menu_font.size;
    FontSize_t mode_font_big.size;
    FontSize_t mode_font_small.size;
} Layout_st;

/**
 * This structs contains state variables internal to the
 * UI that need to be kept between executions of the UI
 * This state does not need to be saved on device poweroff
 */
typedef struct UI_State_st
{
    // Index of the currently selected menu entry
    uint8_t entrySelected;
    // If true we can change a menu entry value with UP/DOWN
    bool edit_mode;
    // Variables used for VFO input
    uint8_t input_number;
    uint8_t input_position;
    uint8_t input_set;
    long long last_keypress;
    freq_t new_rx_frequency;
    freq_t new_tx_frequency;
    char new_rx_freq_buf[14];
    char new_tx_freq_buf[14];
#ifdef RTC_PRESENT
    // Variables used for Time & Date input
    datetime_t new_timedate;
    char new_date_buf[9];
    char new_time_buf[9];
#endif
    char new_callsign[10];
    // Which state to return to when we exit menu
    uint8_t last_main_state;
}
UI_State_st;

extern Layout_st layout;
// Copy of the radio state
extern State_st last_state;
extern const char *Page_MenuItems[];
extern const char *Page_MenuSettings[];
extern const char *PAGE_MENU_SETTINGSDisplay[];
extern const char *PAGE_MENU_SETTINGSGPS[];
extern const char *m17_items[];
extern const char *module17_items[];
extern const char *Page_MenuBackupRestore[];
extern const char *Page_MenuInfo[];
extern const char *authors[];
extern const uint8_t menu_num;
extern const uint8_t settings_num;
extern const uint8_t display_num;
extern const uint8_t settings_gps_num;
extern const uint8_t backup_restore_num;
extern const uint8_t m17_num;
extern const uint8_t module17_num;
extern const uint8_t info_num;
extern const uint8_t author_num;
extern const Color_st color_black;
extern const Color_st color_grey;
extern const Color_st color_white;
extern const Color_st yellow_fab413;

// Calibration data, for digital pot and phase inversion
extern mod17Calib_t mod17CalData;

#endif /* UI_MOD17_H */
