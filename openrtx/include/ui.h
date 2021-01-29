/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
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

#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <state.h>
#include <interfaces/graphics.h>
#include <interfaces/keyboard.h>
#include <stdint.h>
#include <event.h>
#include <hwconfig.h>
#include <settings.h>

// Maximum menu entry length
#define MAX_ENTRY_LEN 16
// Frequency digits
#define FREQ_DIGITS 8
// Time & Date digits
#define TIMEDATE_DIGITS 10

enum uiScreen
{
    MAIN_VFO = 0,
    MAIN_VFO_INPUT,
    MAIN_MEM,
    MENU_TOP,
    MENU_ZONE,
    MENU_CHANNEL,
    MENU_CONTACTS,
    MENU_SMS,
    MENU_GPS,
    MENU_MACRO,
    MENU_SETTINGS,
    SETTINGS_TIMEDATE,
    SETTINGS_TIMEDATE_SET,
    SETTINGS_DISPLAY,
    LOW_BAT
};

enum SetRxTx
{
    SET_RX = 0,
    SET_TX
};

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
    uint16_t bottom_h;
    uint16_t status_v_pad;
    uint16_t line_v_pad;
    uint16_t horizontal_pad;
    uint16_t text_v_offset;
    point_t top_left;
    point_t line1_left;
    point_t line2_left;
    point_t line3_left;
    point_t bottom_left;
    point_t top_right;
    point_t line1_right;
    point_t line2_right;
    point_t line3_right;
    point_t bottom_right;
    fontSize_t top_font;
    fontSize_t line1_font;
    fontSize_t line2_font;
    fontSize_t line3_font;
    fontSize_t bottom_font;
} layout_t;

/** 
 * This structs contains state variables internal to the
 * UI that need to be kept between executions of the UI
 * This state does not need to be saved on device poweroff
 */
typedef struct ui_state_t
{
    uint8_t menu_selected;
    uint8_t input_number;
    uint8_t input_position;
    uint8_t input_set;
    freq_t new_rx_frequency;
    freq_t new_tx_frequency;
    char new_rx_freq_buf[14];
    char new_tx_freq_buf[14];
#ifdef HAS_RTC
    curTime_t new_timedate;
    char new_date_buf[9];
    char new_time_buf[9];
#endif
    uint8_t last_main_state;
} ui_state_t;

extern layout_t layout;
extern settings_t settings;
extern const char *menu_items[6];
extern const char *settings_items[2];
extern const char *display_items[];
extern const uint8_t menu_num;
extern const uint8_t settings_num;
extern const uint8_t display_num;
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
 */
void ui_drawSplashScreen();

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
 * @param last_state: A local copy of the previous radio state
 */
void ui_updateGUI(state_t last_state);

/**
 * This function terminates the User Interface.
 */
void ui_terminate();

#endif /* UI_H */
