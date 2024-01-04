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
    GUI_CMD_TITLE           ,
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
    uint8_t      numOfEntries ;
    uint8_t      itemIndex ;
    uint8_t      scrollOffset ;
    point_t      pos ;
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
    uint8_t         num[ MAX_SCRIPT_DEPTH ] ;
    uint8_t         level ;
    uint8_t*        ptr ;
    uint16_t        index ;
}Page_st;

typedef struct
{
    UI_State_st     uiState ;
    Layout_st       layout ;
    Page_st         page ;
    kbd_msg_t       msg ;
    VPQueueFlags_en queueFlags ;
    bool            f1Handled ;
}GuiState_st;

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

// Color Variable Field Unshifted Masks and Shifts
enum
{
    COLOR_ENC_RED_SHIFT   =   24 ,
    COLOR_ENC_RED_MASK    = 0xFF ,
    COLOR_ENC_GREEN_SHIFT =   16 ,
    COLOR_ENC_GREEN_MASK  = 0xFF ,
    COLOR_ENC_BLUE_SHIFT  =    8 ,
    COLOR_ENC_BLUE_MASK   = 0xFF ,
    COLOR_ENC_ALPHA_SHIFT =    0 ,
    COLOR_ENC_ALPHA_MASK  = 0xFF
};

// Color Variable Field Encoder
#define COLOR_DEF_ENC( r , g , b , a ) (const uint32_t)(                        \
    ( ( (const uint32_t)r & COLOR_ENC_RED_MASK   ) << COLOR_ENC_RED_SHIFT   ) | \
    ( ( (const uint32_t)g & COLOR_ENC_GREEN_MASK ) << COLOR_ENC_GREEN_SHIFT ) | \
    ( ( (const uint32_t)b & COLOR_ENC_BLUE_MASK  ) << COLOR_ENC_BLUE_SHIFT  ) | \
    ( ( (const uint32_t)a & COLOR_ENC_ALPHA_MASK ) << COLOR_ENC_ALPHA_SHIFT )   )

// Color Definitions
#define COLOR_DEF_WHITE         COLOR_DEF_ENC( 255 , 255 , 255 , 255 )
#define COLOR_DEF_BLACK         COLOR_DEF_ENC(   0 ,   0 ,  0  , 255 )
#define COLOR_DEF_GREY          COLOR_DEF_ENC(  60 ,  60 ,  60 , 255 )
#define COLOR_DEF_ALPHA_GREY    COLOR_DEF_ENC(   0 ,   0 ,   0 , 255 )
#define COLOR_DEF_YELLOW_FAB413 COLOR_DEF_ENC( 250 , 180 ,  19 , 255 )
#define COLOR_DEF_RED           COLOR_DEF_ENC( 255 ,   0 ,   0 , 255 )
#define COLOR_DEF_GREEN         COLOR_DEF_ENC(   0 , 255 ,   0 , 255 )
#define COLOR_DEF_BLUE          COLOR_DEF_ENC(   0 ,   0 , 255 , 255 )

// Color Table Definition
#define COLOR_TABLE             \
    COLOR_DEF_BLACK         ,   \
    COLOR_DEF_WHITE         ,   \
    COLOR_DEF_GREY          ,   \
    COLOR_DEF_ALPHA_GREY    ,   \
    COLOR_DEF_YELLOW_FAB413 ,   \
    COLOR_DEF_RED           ,   \
    COLOR_DEF_GREEN         ,   \
    COLOR_DEF_BLUE

// Color Selectors
typedef enum
{
    COLOR_BLACK         ,
    COLOR_WHITE         ,
    COLOR_GREY          ,
    COLOR_ALPHA_GREY    ,
    COLOR_YELLOW_FAB413 ,
    COLOR_RED           ,
    COLOR_GREEN         ,
    COLOR_BLUE          ,
    COLOR_NUM_OF        ,

    COLOR_BG  = COLOR_BLACK         , // background
    COLOR_FG  = COLOR_WHITE         , // foreground
    COLOR_GG  = COLOR_GREY          , // grey ground
    COLOR_AGG = COLOR_ALPHA_GREY    , // alpha grey ground
    COLOR_HL  = COLOR_YELLOW_FAB413 , // highlight
    COLOR_OP0 = COLOR_RED           , // option 0
    COLOR_OP1 = COLOR_GREEN         , // option 1
    COLOR_OP2 = COLOR_BLUE          , // option 2
    COLOR_OP3 = COLOR_YELLOW_FAB413 , // option 3
    COLOR_AL  = COLOR_RED             // alarm

}ColorSelector_en;

// Load the color_t structure with the color variable fields
#define COLOR_LD( c , cv )                                                       \
c->r     = (uint8_t)( ( cv >> COLOR_ENC_RED_SHIFT   ) & COLOR_ENC_RED_MASK   ) ; \
c->g     = (uint8_t)( ( cv >> COLOR_ENC_GREEN_SHIFT ) & COLOR_ENC_GREEN_MASK ) ; \
c->b     = (uint8_t)( ( cv >> COLOR_ENC_BLUE_SHIFT  ) & COLOR_ENC_BLUE_MASK  ) ; \
c->alpha = (uint8_t)( ( cv >> COLOR_ENC_ALPHA_SHIFT ) & COLOR_ENC_ALPHA_MASK ) ;

// Color load fn.
extern void uiColorLoad( color_t* color , ColorSelector_en colorSelector );

extern const uiPageDesc_st* uiGetPageDesc( uiPageNum_en pageNum );
extern const char**         uiGetPageLoc( uiPageNum_en pageNum );
extern uint8_t              uiGetPageNumOf( uiPageNum_en pageNum );

typedef int (*GetMenuList_fn)( char* buf , uint8_t max_len , uint8_t index );

typedef enum
{
    ENTRY_NAME_MENU_TOP       ,
    ENTRY_NAME_BANK_NAME      ,
    ENTRY_NAME_CHANNEL_NAME   ,
    ENTRY_NAME_CONTACT_NAME   ,
    ENTRY_NAME_SETTINGS       ,
    ENTRY_NAME_BACKUP_RESTORE ,
    ENTRY_NAME_INFO           ,
    ENTRY_NAME_DISPLAY        ,
    ENTRY_NAME_SETTINGS_GPS   ,
    ENTRY_NAME_M17            ,
    ENTRY_NAME_VOICE          ,
    ENTRY_NAME_RADIO          ,
    ENTRY_NAME_NUM_OF
}EntryName_en;

typedef enum
{
    ENTRY_VALUE_INFO         ,
    ENTRY_VALUE_DISPLAY      ,
    ENTRY_VALUE_SETTINGS_GPS ,
    ENTRY_VALUE_M17          ,
    ENTRY_VALUE_VOICE        ,
    ENTRY_VALUE_RADIO        ,
    ENTRY_VALUE_NUM_OF
}EntryValue_en;

#endif /* UI_DEFAULT_H */
