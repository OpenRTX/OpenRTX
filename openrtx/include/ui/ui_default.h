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

#define ENABLE_DEBUG_MSG
//#define DISPLAY_DEBUG_MSG

// Maximum menu entry length
#define MAX_ENTRY_LEN 33
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
    PAGE_MENU_TOP_LIST              , // 0x06
    PAGE_MENU_BANK                  , // 0x07
    PAGE_MENU_BANK_LIST             , // 0x08
    PAGE_MENU_CHANNEL               , // 0x09
    PAGE_MENU_CHANNEL_LIST          , // 0x0A
    PAGE_MENU_CONTACTS              , // 0x0B
    PAGE_MENU_CONTACTS_LIST         , // 0x0C
    PAGE_MENU_GPS                   , // 0x0D
    PAGE_MENU_SETTINGS              , // 0x0E
    PAGE_MENU_SETTINGS_LIST         , // 0x0F
    PAGE_MENU_BACKUP_RESTORE        , // 0x10
    PAGE_MENU_BACKUP                , // 0x11
    PAGE_MENU_RESTORE               , // 0x12
    PAGE_MENU_INFO                  , // 0x13
    PAGE_SETTINGS_TIMEDATE          , // 0x14
    PAGE_SETTINGS_TIMEDATE_SET      , // 0x15
    PAGE_SETTINGS_DISPLAY           , // 0x16
    PAGE_SETTINGS_DISPLAY_LIST      , // 0x17
#ifdef GPS_PRESENT
    PAGE_SETTINGS_GPS               , // 0x18
#endif // GPS_PRESENT
    PAGE_SETTINGS_RADIO             , // 0x19
    PAGE_SETTINGS_M17               , // 0x1A
    PAGE_SETTINGS_VOICE             , // 0x1B
    PAGE_SETTINGS_RESET_TO_DEFAULTS , // 0x1C
    PAGE_LOW_BAT                    , // 0x1D
    PAGE_ABOUT                      , // 0x1E
    PAGE_STUBBED                    , // 0x1F
    PAGE_NUM_OF                       // 0x20
}uiPageNum_en;

enum
{
    PAGE_INITIAL = PAGE_MAIN_MEM
};

typedef enum
{
    GUI_LINE_TOP     ,
    GUI_LINE_1       ,
    GUI_LINE_2       ,
    GUI_LINE_3       ,
    GUI_LINE_4       ,
    GUI_LINE_5       ,
    GUI_LINE_BOTTOM  ,
    GUI_LINE_3_LARGE ,
    GUI_LINE_NUM_OF  ,
    GUI_LINE_DEFAULT = GUI_LINE_TOP ,
    GUI_LINE_INITIAL = GUI_LINE_TOP
}Line_en;

typedef enum
{
    GUI_LIST_LINE_NUM_OF = GUI_LINE_NUM_OF
}ListLine_en;

typedef enum
{
    GUI_STYLE_TOP     ,
    GUI_STYLE_1       ,
    GUI_STYLE_2       ,
    GUI_STYLE_3       ,
    GUI_STYLE_4       ,
    GUI_STYLE_5       ,
    GUI_STYLE_BOTTOM  ,
    GUI_STYLE_3_LARGE ,

    GUI_STYLE_GG      , // grey background
    GUI_STYLE_AGG     , // alpha grey overlay foreground
    GUI_STYLE_HL      , // highlight background
    GUI_STYLE_OP0     , // option 0 foreground
    GUI_STYLE_OP1     , // option 1 foreground
    GUI_STYLE_OP2     , // option 2 foreground
    GUI_STYLE_OP3     , // option 3 foreground
    GUI_STYLE_AL      , // alarm foreground

    GUI_STYLE_NUM_OF  ,

    GUI_STYLE_DEFAULT = GUI_STYLE_1 ,
    GUI_STYLE_INITIAL = GUI_STYLE_TOP

}Style_en;

enum
{
    GUI_LINK_NUM_OF = 12
};

typedef enum
{
    LINK_TYPE_NONE  ,
    LINK_TYPE_PAGE  ,
    LINK_TYPE_VALUE
}LinkType_en;

enum
{
    GUI_VAR_NUM_OF = 16
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

enum
{
    MAX_PAGE_DEPTH = 12
};
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
#define COLOR_DEF_BLACK         COLOR_DEF_ENC(   0 ,   0 ,  0  , 255 )
#define COLOR_DEF_WHITE         COLOR_DEF_ENC( 255 , 255 , 255 , 255 )
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
    COLOR_GG  = COLOR_GREY          , // grey background
    COLOR_AGG = COLOR_ALPHA_GREY    , // alpha grey overlay foreground
    COLOR_HL  = COLOR_YELLOW_FAB413 , // highlight background
    COLOR_OP0 = COLOR_RED           , // option 0 foreground
    COLOR_OP1 = COLOR_GREEN         , // option 1 foreground
    COLOR_OP2 = COLOR_BLUE          , // option 2 foreground
    COLOR_OP3 = COLOR_YELLOW_FAB413 , // option 3 foreground
    COLOR_AL  = COLOR_RED             // alarm foreground

}ColorSelector_en;

// Load the Color_st structure with the color variable fields
#define COLOR_LD( c , cv )                                                       \
c->red   = (uint8_t)( ( cv >> COLOR_ENC_RED_SHIFT   ) & COLOR_ENC_RED_MASK   ) ; \
c->green = (uint8_t)( ( cv >> COLOR_ENC_GREEN_SHIFT ) & COLOR_ENC_GREEN_MASK ) ; \
c->blue  = (uint8_t)( ( cv >> COLOR_ENC_BLUE_SHIFT  ) & COLOR_ENC_BLUE_MASK  ) ; \
c->alpha = (uint8_t)( ( cv >> COLOR_ENC_ALPHA_SHIFT ) & COLOR_ENC_ALPHA_MASK ) ;

/**
 * This structs contains state variables internal to the
 * UI that need to be kept between executions of the UI
 * This state does not need to be saved on device poweroff
 */
typedef struct
{
    // Index of the currently selected menu entry
    uint8_t     entrySelected ;
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
    uint8_t     macro_entrySelected ;
#endif // UI_NO_KEYBOARD
}UI_State_st;

typedef struct
{
    FontSize_t size ;
}Font_st;

typedef struct
{
    Align_t          align ;
    Font_st          font ;
    SymbolSize_t     symbolSize ;
    ColorSelector_en colorBG ;
    ColorSelector_en colorFG ;
}Style_st;

typedef struct
{
    Pos_st pos ;
    Pos_t  textY ;
    Pos_t  height ;
}Line_st;

typedef struct
{
    uint8_t pageNum ;
    uint8_t dataSource ;
    Pos_st  pos ;
    bool    displayingList ;
    uint8_t numOfDisplayedLines ;
    uint8_t numOfEntries ;
    uint8_t index ;
    uint8_t selection ;
    uint8_t offset ;
}List_st;

typedef struct
{
    uint8_t pageNum ;
}Link_st;

typedef struct
{
    uint8_t  varNum ;
    Pos_st   pos ;
    uint32_t value ;
}Var_st;

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
    Style_st style ;
    Pos_st   itemPos ;
    Line_st  lines[ GUI_LINE_NUM_OF ] ;
    Style_st styles[ GUI_STYLE_NUM_OF ] ;
    uint8_t  lineIndex ;
    List_st  list ;
    Link_st  link ;
    Var_st   vars[ GUI_VAR_NUM_OF ] ;
    uint8_t  varNumOf ;
    uint8_t  varIndex ;
    Font_st  input_font ;
    Font_st  menu_font ;
    Font_st  mode_font_big ;
    Font_st  mode_font_small ;
}Layout_st;

typedef struct
{
    uint8_t  num ;
    uint8_t  levelList[ MAX_PAGE_DEPTH ] ;
    uint8_t  level ;
    uint8_t* ptr ;
    uint16_t index ;
    bool     renderPage ;
}Page_st;

typedef struct
{
    long long int timeOut ;
    uint8_t       scriptPageNum ;
}Timer_st;

typedef struct
{
    UI_State_st     uiState ;
    Event_st        event ;
    bool            update ;
    bool            pageHasEvents ;
    bool            inEventArea ;
    bool            displayEnabledInitial ;
    bool            displayEnabled ;
    long long int   timeStamp ;
    Timer_st        timer ;
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

typedef int (*GetMenuList_fn)( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );

#endif /* UI_DEFAULT_H */
