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
 *                                                                         *
 *   (2025) Modified by KD0OSS for new modes on Module17                   *
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
    MENU_MODE,
    MENU_BACKUP_RESTORE,
    MENU_BACKUP,
    MENU_RESTORE,
    MENU_INFO,
    MENU_ABOUT,
    SETTINGS_SMS,
    SETTINGS_TIMEDATE,
    SETTINGS_TIMEDATE_SET,
    SETTINGS_DISPLAY,
    SETTINGS_GPS,
    SETTINGS_M17,
#if defined(CONFIG_DSTAR)
    SETTINGS_DSTAR,
#endif
#if defined(CONFIG_P25)
    SETTINGS_P25,
#endif
    SETTINGS_FM,
    SETTINGS_MODULE17,
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
    M_SETTINGS = 0,
#ifdef CONFIG_GPS
    M_GPS,
#endif
    M_MODE,
    M_INFO,
    M_ABOUT,
    M_SHUTDOWN
};

enum settingsItems
{
    S_DISPLAY = 0
#ifdef CONFIG_RTC
    ,S_TIMEDATE
#endif
#ifdef CONFIG_GPS
    ,S_GPS
#endif
    ,S_M17
    ,S_FM
#ifdef CONFIG_DSTAR
    ,S_DSTAR
#endif
#if defined CONFIG_P25
    ,S_P25
#endif
    ,S_MOD17
    ,S_RESET2DEFAULTS
};

enum modeItems
{
    M17
    ,FM
#ifdef CONFIG_DSTAR
    ,DSTAR
#endif
#ifdef CONFIG_P25
    ,P25
#endif
};

enum modeConfItems
{
    M_M17
    ,M_FM
#ifdef CONFIG_DSTAR
    ,M_DSTAR
#endif
#ifdef CONFIG_P25
    ,P_25
#endif
};

enum backupRestoreItems
{
    BR_BACKUP = 0,
    BR_RESTORE
};

enum displayItems
{
    D_BRIGHTNESS = 0
};

#ifdef CONFIG_GPS
enum settingsGPSItems
{
    G_ENABLED = 0,
    G_SET_TIME,
    G_TIMEZONE
};
#endif

enum fmItems
{
    M_FMRXLEVEL = 0,
    M_FMTXLEVEL,
    M_CTCSSRX,
    M_CTCSSTX,
    M_CTCSSTX_LEV,
    M_CTCSSRX_THRSHHI,
    M_CTCSSRX_THRSHLO,
    M_NOISESQ,
    M_NOISESQ_THRSHHI,
    M_NOISESQ_THRSHLO
    //   M_COSINVERT,
    //   M_ACCESSMODE,
};

enum m17smsItems
{
    M_SMSSEND = 0,
    M_SMSVIEW,
    M_SMSMATCHCALL
};

enum m17Items
{
    M_CALLSIGN = 0,
    M_METATEXT,
    M_SMS,
    M_CAN,
    M_CAN_RX
};

#ifdef CONFIG_DSTAR
enum dstarItems
{
    M_MyCall = 0,
    M_UrCall,
    M_Suffix,
    M_Rpt1Call,
    M_Rpt2Call,
    M_Message,
    M_DSTARRXLEVEL,
    M_DSTARTXLEVEL
};
#endif

#ifdef CONFIG_P25
enum p25Items
{
    M_SRCID = 0,
    M_DSTID,
    M_NAC,
    M_P25RXLEVEL,
    M_P25TXLEVEL
};
#endif

enum module17Items
{
    D_MICGAIN = 0,
    D_PTTINLEVEL,
    D_PTTOUTLEVEL,
    D_TXINVERT,
    D_RXINVERT,
    D_TXWIPER,
    D_RXWIPER
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
    uint16_t line4_h;
    uint16_t line5_h;
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
    point_t line4_pos;
    point_t line5_pos;
    point_t bottom_pos;
    fontSize_t top_font;
    symbolSize_t top_symbol_font;
    fontSize_t line1_font;
    symbolSize_t line1_symbol_font;
    fontSize_t line2_font;
    symbolSize_t line2_symbol_font;
    fontSize_t line3_font;
    symbolSize_t line3_symbol_font;
    fontSize_t line4_font;
    symbolSize_t line4_symbol_font;
    fontSize_t line5_font;
    symbolSize_t line5_symbol_font;
    fontSize_t bottom_font;
    fontSize_t input_font;
    fontSize_t message_font;
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
#if defined(CONFIG_DSTAR)
    bool edit_mycall;
    bool edit_urcall;
    bool edit_suffix;
    bool edit_rpt1call;
    bool edit_rpt2call;
#endif
    bool edit_message;
    bool edit_sms;
    bool view_sms;
#if defined(CONFIG_P25)
    bool edit_srcid;
    bool edit_dstid;
    bool edit_nac;
#endif
    // Variables used for VFO input
    uint8_t input_number;
    uint8_t input_position;
    uint8_t input_set;
    long long last_keypress;
    freq_t new_rx_frequency;
    freq_t new_tx_frequency;
    char new_rx_freq_buf[14];
    char new_tx_freq_buf[14];
#ifdef CONFIG_RTC
    // Variables used for Time & Date input
    datetime_t new_timedate;
    char new_date_buf[9];
    char new_time_buf[9];
#endif
    char new_callsign[10];
    char new_message[822];
    uint8_t currentSMSMessage;
    // Which state to return to when we exit menu
    uint8_t last_main_state;
}
ui_state_t;

extern layout_t layout;
// Copy of the radio state
extern state_t last_state;
extern const char *menu_items[];
extern const char *settings_items[];
extern const char *mode_items[];
extern const char *display_items[];
extern const char *settings_gps_items[];
extern const char *m17sms_items[];
extern const char *m17_items[];
extern const char *module17_items[];
#if defined(CONFIG_DSTAR)
extern const char *dstar_items[];
#endif
extern const char *fm_items[];
#if defined(CONFIG_P25)
extern const char *p25_items[];
#endif
extern const char *backup_restore_items[];
extern const char *info_items[];
extern const char *authors[];

extern uint8_t digital_mode;
extern const uint8_t menu_num;
extern const uint8_t settings_num;
extern const uint8_t mode_num;
extern const uint8_t mode_sel_num;
extern const uint8_t display_num;
extern const uint8_t settings_gps_num;
extern const uint8_t backup_restore_num;
extern const uint8_t fm_num;
extern const uint8_t m17sms_num;
extern const uint8_t m17_num;
extern const uint8_t module17_num;
#if defined(CONFIG_DSTAR)
extern const uint8_t dstar_num;
#endif
#if defined(CONFIG_P25)
extern const uint8_t p25_num;
#endif
extern const uint8_t info_num;
extern const uint8_t author_num;
extern const color_t color_black;
extern const color_t color_grey;
extern const color_t color_white;
extern const color_t yellow_fab413;

// Calibration data, for digital pot and phase inversion
extern mod17Calib_t mod17CalData;

#endif /* UI_MOD17_H */
