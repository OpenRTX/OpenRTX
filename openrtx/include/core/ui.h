/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
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

#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <state.h>
#include <interfaces/graphics.h>
#include <interfaces/keyboard.h>
#include <stdint.h>
#include <event.h>
#include <hwconfig.h>

// Maximum menu entry length
#define MAX_ENTRY_LEN 21
// Frequency digits
#define FREQ_DIGITS 8
// Time & Date digits
#define TIMEDATE_DIGITS 10

enum uiScreen
{
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
    SETTINGS_M17,
    SETTINGS_RESET2DEFAULTS,
    LOW_BAT
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
    M_BANK = 0,
    M_CHANNEL,
    M_CONTACTS,
#ifdef HAS_GPS
    M_GPS,
#endif
    M_SETTINGS,
    M_BACKUP_RESTORE,
    M_INFO,
    M_ABOUT
};

enum settingsItems
{
    S_DISPLAY = 0
#ifdef HAS_RTC
    ,S_TIMEDATE
#endif
#ifdef HAS_GPS
    ,S_GPS
#endif
    ,S_M17
    ,S_RESET2DEFAULTS
};

enum backupRestoreItems
{
    BR_BACKUP = 0,
    BR_RESTORE
};

enum displayItems
{
    D_BRIGHTNESS = 0
#ifdef SCREEN_CONTRAST
    ,D_CONTRAST
#endif
    ,D_TIMER
};

#ifdef HAS_GPS
enum settingsGPSItems
{
    G_ENABLED = 0,
    G_SET_TIME,
    G_TIMEZONE
};
#endif

/**
 * Struct containing a set of positions and sizes that get
 * calculated for the selected display size.
 * Using these parameters make the UI automatically adapt
 * To displays of different sizes
 */
typedef struct layout_t
{
    uint16_t hline_h;
    uint16_t top_h;
    uint16_t line1_h;
    uint16_t line2_h;
    uint16_t line3_h;
    uint16_t menu_h;
    uint16_t bottom_h;
    uint16_t bottom_pad;
    uint16_t status_v_pad;
    uint16_t horizontal_pad;
    uint16_t text_v_offset;
    point_t top_pos;
    point_t line1_pos;
    point_t line2_pos;
    point_t line3_pos;
    point_t bottom_pos;
    fontSize_t top_font;
    fontSize_t line1_font;
    fontSize_t line2_font;
    fontSize_t line3_font;
    fontSize_t bottom_font;
    fontSize_t input_font;
    fontSize_t menu_font;
    fontSize_t mode_font_big;
    fontSize_t mode_font_small;
} layout_t;

/**
 * This structs contains state variables internal to the
 * UI that need to be kept between executions of the UI
 * This state does not need to be saved on device poweroff
 */
typedef struct ui_state_t
{
    // Index of the currently selected menu entry
    uint8_t menu_selected;
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
#ifdef HAS_RTC
    // Variables used for Time & Date input
    datetime_t new_timedate;
    char new_date_buf[9];
    char new_time_buf[9];
#endif
    char new_callsign[10];
    // Which state to return to when we exit menu
    uint8_t last_main_state;
}
ui_state_t;

extern layout_t layout;
// Copy of the radio state
extern state_t last_state;
extern const char *menu_items[];
extern const char *settings_items[];
extern const char *display_items[];
extern const char *settings_gps_items[];
extern const char *backup_restore_items[];
extern const char *info_items[];
extern const char *authors[];
extern const uint8_t menu_num;
extern const uint8_t settings_num;
extern const uint8_t display_num;
extern const uint8_t settings_gps_num;
extern const uint8_t backup_restore_num;
extern const uint8_t info_num;
extern const uint8_t author_num;
extern const color_t color_black;
extern const color_t color_grey;
extern const color_t color_white;
extern const color_t yellow_fab413;

/**
 * This function initialises the User Interface, starting the
 * Finite State Machine describing the user interaction.
 */
void ui_init();

/**
 * This function writes the OpenRTX splash screen image into the framebuffer.
 * @param centered: if true the logo will be printed at the center of
 * the screen, otherwise it will be printed at the top of the screen.
 */
void ui_drawSplashScreen(bool centered);

/**
 * This function updates the local copy of the radio state
 * the local copy is called last_state
 * and is accessible from all the UI code as extern variable.
 */
void ui_saveState();

/**
 * This function advances the User Interface FSM, basing on the
 * current radio state and the keys pressed.
 * @param last_state: A local copy of the previous radio state
 * @param event: An event from other threads
 * @param sync_rtx: If true RTX needs to be synchronized
 */
void ui_updateFSM(event_t event, bool *sync_rtx);

/**
 * This function redraws the GUI based on the last radio state.
 */
void ui_updateGUI();

/**
 * This function terminates the User Interface.
 */
void ui_terminate();

#endif /* UI_H */
