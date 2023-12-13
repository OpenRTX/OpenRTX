/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef UI_DEFAULT_H
#define UI_DEFAULT_H

#include <stdbool.h>
#include <state.h>
#include <graphics.h>
#include <interfaces/keyboard.h>
#include <stdint.h>
#include <input.h>
#include <voicePromptUtils.h>
#include <hwconfig.h>
#include <event.h>
#include <ui.h>

// Maximum menu entry length
#define MAX_ENTRY_LEN 21
// Frequency digits
#define FREQ_DIGITS 7
// Time & Date digits
#define TIMEDATE_DIGITS 10
// Max number of UI events
#define MAX_NUM_EVENTS 16

typedef enum
{
    PAGE_MAIN_VFO                   ,
    PAGE_MAIN_VFO_INPUT             ,
    PAGE_MAIN_MEM                   ,
    PAGE_MODE_VFO                   ,
    PAGE_MODE_MEM                   ,
    PAGE_MENU_TOP                   ,
    PAGE_MENU_BANK                  ,
    PAGE_MENU_CHANNEL               ,
    PAGE_MENU_CONTACTS              ,
    PAGE_MENU_GPS                   ,
    PAGE_MENU_SETTINGS              ,
    PAGE_MENU_BACKUP_RESTORE        ,
    PAGE_MENU_BACKUP                ,
    PAGE_MENU_RESTORE               ,
    PAGE_MENU_INFO                  ,
    PAGE_MENU_ABOUT                 ,
    PAGE_SETTINGS_TIMEDATE          ,
    PAGE_SETTINGS_TIMEDATE_SET      ,
    PAGE_SETTINGS_DISPLAY           ,
    PAGE_SETTINGS_GPS               ,
    PAGE_SETTINGS_RADIO             ,
    PAGE_SETTINGS_M17               ,
    PAGE_SETTINGS_VOICE             ,
    PAGE_SETTINGS_RESET_TO_DEFAULTS ,
    PAGE_LOW_BAT                    ,
    PAGE_AUTHORS                    ,
    PAGE_BLANK                      ,
    PAGE_NUM_OF
}uiPageNum_en;

// GUI Commands
enum
{
    GUI_CMD_NULL            ,
    GUI_CMD_TEXT            ,
    GUI_CMD_LINK            ,
    GUI_CMD_VALUE           ,
    GUI_CMD_LINE_END = 0x0A ,
    GUI_CMD_END      = 0x1F ,
    GUI_CMD_NUM_OF
};

// GUI Values
enum
{
    GUI_VAL_STUBBED ,
    GUI_VAL_NUM_OF
};

typedef struct
{
    const char** loc ;
    uint8_t      numOf ;
}uiPageDesc_st;

enum SetRxTx
{
    SET_RX ,
    SET_TX
};

// This enum is needed to have item numbers that match
// menu elements even if some elements may be missing (GPS)
typedef enum
{
    M_BANK     ,
    M_CHANNEL  ,
    M_CONTACTS ,
#ifdef GPS_PRESENT
    M_GPS      ,
#endif
    M_SETTINGS ,
    M_INFO     ,
    M_ABOUT
}MenuItems_en;

typedef enum
{
    S_DISPLAY = 0,
#ifdef RTC_PRESENT
    S_TIMEDATE,
#endif
#ifdef GPS_PRESENT
    S_GPS,
#endif
    S_RADIO,
    S_M17,
    S_VOICE,
    S_RESET2DEFAULTS,
}SettingsItems_en;

typedef enum
{
    BR_BACKUP  ,
    BR_RESTORE
}BackupRestoreItems_en;

typedef enum
{
#ifdef SCREEN_BRIGHTNESS
    D_BRIGHTNESS ,
#endif
#ifdef SCREEN_CONTRAST
    D_CONTRAST   ,
#endif
    D_TIMER
}DisplayItems_en;

#ifdef GPS_PRESENT
typedef enum
{
    G_ENABLED  ,
    G_SET_TIME ,
    G_TIMEZONE
}SettingsGPSItems_en;
#endif

typedef enum
{
    VP_LEVEL    ,
    VP_PHONETIC
}SettingsVoicePromptItems_en;

typedef enum
{
    R_OFFSET    ,
    R_DIRECTION ,
    R_STEP
}SettingsRadioItems_en;

typedef enum
{
    M17_CALLSIGN ,
    M17_CAN      ,
    M17_CAN_RX
}SettingsM17Items_en;

enum
{
    MAX_SCRIPT_DEPTH = 5
};

/**
 * Struct containing a set of positions and sizes that get
 * calculated for the selected display size.
 * Using these parameters make the UI automatically adapt
 * To displays of different sizes
 */
typedef struct
{
    uint16_t     hline_h ;
    uint16_t     top_h ;
    uint16_t     line1_h ;
    uint16_t     line2_h ;
    uint16_t     line3_h ;
    uint16_t     line3_large_h ;
    uint16_t     line4_h ;
    uint16_t     menu_h ;
    uint16_t     bottom_h ;
    uint16_t     bottom_pad ;
    uint16_t     status_v_pad ;
    uint16_t     horizontal_pad ;
    uint16_t     text_v_offset ;
    point_t      top_pos ;
    point_t      line1_pos ;
    point_t      line2_pos ;
    point_t      line3_pos ;
    point_t      line3_large_pos ;
    point_t      line4_pos ;
    point_t      bottom_pos ;
    fontSize_t   top_font ;
    symbolSize_t top_symbol_size ;
    fontSize_t   line1_font ;
    symbolSize_t line1_symbol_size ;
    fontSize_t   line2_font ;
    symbolSize_t line2_symbol_size ;
    fontSize_t   line3_font ;
    symbolSize_t line3_symbol_size ;
    fontSize_t   line3_large_font ;
    fontSize_t   line4_font ;
    symbolSize_t line4_symbol_size ;
    fontSize_t   bottom_font ;
    fontSize_t   input_font ;
    fontSize_t   menu_font ;
    fontSize_t   mode_font_big ;
    fontSize_t   mode_font_small ;
}Layout_st;

typedef struct
{
    Layout_st       layout ;
    uint8_t         pageNum[ MAX_SCRIPT_DEPTH ] ;
    uint8_t         pageLevel ;
    uint8_t*        pagePtr ;
    uint16_t        pageIndex ;
    kbd_msg_t       msg ;
    VPQueueFlags_en queueFlags ;
    bool            f1Handled ;
}GuiState_st;

/**
 * This structs contains state variables internal to the
 * UI that need to be kept between executions of the UI
 * This state does not need to be saved on device poweroff
 */
typedef struct
{
    // Index of the currently selected menu entry
    uint8_t     menu_selected ;
    // If true we can change a menu entry value with UP/DOWN
    bool        edit_mode ;
    bool        input_locked ;
    // Variables used for VFO input
    KeyNum_en   input_number ;
    uint8_t     input_position ;
    uint8_t     input_set ;
    long long   last_keypress ;
    freq_t      new_rx_frequency ;
    freq_t      new_tx_frequency ;
    char        new_rx_freq_buf[ 14 ] ;
    char        new_tx_freq_buf[ 14 ] ;
#ifdef RTC_PRESENT
    // Variables used for Time & Date input
    datetime_t  new_timedate ;
    char        new_date_buf[ 9 ] ;
    char        new_time_buf[ 9 ] ;
#endif
    char        new_callsign[ 10 ] ;
    freq_t      new_offset ;
    // Which state to return to when we exit menu
    uint8_t     last_main_state ;
#if defined(UI_NO_KEYBOARD)
    uint8_t     macro_menu_selected ;
#endif // UI_NO_KEYBOARD
}UI_State_st;

extern GuiState_st      GuiState ;
extern State_st         last_state ;
extern bool             macro_latched ;
extern const char*      Page_MenuItems[] ;
extern const char*      Page_MenuSettings[] ;
extern const char*      Page_SettingsDisplay[] ;
extern const char*      Page_SettingsGPS[] ;
extern const char*      Page_SettingsRadio[] ;
extern const char*      Page_SettingsM17[] ;
extern const char*      Page_SettingsVoice[] ;

extern const char*      Page_MenuBackupRestore[] ;
extern const char*      Page_MenuInfo[] ;
extern const char*      authors[] ;

extern const color_t    Color_Black ;
extern const color_t    Color_Grey ;
extern const color_t    Color_White ;
//@@@KL
extern const color_t    Color_Red ;
extern const color_t    Color_Green ;
extern const color_t    Color_Blue ;

extern const color_t    Color_Yellow_Fab413 ;

extern const uiPageDesc_st* uiGetPageDesc( uiPageNum_en pageNum );
extern const char**         uiGetPageLoc( uiPageNum_en pageNum );
extern uint8_t              uiGetPageNumOf( uiPageNum_en pageNum );

#endif /* UI_DEFAULT_H */
