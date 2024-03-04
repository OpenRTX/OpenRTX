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

//#define DISPLAY_DEBUG_MSG

#ifndef DISPLAY_DEBUG_MSG

#define DEBUG_SET_TRACE0( traceVal )
#define DEBUG_SET_TRACE1( traceVal )
#define DEBUG_SET_TRACE2( traceVal )
#define DEBUG_SET_TRACE3( traceVal )

#else // DISPLAY_DEBUG_MSG

extern void Debug_SetTrace0( uint8_t traceVal );
extern void Debug_SetTrace1( uint8_t traceVal );
extern void Debug_SetTrace2( uint8_t traceVal );
extern void Debug_SetTrace3( uint8_t traceVal );

#define DEBUG_SET_TRACE0( traceVal )     Debug_SetTrace0( (uint8_t)traceVal );
#define DEBUG_SET_TRACE1( traceVal )     Debug_SetTrace1( (uint8_t)traceVal );
#define DEBUG_SET_TRACE2( traceVal )     Debug_SetTrace2( (uint8_t)traceVal );
#define DEBUG_SET_TRACE3( traceVal )     Debug_SetTrace3( (uint8_t)traceVal );

#endif // DISPLAY_DEBUG_MSG

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
    PAGE_MAIN_VFO                   , // 0x00
    PAGE_MAIN_VFO_INPUT             , // 0x01
    PAGE_MAIN_MEM                   , // 0x02
    PAGE_MODE_VFO                   , // 0x03
    PAGE_MODE_MEM                   , // 0x04
    PAGE_MENU_TOP                   , // 0x05
    PAGE_MENU_BANK                  , // 0x06
    PAGE_MENU_CHANNEL               , // 0x07
    PAGE_MENU_CONTACTS              , // 0x08
    PAGE_MENU_GPS                   , // 0x09
    PAGE_MENU_SETTINGS              , // 0x0A
    PAGE_MENU_BACKUP_RESTORE        , // 0x0B
    PAGE_MENU_BACKUP                , // 0x0C
    PAGE_MENU_RESTORE               , // 0x0D
    PAGE_MENU_INFO                  , // 0x0E
    PAGE_MENU_ABOUT                 , // 0x0F
    PAGE_SETTINGS_TIMEDATE          , // 0x10
    PAGE_SETTINGS_TIMEDATE_SET      , // 0x11
    PAGE_SETTINGS_DISPLAY           , // 0x12
    PAGE_SETTINGS_GPS               , // 0x13
    PAGE_SETTINGS_RADIO             , // 0x14
    PAGE_SETTINGS_M17               , // 0x15
    PAGE_SETTINGS_VOICE             , // 0x16
    PAGE_SETTINGS_RESET_TO_DEFAULTS , // 0x17
    PAGE_LOW_BAT                    , // 0x18
    PAGE_ABOUT                      , // 0x19
    PAGE_STUBBED                    , // 0x1A
    PAGE_NUM_OF                       // 0x1B
}uiPageNum_en;

enum
{
    PAGE_INITIAL = PAGE_MAIN_MEM
};

enum
{
    GUI_LINE_TOP     ,
    GUI_LINE_1       ,
    GUI_LINE_2       ,
    GUI_LINE_3       ,
    GUI_LINE_4       ,
    GUI_LINE_BOTTOM  ,
    GUI_LINE_3_LARGE ,
    GUI_LINE_NUM_OF
};

enum
{
    LINK_MAX_NUM_OF = 12
};

typedef enum
{
    LINK_TYPE_NONE  ,
    LINK_TYPE_PAGE  ,
    LINK_TYPE_VALUE
}LinkType_en;

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

enum
{
    MAX_PAGE_DEPTH = 8
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
#if defined(UI_NO_KEYBOARD)
    uint8_t     macro_menu_selected ;
#endif // UI_NO_KEYBOARD
}UI_State_st;

typedef struct
{
    FontSize_t size ;
}Font_st;

typedef struct
{
    Pos_st       pos ;
    Pos_t        height ;
    Align_t      align ;
    Font_st      font ;
    SymbolSize_t symbolSize ;
}Line_st;

typedef struct
{
    uint8_t type ;
    uint8_t num ;
    uint8_t amt ;
}Link_st;

/**
 * Struct containing a set of positions and sizes that get
 * calculated for the selected display size.
 * Using these parameters make the UI automatically adapt
 * To displays of different sizes
 */
typedef struct
{
    uint16_t hline_h ;
    uint16_t menu_h ;
    uint16_t bottom_pad ;
    uint16_t status_v_pad ;
    uint16_t horizontal_pad ;
    uint16_t text_v_offset ;
    uint8_t  numOfEntries ;
    uint8_t  scrollOffset ;
    Line_st  line ;
    Line_st  lineStyle[ GUI_LINE_NUM_OF ] ;
    uint8_t  lineIndex ;
    Link_st  links[ LINK_MAX_NUM_OF ] ;
    uint8_t  linkNumOf ;
    uint8_t  linkIndex ;
    Font_st  input_font ;
    Font_st  menu_font ;
    Font_st  mode_font_big ;
    Font_st  mode_font_small ;
    bool     printDisplayOn ;
    bool     inSelect ;
}Layout_st;

typedef struct
{
    uint8_t         num ;
    uint8_t         levelList[ MAX_PAGE_DEPTH ] ;
    uint8_t         level ;
    uint8_t*        ptr ;
    uint16_t        index ;
    bool            renderPage ;
}Page_st;

typedef struct
{
    UI_State_st     uiState ;
    Event_st        event ;
    bool            update ;
    bool            pageHasEvents ;
    bool            inEventArea ;
    bool            sync_rtx ;
    bool            handled ;
    Page_st         page ;
    Layout_st       layout ;
    kbd_msg_t       msg ;
    VPQueueFlags_en queueFlags ;
    bool            f1Handled ;
}GuiState_st;

extern GuiState_st  GuiState ;
extern State_st     last_state ;
extern bool         macro_latched ;

extern const uiPageDesc_st* uiGetPageDesc( uiPageNum_en pageNum );
extern const char**         uiGetPageLoc( uiPageNum_en pageNum );
extern uint8_t              uiGetPageNumOf( GuiState_st* guiState );

typedef int (*GetMenuList_fn)( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );

#endif /* UI_DEFAULT_H */
