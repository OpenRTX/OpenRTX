/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
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
#include <math.h>
#include <ui/ui_default.h>
#include <rtx.h>
#include <interfaces/platform.h>
#include <interfaces/display.h>
#include <interfaces/cps_io.h>
#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include <string.h>
#include <battery.h>
#include <input.h>
#include <utils.h>
#include <hwconfig.h>
#include <voicePromptUtils.h>
#include <beeps.h>

//@@@KL #include "ui_m17.h"

#define DISPLAY_DEBUG_MSG

#ifdef DISPLAY_DEBUG_MSG

static char    counter = 0 ; //@@@KL
static uint8_t trace   = 0 ; //@@@KL

static void DisplayDebugMsg( void )
{
    //@@@KL
    if( counter < 10 )
    {
        counter++ ;
    }
    else
    {
        counter = 0 ;
    }
    gfx_print( layout.top_pos , layout.top_font , TEXT_ALIGN_LEFT , color_white ,
               "%c%d" , (char)( '0' + counter ) , trace );//@@@KL
}

#endif // DISPLAY_DEBUG_MSG



/* UI main screen functions, their implementation is in "ui_main.c" */
extern void ui_draw( state_t* state , ui_state_st* ui_state );
extern bool _ui_drawMacroMenu( ui_state_st* ui_state );
extern void _ui_reset_menu_anouncement_tracking( void );

const char *menu_items[] =
{
    "Banks"    ,
    "Channels" ,
    "Contacts" ,
#ifdef GPS_PRESENT
    "GPS"      ,
#endif // RTC_PRESENT
    "Settings" ,
    "Info"     ,
    "About"
};

const char *settings_items[] =
{
    "Display"          ,
#ifdef RTC_PRESENT
    "Time & Date"      ,
#endif // RTC_PRESENT
#ifdef GPS_PRESENT
    "GPS"              ,
#endif // GPS_PRESENT
    "Radio"            ,
    "M17"              ,
    "Accessibility"    ,
    "Default Settings"
};

const char *display_items[] =
{
#ifdef SCREEN_BRIGHTNESS
    "Brightness" ,
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
    "Contrast"   ,
#endif // SCREEN_CONTRAST
    "Timer"
};

#ifdef GPS_PRESENT
const char *settings_gps_items[] =
{
    "GPS Enabled"  ,
    "GPS Set Time" ,
    "UTC Timezone"
};
#endif // GPS_PRESENT

const char *settings_radio_items[] =
{
    "Offset"    ,
    "Direction" ,
    "Step"
};

const char * settings_m17_items[] =
{
    "Callsign"     ,
    "CAN"          ,
    "CAN RX Check"
};

const char * settings_voice_items[] =
{
    "Voice"    ,
    "Phonetic"
};

const char *backup_restore_items[] =
{
    "Backup"  ,
    "Restore"
};

const char *info_items[] =
{
    ""             ,
    "Bat. Voltage" ,
    "Bat. Charge"  ,
    "RSSI"         ,
    "Used heap"    ,
    "Band"         ,
    "VHF"          ,
    "UHF"          ,
    "Hw Version"   ,
#ifdef PLATFORM_TTWRPLUS
    "Radio"        ,
    "Radio FW"     ,
#endif // PLATFORM_TTWRPLUS
};

const char *authors[] =
{
    "Niccolo' IU2KIN" ,
    "Silvano IU2KWO"  ,
    "Federico IU2NUO" ,
    "Fred IU2NRO"     ,
    "Kim VK6KL"         //@@@KL
};

static const char *symbols_ITU_T_E161[] =
{
    " 0"        ,
    ",.?1"      ,
    "abc2ABC"   ,
    "def3DEF"   ,
    "ghi4GHI"   ,
    "jkl5JKL"   ,
    "mno6MNO"   ,
    "pqrs7PQRS" ,
    "tuv8TUV"   ,
    "wxyz9WXYZ" ,
    "-/*"       ,
    "#"
};

static const char *symbols_ITU_T_E161_callsign[] =
{
    "0 "    ,
    "1"     ,
    "ABC2"  ,
    "DEF3"  ,
    "GHI4"  ,
    "JKL5"  ,
    "MNO6"  ,
    "PQRS7" ,
    "TUV8"  ,
    "WXYZ9" ,
    "-/"    ,
    ""
};

const char* page_stubbed[] =
{
    "Page Stubbed"
};

#define PAGE_DESC_DEF( loc )    { loc , sizeof( loc ) / sizeof( loc[ 0 ] ) }

const uiPageDesc_st uiPageDescTable[] =
{
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MAIN_VFO
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MAIN_VFO_INPUT
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MAIN_MEM
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MODE_VFO
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MODE_MEM
    PAGE_DESC_DEF( menu_items           ) , // PAGE_MENU_TOP
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MENU_BANK
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MENU_CHANNEL
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MENU_CONTACTS
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MENU_GPS
    PAGE_DESC_DEF( settings_items       ) , // PAGE_MENU_SETTINGS
    PAGE_DESC_DEF( backup_restore_items ) , // PAGE_MENU_BACKUP_RESTORE
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MENU_BACKUP
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MENU_RESTORE
    PAGE_DESC_DEF( info_items           ) , // PAGE_MENU_INFO
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_MENU_ABOUT
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_SETTINGS_TIMEDATE
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_SETTINGS_TIMEDATE_SET
    PAGE_DESC_DEF( display_items        ) , // PAGE_SETTINGS_DISPLAY
#ifdef GPS_PRESENT
    PAGE_DESC_DEF( settings_gps_items   ) , // PAGE_SETTINGS_GPS
#endif // GPS_PRESENT
    PAGE_DESC_DEF( settings_radio_items ) , // PAGE_SETTINGS_RADIO
    PAGE_DESC_DEF( settings_m17_items   ) , // PAGE_SETTINGS_M17
    PAGE_DESC_DEF( settings_voice_items ) , // PAGE_SETTINGS_VOICE
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_SETTINGS_RESET_TO_DEFAULTS
    PAGE_DESC_DEF( page_stubbed         ) , // PAGE_LOW_BAT
    PAGE_DESC_DEF( authors              ) , // PAGE_AUTHORS
    PAGE_DESC_DEF( page_stubbed         )   // PAGE_BLANK
};

const color_t color_black   = {   0 ,   0 ,  0  , 255 };
const color_t color_grey    = {  60 ,  60 ,  60 , 255 };
const color_t color_white   = { 255 , 255 , 255 , 255 };
const color_t yellow_fab413 = { 250 , 180 ,  19 , 255 };
//@@@KL
const color_t color_red     = { 255 ,   0 ,   0 , 255 };
const color_t color_green   = {   0 , 255 ,   0 , 255 };
const color_t color_blue    = {   0 ,   0 , 255 , 255 };

       state_t     last_state ;
       bool        macro_latched ;
static ui_state_st ui_state ;
static bool        macro_menu      = false ;
static bool        redraw_needed   = true ;

static bool        standby         = false ;
static long long   last_event_tick = 0 ;

// UI event queue
static uint8_t     evQueue_rdPos ;
static uint8_t     evQueue_wrPos ;
static event_t     evQueue[ MAX_NUM_EVENTS ] ;

enum
{
    // Horizontal line height
    SCREEN_HLINE_H       = 1 ,
    // Compensate for fonts printing below the start position
    SCREEN_TEXT_V_OFFSET = 1
};

// Calculate UI layout depending on vertical resolution
// Tytera MD380, MD-UV380
#if SCREEN_HEIGHT > 127
enum
{
    // Height and padding shown in diagram at beginning of file
    SCREEN_TOP_H               = 16 ,
    SCREEN_TOP_PAD             =  4 ,
    SCREEN_LINE1_H             = 20 ,
    SCREEN_LINE2_H             = 20 ,
    SCREEN_LINE3_H             = 20 ,
    SCREEN_LINE3_LARGE_H       = 40 ,
    SCREEN_LINE4_H             = 20 ,
    SCREEN_MENU_H              = 16 ,
    SCREEN_BOTTOM_H            = 23 ,
    SCREEN_BOTTOM_PAD          = SCREEN_TOP_PAD ,
    SCREEN_STATUS_V_PAD        =  2 ,
    SCREEN_SMALL_LINE_V_PAD    =  2 ,
    SCREEN_BIG_LINE_V_PAD      =  6 ,
    SCREEN_HORIZONTAL_PAD      =  4 ,
    // Top bar font: 8 pt
    SCREEN_TOP_FONT            = FONT_SIZE_8PT     , // fontSize_t
    SCREEN_TOP_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // symbolSize_t
    // Text line font: 8 pt
    SCREEN_LINE1_FONT          = FONT_SIZE_8PT     , // fontSize_t
    SCREEN_LINE1_SYMBOL_SIZE   = SYMBOLS_SIZE_8PT  , // symbolSize_t
    SCREEN_LINE2_FONT          = FONT_SIZE_8PT     , // fontSize_t
    SCREEN_LINE2_SYMBOL_SIZE   = SYMBOLS_SIZE_8PT  , // symbolSize_t
    SCREEN_LINE3_FONT          = FONT_SIZE_8PT     , // fontSize_t
    SCREEN_LINE3_SYMBOL_SIZE   = SYMBOLS_SIZE_8PT  , // symbolSize_t
    SCREEN_LINE4_FONT          = FONT_SIZE_8PT     , // fontSize_t
    SCREEN_LINE4_SYMBOL_SIZE   = SYMBOLS_SIZE_8PT  , // symbolSize_t
    // Frequency line font: 16 pt
    SCREEN_LINE3_LARGE_FONT    = FONT_SIZE_16PT    , // fontSize_t
    // Bottom bar font: 8 pt
    SCREEN_BOTTOM_FONT         = FONT_SIZE_8PT     , // fontSize_t
    // TimeDate/Frequency input font
    SCREEN_INPUT_FONT          = FONT_SIZE_12PT    , // fontSize_t
    // Menu font
    SCREEN_MENU_FONT           = FONT_SIZE_8PT     , // fontSize_t
    // Mode screen frequency font: 12 pt
    SCREEN_MODE_FONT_BIG       = FONT_SIZE_12PT    , // fontSize_t
    // Mode screen details font: 9 pt
    SCREEN_MODE_FONT_SMALL     = FONT_SIZE_9PT       // fontSize_t
};

// Radioddity GD-77
#elif SCREEN_HEIGHT > 63
enum
{
    // Height and padding shown in diagram at beginning of file
    SCREEN_TOP_H               = 11 ,
    SCREEN_TOP_PAD             =  1 ,
    SCREEN_LINE1_H             = 10 ,
    SCREEN_LINE2_H             = 10 ,
    SCREEN_LINE3_H             = 10 ,
    SCREEN_LINE3_LARGE_H       = 16 ,
    SCREEN_LINE4_H             = 10 ,
    SCREEN_MENU_H              = 10 ,
    SCREEN_BOTTOM_H            = 15 ,
    SCREEN_BOTTOM_PAD          =  0 ,
    SCREEN_STATUS_V_PAD        =  1 ,
    SCREEN_SMALL_LINE_V_PAD    =  1 ,
    SCREEN_BIG_LINE_V_PAD      =  0 ,
    SCREEN_HORIZONTAL_PAD      =  4 ,
    // Top bar font: 6 pt
    SCREEN_TOP_FONT            = FONT_SIZE_6PT     , // fontSize_t
    SCREEN_TOP_SYMBOL_SIZE     = SYMBOLS_SIZE_6PT  , // symbolSize_t
    // Middle line fonts: 5, 8, 8 pt
    SCREEN_LINE1_FONT          = FONT_SIZE_6PT     , // fontSize_t
    SCREEN_LINE1_SYMBOL_SIZE   = SYMBOLS_SIZE_6PT  , // symbolSize_t
    SCREEN_LINE2_FONT          = FONT_SIZE_6PT     , // fontSize_t
    SCREEN_LINE2_SYMBOL_SIZE   = SYMBOLS_SIZE_6PT  , // symbolSize_t
    SCREEN_LINE3_FONT          = FONT_SIZE_6PT     , // fontSize_t
    SCREEN_LINE3_SYMBOL_SIZE   = SYMBOLS_SIZE_6PT  , // symbolSize_t
    SCREEN_LINE3_LARGE_FONT    = FONT_SIZE_10PT    , // fontSize_t
    SCREEN_LINE4_FONT          = FONT_SIZE_6PT     , // fontSize_t
    SCREEN_LINE4_SYMBOL_SIZE   = SYMBOLS_SIZE_6PT  , // symbolSize_t
    // Bottom bar font: 6 pt
    SCREEN_BOTTOM_FONT         = FONT_SIZE_6PT     , // fontSize_t
    // TimeDate/Frequency input font
    SCREEN_INPUT_FONT          = FONT_SIZE_8PT     , // fontSize_t
    // Menu font
    SCREEN_MENU_FONT           = FONT_SIZE_6PT     , // fontSize_t
    // Mode screen frequency font: 9 pt
    SCREEN_MODE_FONT_BIG       = FONT_SIZE_9PT     , // fontSize_t
    // Mode screen details font: 6 pt
    SCREEN_MODE_FONT_SMALL     = FONT_SIZE_6PT       // fontSize_t
};

// Radioddity RD-5R
#elif SCREEN_HEIGHT > 47
enum
{
    // Height and padding shown in diagram at beginning of file
    SCREEN_TOP_H               = 11 ,
    SCREEN_TOP_PAD             =  1 ,
    SCREEN_LINE1_H             =  0 ,
    SCREEN_LINE2_H             = 10 ,
    SCREEN_LINE3_H             = 10 ,
    SCREEN_LINE3_LARGE_H       = 18 ,
    SCREEN_LINE4_H             = 10 ,
    SCREEN_MENU_H              = 10 ,
    SCREEN_BOTTOM_H            =  0 ,
    SCREEN_BOTTOM_PAD          =  0 ,
    SCREEN_STATUS_V_PAD        =  1 ,
    SCREEN_SMALL_LINE_V_PAD    =  1 ,
    SCREEN_BIG_LINE_V_PAD      =  0 ,
    SCREEN_HORIZONTAL_PAD      =  4 ,
    // Top bar font: 6 pt
    SCREEN_TOP_FONT            = FONT_SIZE_6PT     , // fontSize_t
    SCREEN_TOP_SYMBOL_SIZE     = SYMBOLS_SIZE_6PT  , // symbolSize_t
    // Middle line fonts: 16, 16
    SCREEN_LINE2_FONT          = FONT_SIZE_6PT     , // fontSize_t
    SCREEN_LINE3_FONT          = FONT_SIZE_6PT     , // fontSize_t
    SCREEN_LINE4_FONT          = FONT_SIZE_6PT     , // fontSize_t
    SCREEN_LINE3_LARGE_FONT    = FONT_SIZE_12PT    , // fontSize_t
    // TimeDate/Frequency input font
    SCREEN_INPUT_FONT          = FONT_SIZE_8PT     , // fontSize_t
    // Menu font
    SCREEN_MENU_FONT           = FONT_SIZE_6PT     , // fontSize_t
    // Mode screen frequency font: 9 pt
    SCREEN_MODE_FONT_BIG       = FONT_SIZE_9PT     , // fontSize_t
    // Mode screen details font: 6 pt
    SCREEN_MODE_FONT_SMALL     = FONT_SIZE_6PT     , // fontSize_t
    // Not present on this resolution
    SCREEN_LINE1_FONT          =  0                , // fontSize_t
    SCREEN_BOTTOM_FONT         =  0                  // fontSize_t
};
#else
    #error Unsupported vertical resolution!
#endif

// Calculate printing positions
#define SCREEN_DEF_TOP_POS          { SCREEN_HORIZONTAL_PAD , SCREEN_TOP_H - SCREEN_STATUS_V_PAD - SCREEN_TEXT_V_OFFSET }
#define SCREEN_DEF_LINE1_POS        { SCREEN_HORIZONTAL_PAD , SCREEN_TOP_H + SCREEN_TOP_PAD + SCREEN_LINE1_H - SCREEN_SMALL_LINE_V_PAD - SCREEN_TEXT_V_OFFSET }
#define SCREEN_DEF_LINE2_POS        { SCREEN_HORIZONTAL_PAD , SCREEN_TOP_H + SCREEN_TOP_PAD + SCREEN_LINE1_H + SCREEN_LINE2_H - SCREEN_SMALL_LINE_V_PAD - SCREEN_TEXT_V_OFFSET }
#define SCREEN_DEF_LINE3_POS        { SCREEN_HORIZONTAL_PAD , SCREEN_TOP_H + SCREEN_TOP_PAD + SCREEN_LINE1_H + SCREEN_LINE2_H + SCREEN_LINE3_H - SCREEN_SMALL_LINE_V_PAD - SCREEN_TEXT_V_OFFSET }
#define SCREEN_DEF_LINE4_POS        { SCREEN_HORIZONTAL_PAD , SCREEN_TOP_H + SCREEN_TOP_PAD + SCREEN_LINE1_H + SCREEN_LINE2_H + SCREEN_LINE3_H + SCREEN_LINE4_H - SCREEN_SMALL_LINE_V_PAD - SCREEN_TEXT_V_OFFSET }
#define SCREEN_DEF_LINE3_LARGE_POS  { SCREEN_HORIZONTAL_PAD , SCREEN_TOP_H + SCREEN_TOP_PAD + SCREEN_LINE1_H + SCREEN_LINE2_H + SCREEN_LINE3_LARGE_H - SCREEN_BIG_LINE_V_PAD - SCREEN_TEXT_V_OFFSET }
#define SCREEN_DEF_BOTTOM_POS       { SCREEN_HORIZONTAL_PAD , SCREEN_HEIGHT - SCREEN_BOTTOM_PAD - SCREEN_STATUS_V_PAD - SCREEN_TEXT_V_OFFSET }

layout_t layout =
{
    SCREEN_HLINE_H             ,
    SCREEN_TOP_H               ,
    SCREEN_LINE1_H             ,
    SCREEN_LINE2_H             ,
    SCREEN_LINE3_H             ,
    SCREEN_LINE3_LARGE_H       ,
    SCREEN_LINE4_H             ,
    SCREEN_MENU_H              ,
    SCREEN_BOTTOM_H            ,
    SCREEN_BOTTOM_PAD          ,
    SCREEN_STATUS_V_PAD        ,
    SCREEN_HORIZONTAL_PAD      ,
    SCREEN_TEXT_V_OFFSET       ,
    SCREEN_DEF_TOP_POS         ,
    SCREEN_DEF_LINE1_POS       ,
    SCREEN_DEF_LINE2_POS       ,
    SCREEN_DEF_LINE3_POS       ,
    SCREEN_DEF_LINE3_LARGE_POS ,
    SCREEN_DEF_LINE4_POS       ,
    SCREEN_DEF_BOTTOM_POS      ,
    SCREEN_TOP_FONT            ,
    SCREEN_TOP_SYMBOL_SIZE     ,
    SCREEN_LINE1_FONT          ,
    SCREEN_LINE1_SYMBOL_SIZE   ,
    SCREEN_LINE2_FONT          ,
    SCREEN_LINE2_SYMBOL_SIZE   ,
    SCREEN_LINE3_FONT          ,
    SCREEN_LINE3_SYMBOL_SIZE   ,
    SCREEN_LINE3_LARGE_FONT    ,
    SCREEN_LINE4_FONT          ,
    SCREEN_LINE4_SYMBOL_SIZE   ,
    SCREEN_BOTTOM_FONT         ,
    SCREEN_INPUT_FONT          ,
    SCREEN_MENU_FONT           ,
    SCREEN_MODE_FONT_BIG       ,
    SCREEN_MODE_FONT_SMALL
};

typedef struct
{
    kbd_msg_t      msg ;
    vpQueueFlags_t queueFlags ;
    bool           f1Handled ;
}GuiState_st;

static bool ui_updateFSM_PAGE_MAIN_VFO( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MAIN_VFO_INPUT( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MAIN_MEM( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MODE_VFO( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MODE_MEM( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MENU_TOP( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MENU_BANK( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MENU_CHANNEL( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MENU_CONTACTS( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MENU_GPS( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MENU_SETTINGS( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MENU_BACKUP_RESTORE( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MENU_BACKUP( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MENU_RESTORE( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MENU_INFO( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_MENU_ABOUT( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE_SET( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_SETTINGS_DISPLAY( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_SETTINGS_GPS( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_SETTINGS_RADIO( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_SETTINGS_M17( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_SETTINGS_VOICE( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_SETTINGS_RESET_TO_DEFAULTS( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_LOW_BAT( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_AUTHORS( GuiState_st* guiState , ui_state_st* uiState );
static bool ui_updateFSM_PAGE_BLANK( GuiState_st* guiState , ui_state_st* uiState );

typedef bool (*ui_updateFSM_PAGE_fn)( GuiState_st* guiState , ui_state_st* uiState );

static const ui_updateFSM_PAGE_fn ui_updateFSM_PAGE_T[ PAGE_NUM_OF ] =
{
    ui_updateFSM_PAGE_MAIN_VFO                   ,
    ui_updateFSM_PAGE_MAIN_VFO_INPUT             ,
    ui_updateFSM_PAGE_MAIN_MEM                   ,
    ui_updateFSM_PAGE_MODE_VFO                   ,
    ui_updateFSM_PAGE_MODE_MEM                   ,
    ui_updateFSM_PAGE_MENU_TOP                   ,
    ui_updateFSM_PAGE_MENU_BANK                  ,
    ui_updateFSM_PAGE_MENU_CHANNEL               ,
    ui_updateFSM_PAGE_MENU_CONTACTS              ,
    ui_updateFSM_PAGE_MENU_GPS                   ,
    ui_updateFSM_PAGE_MENU_SETTINGS              ,
    ui_updateFSM_PAGE_MENU_BACKUP_RESTORE        ,
    ui_updateFSM_PAGE_MENU_BACKUP                ,
    ui_updateFSM_PAGE_MENU_RESTORE               ,
    ui_updateFSM_PAGE_MENU_INFO                  ,
    ui_updateFSM_PAGE_MENU_ABOUT                 ,
    ui_updateFSM_PAGE_SETTINGS_TIMEDATE          ,
    ui_updateFSM_PAGE_SETTINGS_TIMEDATE_SET      ,
    ui_updateFSM_PAGE_SETTINGS_DISPLAY           ,
    ui_updateFSM_PAGE_SETTINGS_GPS               ,
    ui_updateFSM_PAGE_SETTINGS_RADIO             ,
    ui_updateFSM_PAGE_SETTINGS_M17               ,
    ui_updateFSM_PAGE_SETTINGS_VOICE             ,
    ui_updateFSM_PAGE_SETTINGS_RESET_TO_DEFAULTS ,
    ui_updateFSM_PAGE_LOW_BAT                    ,
    ui_updateFSM_PAGE_AUTHORS                    ,
    ui_updateFSM_PAGE_BLANK
};

const uiPageDesc_st* uiGetPageDesc( uiPageNum_en pageNum )
{
    return &uiPageDescTable[ pageNum ];
}

const char** uiGetPageLoc( uiPageNum_en pageNum )
{
    uiPageNum_en pgNum = pageNum ;

    if( pgNum >= PAGE_NUM_OF )
    {
        pgNum = PAGE_BLANK ;
    }

    return uiPageDescTable[ pgNum ].loc ;
}

uint8_t uiGetPageNumOf( uiPageNum_en pageNum )
{
    uiPageNum_en pgNum = pageNum ;

    if( pgNum >= PAGE_NUM_OF )
    {
        pgNum = PAGE_BLANK ;
    }

    return uiPageDescTable[ pgNum ].numOf ;
}

static freq_t _ui_freq_add_digit( freq_t freq , uint8_t pos , uint8_t number )
{
    freq_t coefficient = 100 ;

    for( uint8_t index = 0 ; index < FREQ_DIGITS - pos ; index++ )
    {
        coefficient *= 10 ;
    }

    return freq += number * coefficient ;

}

#ifdef RTC_PRESENT
static void _ui_timedate_add_digit( datetime_t* timedate ,
                                    uint8_t     pos      ,
                                    uint8_t     number     )
{
    vp_flush();
    vp_queueInteger( number );
    if( ( pos == 2 ) || ( pos == 4 ) )
    {
        vp_queuePrompt( PROMPT_SLASH );
    }
    // just indicates separation of date and time.
    if( pos == 6 ) // start of time.
    {
        vp_queueString( "hh:mm" , vpAnnounceCommonSymbols | vpAnnounceLessCommonSymbols );
    }
    if( pos == 8 )
    {
        vp_queuePrompt( PROMPT_COLON );
    }
    vp_play();

    switch( pos )
    {
        // Set date
        case 1:
        {
            timedate->date += number * 10 ;
            break ;
        }
        case 2:
        {
            timedate->date += number ;
            break ;
        }
        // Set month
        case 3:
        {
            timedate->month += number * 10 ;
            break ;
        }
        case 4:
        {
            timedate->month += number ;
            break ;
        }
        // Set year
        case 5:
        {
            timedate->year += number * 10 ;
            break ;
        }
        case 6:
        {
            timedate->year += number ;
            break ;
        }
        // Set hour
        case 7:
        {
            timedate->hour += number * 10 ;
            break ;
        }
        case 8:
        {
            timedate->hour += number ;
            break ;
        }
        // Set minute
        case 9:
        {
            timedate->minute += number * 10 ;
            break ;
        }
        case 10:
        {
            timedate->minute += number ;
            break ;
        }
    }
}
#endif

static bool _ui_freq_check_limits( freq_t freq )
{
          bool      valid  = false ;
    const hwInfo_t* hwinfo = platform_getHwInfo();

    if( hwinfo->vhf_band )
    {
        // hwInfo_t frequencies are in MHz
        if( ( freq >= ( hwinfo->vhf_minFreq * 1000000 ) ) &&
            ( freq <= ( hwinfo->vhf_maxFreq * 1000000 ) )    )
        {
            valid = true ;
        }
    }
    if( hwinfo->uhf_band )
    {
        // hwInfo_t frequencies are in MHz
        if( ( freq >= ( hwinfo->uhf_minFreq * 1000000 ) ) &&
            ( freq <= ( hwinfo->uhf_maxFreq * 1000000 ) )    )
        {
            valid = true ;
        }
    }
    return valid ;
}

static bool _ui_channel_valid( channel_t* channel )
{
    bool valid = false ;

    if( ( _ui_freq_check_limits( channel->rx_frequency ) ) &&
        ( _ui_freq_check_limits( channel->tx_frequency ) )    )
    {
       valid = true ;
    }

    return valid ;
}

static bool _ui_drawDarkOverlay( void )
{
    color_t alpha_grey = { 0 , 0 , 0,  255 };
    point_t origin     = { 0 , 0 };

    gfx_drawRect( origin , SCREEN_WIDTH , SCREEN_HEIGHT , alpha_grey , true );

    return true;

}

static int _ui_fsm_loadChannel( int16_t channel_index , bool* sync_rtx )
{
    channel_t channel;
    int32_t   selected_channel = channel_index ;
    int       result ;

    // If a bank is active, get index from current bank
    if( state.bank_enabled )
    {
        bankHdr_t bank = { 0 };
        cps_readBankHeader( &bank, state.bank );
        if( (channel_index < 0 ) || ( channel_index >= bank.ch_count ) )
        {
            return -1 ;
        }
        channel_index = cps_readBankData( state.bank , channel_index );
    }

    result = cps_readChannel( &channel , channel_index );

    // Read successful and channel is valid
    if( ( result != -1 ) && _ui_channel_valid( &channel ) )
    {
        // Set new channel index
        state.channel_index = selected_channel ;
        // Copy channel read to state
        state.channel       = channel ;
        *sync_rtx           = true;
    }

    return result
    ;
}

static void _ui_fsm_confirmVFOInput( bool*sync_rtx )
{
    vp_flush();

    switch( ui_state.input_set )
    {
        case SET_RX :
        {
            // Switch to TX input
            ui_state.input_set      = SET_TX;
            // Reset input position
            ui_state.input_position = 0;
            // announce the rx frequency just confirmed with Enter.
            vp_queueFrequency( ui_state.new_rx_frequency );
            // defer playing till the end.
            // indicate that the user has moved to the tx freq field.
            vp_announceInputReceiveOrTransmit( true , vpqDefault );
            break ;
        }
        case SET_TX :
        {
            // Save new frequency setting
            // If TX frequency was not set, TX = RX
            if( ui_state.new_tx_frequency == 0 )
            {
                ui_state.new_tx_frequency = ui_state.new_rx_frequency ;
            }
            // Apply new frequencies if they are valid
            if( _ui_freq_check_limits( ui_state.new_rx_frequency ) &&
                _ui_freq_check_limits(ui_state.new_tx_frequency  )    )
            {
                state.channel.rx_frequency = ui_state.new_rx_frequency ;
                state.channel.tx_frequency = ui_state.new_tx_frequency ;
                *sync_rtx                  = true ;
                // force init to clear any prompts in progress.
                // defer play because play is called at the end of the function
                //due to above freq queuing.
                vp_announceFrequencies( state.channel.rx_frequency ,
                                        state.channel.tx_frequency , vpqInit );
            }
            else
            {
                vp_announceError( vpqInit );
            }

            state.ui_screen = PAGE_MAIN_VFO ;
            break ;
        }
    }

    vp_play();
}

static void _ui_fsm_insertVFONumber( kbd_msg_t msg , bool* sync_rtx )
{
    // Advance input position
    ui_state.input_position += 1 ;
    // clear any prompts in progress.
    vp_flush() ;
    // Save pressed number to calculate frequency and show in GUI
    ui_state.input_number = input_getPressedNumber( msg );
    // queue the digit just pressed.
    vp_queueInteger( ui_state.input_number );
    // queue  point if user has entered three digits.
    if( ui_state.input_position == 3 )
    {
        vp_queuePrompt( PROMPT_POINT );
    }

    switch( ui_state.input_set )
    {
        case SET_RX :
        {
            if( ui_state.input_position == 1 )
            {
                ui_state.new_rx_frequency = 0 ;
            }
            // Calculate portion of the new RX frequency
            ui_state.new_rx_frequency = _ui_freq_add_digit( ui_state.new_rx_frequency ,
                                                            ui_state.input_position   ,
                                                            ui_state.input_number       );
            if( ui_state.input_position >= FREQ_DIGITS )
            {
                // queue the rx freq just completed.
                vp_queueFrequency( ui_state.new_rx_frequency );
                /// now queue tx as user has changed fields.
                vp_queuePrompt( PROMPT_TRANSMIT );
                // Switch to TX input
                ui_state.input_set        = SET_TX ;
                // Reset input position
                ui_state.input_position   = 0 ;
                // Reset TX frequency
                ui_state.new_tx_frequency = 0 ;
            }
            break ;
        }
        case SET_TX :
        {
            if( ui_state.input_position == 1 )
            {
                ui_state.new_tx_frequency = 0 ;
            }
            // Calculate portion of the new TX frequency
            ui_state.new_tx_frequency = _ui_freq_add_digit( ui_state.new_tx_frequency ,
                                                            ui_state.input_position   ,
                                                            ui_state.input_number       );
            if( ui_state.input_position >= FREQ_DIGITS )
            {
                // Save both inserted frequencies
                if( _ui_freq_check_limits( ui_state.new_rx_frequency ) &&
                    _ui_freq_check_limits( ui_state.new_tx_frequency )    )
                {
                    state.channel.rx_frequency = ui_state.new_rx_frequency ;
                    state.channel.tx_frequency = ui_state.new_tx_frequency ;
                    *sync_rtx                  = true;
                    // play is called at end.
                    vp_announceFrequencies( state.channel.rx_frequency ,
                                            state.channel.tx_frequency , vpqInit );
                }

                state.ui_screen = PAGE_MAIN_VFO ;
            }
            break ;
        }
    }

    vp_play();
}

#ifdef SCREEN_BRIGHTNESS
static void _ui_changeBrightness( int variation )
{
    state.settings.brightness += variation ;

    // Max value for brightness is 100, min value is set to 5 to avoid complete
    //  display shutdown.
    if( state.settings.brightness > 100 )
    {
        state.settings.brightness = 100 ;
    }
    else if( state.settings.brightness < 5 )
    {
        state.settings.brightness = 5 ;
    }

    display_setBacklightLevel( state.settings.brightness );
}
#endif // SCREEN_BRIGHTNESS

#ifdef SCREEN_CONTRAST
static void _ui_changeContrast( int variation )
{
    if( variation >= 0 )
    {
        state.settings.contrast =
        (255 - state.settings.contrast < variation) ? 255 : state.settings.contrast + variation ;
    }
    else
    {
        state.settings.contrast =
        (state.settings.contrast < -variation) ? 0 : state.settings.contrast + variation ;
    }

    display_setContrast( state.settings.contrast );
}
#endif // SCREEN_CONTRAST

static void _ui_changeTimer( int variation )
{
    if( ( state.settings.display_timer == TIMER_OFF && variation < 0 ) ||
        ( state.settings.display_timer == TIMER_1H  && variation > 0 )    )
    {
        return ;
    }

    state.settings.display_timer += variation ;
}

static inline void _ui_changeM17Can( int variation )
{
    uint8_t can = state.settings.m17_can ;

    state.settings.m17_can = ( can + variation ) % 16 ;

}

static void _ui_changeVoiceLevel( int variation )
{
    if( ( ( state.settings.vpLevel == vpNone ) && ( variation < 0 ) ) ||
        ( ( state.settings.vpLevel == vpHigh ) && ( variation > 0 ) )    )
    {
        return ;
    }

    state.settings.vpLevel += variation ;

    // Force these flags to ensure the changes are spoken for levels 1 through 3.
    vpQueueFlags_t flags = vpqInit                 |
                           vpqAddSeparatingSilence |
                           vpqPlayImmediately        ;

    if( !vp_isPlaying() )
    {
        flags |= vpqIncludeDescriptions ;
    }

    vp_announceSettingsVoiceLevel( flags );
}

static void _ui_changePhoneticSpell( bool newVal )
{
    state.settings.vpPhoneticSpell = newVal ? 1 : 0 ;

    vp_announceSettingsOnOffToggle( &currentLanguage->phonetic      ,
                                     vp_getVoiceLevelQueueFlags()   ,
                                     state.settings.vpPhoneticSpell   );
}

static bool _ui_checkStandby( long long time_since_last_event )
{
    bool result = false ;

    if( !standby )
    {
        switch( state.settings.display_timer )
        {
            case TIMER_OFF :
            {
                break ;
            }
            case TIMER_5S :
            case TIMER_10S :
            case TIMER_15S :
            case TIMER_20S :
            case TIMER_25S :
            case TIMER_30S :
            {
                result = time_since_last_event >= ( 5000 * state.settings.display_timer );
                break ;
            }
            case TIMER_1M :
            case TIMER_2M :
            case TIMER_3M :
            case TIMER_4M :
            case TIMER_5M :
            {
                result = time_since_last_event >=
                         ( 60000 * ( state.settings.display_timer - ( TIMER_1M - 1 ) ) );
                break ;
            }
            case TIMER_15M :
            case TIMER_30M :
            case TIMER_45M :
            {
                result = time_since_last_event >=
                         ( 60000 * 15 * ( state.settings.display_timer - ( TIMER_15M - 1 ) ) );
                break ;
            }
            case TIMER_1H :
            {
                result = time_since_last_event >= 60 * 60 * 1000 ;
                break ;
            }
        }
    }

    return result ;
}

static void _ui_enterStandby()
{
    if( !standby )
    {
        standby       = true ;
        redraw_needed = false ;
        display_setBacklightLevel( 0 );
    }
}

static bool _ui_exitStandby( long long now )
{
    bool result     = false ;

    last_event_tick = now ;

    if( standby )
    {
        standby       = false ;
        redraw_needed = true ;
        display_setBacklightLevel( state.settings.brightness );
        result        = true ;
    }

    return result ;

}

static void _ui_fsm_menuMacro( kbd_msg_t msg , bool* sync_rtx )
{
    // If there is no keyboard left and right select the menu entry to edit
#ifdef UI_NO_KEYBOARD

    switch( msg.keys )
    {
        case KNOB_LEFT :
        {
            ui_state.macro_menu_selected-- ;
            ui_state.macro_menu_selected += 9 ;
            ui_state.macro_menu_selected %= 9 ;
            break ;
        }
        case KNOB_RIGHT :
        {
            ui_state.macro_menu_selected++ ;
            ui_state.macro_menu_selected %= 9 ;
            break ;
        }
        case KEY_ENTER :
        {
            if( !msg.long_press )
            {
                ui_state.input_number = ui_state.macro_menu_selected + 1 ;
            }
            else
            {
                ui_state.input_number = 0 ;
            }
            break ;
        }
        default :
        {
            ui_state.input_number = 0 ;
            break ;
        }
    }
#else // UI_NO_KEYBOARD
    ui_state.input_number     = input_getPressedNumber( msg );
#endif // UI_NO_KEYBOARD
    // CTCSS Encode/Decode Selection
    bool tone_tx_enable       = state.channel.fm.txToneEn ;
    bool tone_rx_enable       = state.channel.fm.rxToneEn ;
    uint8_t tone_flags        = ( tone_tx_enable << 1 ) | tone_rx_enable ;
    vpQueueFlags_t queueFlags = vp_getVoiceLevelQueueFlags();

    switch( ui_state.input_number )
    {
        case KEY_NUM_1 :
        {
            if( state.channel.mode == OPMODE_FM )
            {
                if( state.channel.fm.txTone == 0 )
                {
                    state.channel.fm.txTone = MAX_TONE_INDEX - 1 ;
                }
                else
                {
                    state.channel.fm.txTone--;
                }

                state.channel.fm.txTone %= MAX_TONE_INDEX ;
                state.channel.fm.rxTone  = state.channel.fm.txTone ;
                *sync_rtx                = true ;
                vp_announceCTCSS( state.channel.fm.rxToneEn ,
                                  state.channel.fm.rxTone   ,
                                  state.channel.fm.txToneEn ,
                                  state.channel.fm.txTone   ,
                                  queueFlags                  );
            }
            break;
        }
        case KEY_NUM_2 :
        {
            if( state.channel.mode == OPMODE_FM )
            {
                state.channel.fm.txTone++ ;
                state.channel.fm.txTone %= MAX_TONE_INDEX ;
                state.channel.fm.rxTone  = state.channel.fm.txTone ;
                *sync_rtx                = true ;
                vp_announceCTCSS( state.channel.fm.rxToneEn ,
                                  state.channel.fm.rxTone   ,
                                  state.channel.fm.txToneEn ,
                                  state.channel.fm.txTone   ,
                                  queueFlags                  );
            }
            break ;
        }
        case KEY_NUM_3 :
        {
            if( state.channel.mode == OPMODE_FM )
            {
                tone_flags++;
                tone_flags                %= 4 ;
                tone_tx_enable             = tone_flags >> 1 ;
                tone_rx_enable             = tone_flags & 1 ;
                state.channel.fm.txToneEn  = tone_tx_enable ;
                state.channel.fm.rxToneEn  = tone_rx_enable ;
                *sync_rtx                  = true ;
                vp_announceCTCSS( state.channel.fm.rxToneEn ,
                                  state.channel.fm.rxTone   ,
                                  state.channel.fm.txToneEn ,
                                  state.channel.fm.txTone   ,
                                  queueFlags |vpqIncludeDescriptions );
            }
            break ;
        }
        case KEY_NUM_4 :
        {
            if( state.channel.mode == OPMODE_FM )
            {
                state.channel.bandwidth++ ;
                state.channel.bandwidth %= 3 ;
                *sync_rtx                = true ;
                vp_announceBandwidth( state.channel.bandwidth , queueFlags );
            }
            break ;
        }
        case KEY_NUM_5 :
        {
            // Cycle through radio modes
            if( state.channel.mode == OPMODE_FM )
            {
                state.channel.mode = OPMODE_M17 ;
            }
            else if( state.channel.mode == OPMODE_M17 )
            {
                state.channel.mode = OPMODE_FM;
            }
            else
            {
                //catch any invalid states so they don't get locked out
                state.channel.mode = OPMODE_FM ;
            }
            *sync_rtx = true ;
            vp_announceRadioMode( state.channel.mode, queueFlags );
            break ;
        }
        case KEY_NUM_6 :
        {
            float power ;
            if( state.channel.power == 100 )
            {
                state.channel.power = 135 ;
            }
            else
            {
                state.channel.power = 100 ;
            }
            *sync_rtx = true ;
            power     = dBmToWatt( state.channel.power );
            vp_anouncePower( power , queueFlags );
            break ;
        }
#ifdef SCREEN_BRIGHTNESS
        case KEY_NUM_7 :
        {
            _ui_changeBrightness( -5 );
            vp_announceSettingsInt( &currentLanguage->brightness , queueFlags ,
                                     state.settings.brightness                  );
            break ;
        }
        case KEY_NUM_8 :
        {
            _ui_changeBrightness( +5 );
            vp_announceSettingsInt( &currentLanguage->brightness , queueFlags ,
                                     state.settings.brightness                  );
            break ;
        }
#endif // SCREEN_BRIGHTNESS
        case KEY_NUM_9 :
        {
            if( !ui_state.input_locked )
            {
                ui_state.input_locked = true ;
            }
            else
            {
                ui_state.input_locked = false ;
            }
            break ;
        }
        default :
        {
            break ;
        }
    }

#ifdef PLATFORM_TTWRPLUS
    if( msg.keys & KEY_VOLDOWN )
#else // PLATFORM_TTWRPLUS
    if( msg.keys & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
#endif // PLATFORM_TTWRPLUS
    {
#ifdef HAS_ABSOLUTE_KNOB // If the radio has an absolute position knob
        state.settings.sqlLevel = platform_getChSelector() - 1 ;
#endif // HAS_ABSOLUTE_KNOB
        if(state.settings.sqlLevel > 0)
        {
            state.settings.sqlLevel -= 1 ;
            *sync_rtx                = true ;
            vp_announceSquelch( state.settings.sqlLevel , queueFlags );
        }
    }

#ifdef PLATFORM_TTWRPLUS
    else if( msg.keys & KEY_VOLUP )
#else // PLATFORM_TTWRPLUS
    else if( msg.keys & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
#endif // PLATFORM_TTWRPLUS
    {
#ifdef HAS_ABSOLUTE_KNOB
        state.settings.sqlLevel = platform_getChSelector() - 1 ;
#endif // HAS_ABSOLUTE_KNOB
        if(state.settings.sqlLevel < 15)
        {
            state.settings.sqlLevel += 1 ;
            *sync_rtx                = true;
            vp_announceSquelch( state.settings.sqlLevel , queueFlags );
        }
    }
}

static void _ui_menuUp( uint8_t menu_entries )
{
    if( ui_state.menu_selected > 0 )
    {
        ui_state.menu_selected -= 1 ;
    }
    else
    {
        ui_state.menu_selected = menu_entries - 1 ;
    }
    vp_playMenuBeepIfNeeded( ui_state.menu_selected == 0 );
}

static void _ui_menuDown( uint8_t menu_entries )
{
    if( ui_state.menu_selected < ( menu_entries - 1 ) )
    {
        ui_state.menu_selected += 1 ;
    }
    else
    {
        ui_state.menu_selected = 0 ;
    }
    vp_playMenuBeepIfNeeded( ui_state.menu_selected == 0);
}

static void _ui_menuBack( uint8_t prev_state )
{
    if( ui_state.edit_mode )
    {
        ui_state.edit_mode = false ;
    }
    else
    {
        // Return to previous menu
        state.ui_screen        = prev_state ;
        // Reset menu selection
        ui_state.menu_selected = 0 ;
        vp_playMenuBeepIfNeeded( true );
    }
}

static void _ui_textInputReset( char* buf )
{
    ui_state.input_number   = 0 ;
    ui_state.input_position = 0 ;
    ui_state.input_set      = 0 ;
    ui_state.last_keypress  = 0 ;
    memset( buf , 0 , 9 );
    buf[ 0 ]                = '_';
}

static void _ui_textInputKeypad( char* buf , uint8_t max_len , kbd_msg_t msg , bool callsign )
{
    if( ui_state.input_position < max_len )
    {
        long long now         = getTick();
        // Get currently pressed number key
        uint8_t   num_key     = input_getPressedNumber( msg );
        // Get number of symbols related to currently pressed key
        uint8_t   num_symbols = 0 ;

        if( callsign )
        {
            num_symbols = strlen( symbols_ITU_T_E161_callsign[ num_key ] );
        }
        else
        {
            num_symbols = strlen( symbols_ITU_T_E161[ num_key ] );
        }

        // Skip keypad logic for first keypress
        if( ui_state.last_keypress != 0 )
        {
            // Same key pressed and timeout not expired: cycle over chars of current key
            if( ( ui_state.input_number == num_key                          ) &&
                ( ( now - ui_state.last_keypress ) < input_longPressTimeout )    )
            {
                ui_state.input_set = ( ui_state.input_set + 1 ) % num_symbols ;
            }
            // Different key pressed: save current char and change key
            else
            {
                ui_state.input_position += 1 ;
                ui_state.input_set       = 0 ;
            }
        }
        // Show current character on buffer
        if( callsign )
        {
            buf[ ui_state.input_position ] = symbols_ITU_T_E161_callsign[ num_key ][ ui_state.input_set ];
        }
        else
        {
            buf[ ui_state.input_position ] = symbols_ITU_T_E161[ num_key ][ ui_state.input_set ];
        }
        // Announce the character
        vp_announceInputChar( buf[ ui_state.input_position ] );
        // Update reference values
        ui_state.input_number  = num_key ;
        ui_state.last_keypress = now ;
    }
}

static void _ui_textInputConfirm( char* buf )
{
    buf[ ui_state.input_position + 1 ] = '\0' ;
}

static void _ui_textInputDel( char* buf )
{
    // announce the char about to be backspaced.
    // Note this assumes editing callsign.
    // If we edit a different buffer which allows the underline char, we may
    // not want to exclude it, but when editing callsign, we do not want to say
    // underline since it means the field is empty.
    if( buf[ ui_state.input_position ] &&
        buf[ ui_state.input_position ] != '_' )
    {
        vp_announceInputChar( buf[ ui_state.input_position ] );
    }

    buf[ ui_state.input_position ] = '\0' ;
    // Move back input cursor
    if( ui_state.input_position > 0 )
    {
        ui_state.input_position--;
    // If we deleted the initial character, reset starting condition
    }
    else
    {
        ui_state.last_keypress = 0 ;
    }
    ui_state.input_set = 0 ;
}

static void _ui_numberInputKeypad( uint32_t* num , kbd_msg_t msg )
{
    long long now = getTick();

#ifdef UI_NO_KEYBOARD
    // If knob is turned, increment or Decrement
    if( guiState->msg.keys & KNOB_LEFT )
    {
        *num = *num + 1 ;
        if( *num % 10 == 0 )
        {
            *num = *num - 10 ;
        }
    }

    if( guiState->msg.keys & KNOB_RIGHT )
    {
        if( *num == 0 )
        {
            *num = 9 ;
        }
        else
        {
            *num = *num - 1 ;
            if( ( *num % 10 ) == 9 )
            {
                *num = *num + 10;
            }
        }
    }

    // If enter is pressed, advance to the next digit
    if( guiState->msg.keys & KEY_ENTER )
    {
        *num *= 10 ;
    }
    // Announce the character
    vp_announceInputChar( '0' + ( *num % 10 ) );

    // Update reference values
    ui_state.input_number = *num % 10 ;
#else // UI_NO_KEYBOARD
    // Maximum frequency len is uint32_t max value number of decimal digits
    if( ui_state.input_position >= 10 )
    {
        return;
    }
    // Get currently pressed number key
    uint8_t num_key = input_getPressedNumber( msg );
    *num *= 10 ;
    *num += num_key ;

    // Announce the character
    vp_announceInputChar( '0' + num_key );

    // Update reference values
    ui_state.input_number  = num_key;
#endif // UI_NO_KEYBOARD

    ui_state.last_keypress = now;
}

static void _ui_numberInputDel( uint32_t* num )
{
    // announce the digit about to be backspaced.
    vp_announceInputChar( '0' + ( *num % 10 ) );

    // Move back input cursor
    if( ui_state.input_position > 0 )
    {
        ui_state.input_position-- ;
    }
    else
    {
        ui_state.last_keypress = 0 ;
    }
    ui_state.input_set = 0 ;
}

void ui_init( void )
{
    last_event_tick = getTick();
    redraw_needed   = true ;
    // Initialize struct ui_state to all zeroes
    // This syntax is called compound literal
    // https://stackoverflow.com/questions/6891720/initialize-reset-struct-to-zero-null
    ui_state = (const ui_state_st){ 0 };
}

void ui_drawSplashScreen( void )
{
    gfx_clearScreen();

    #if SCREEN_HEIGHT > 64
    static const point_t    logo_orig = { 0 , ( SCREEN_HEIGHT / 2 ) - 6 } ;
    static const point_t    call_orig = { 0 , SCREEN_HEIGHT - 8 } ;
    static const fontSize_t logo_font = FONT_SIZE_12PT ;
    static const fontSize_t call_font = FONT_SIZE_8PT ;
    #else
    static const point_t    logo_orig = { 0 , 19 } ;
    static const point_t    call_orig = { 0 , SCREEN_HEIGHT - 8 } ;
    static const fontSize_t logo_font = FONT_SIZE_8PT ;
    static const fontSize_t call_font = FONT_SIZE_6PT ;
    #endif

    gfx_print( logo_orig , logo_font , TEXT_ALIGN_CENTER,  yellow_fab413 , "O P N\nR T X" );
    gfx_print( call_orig , call_font , TEXT_ALIGN_CENTER , color_white   , state.settings.callsign );

    vp_announceSplashScreen();
}

void ui_saveState( void )
{
    last_state = state ;
}

#ifdef GPS_PRESENT
static float    priorGPSSpeed         =   0 ;
static float    priorGPSAltitude      =   0 ;
static float    priorGPSDirection     = 500 ; // impossible value init.
static uint8_t  priorGPSFixQuality    =   0 ;
static uint8_t  priorGPSFixType       =   0 ;
static uint8_t  priorSatellitesInView =   0 ;
static uint32_t vpGPSLastUpdate       =   0 ;

static vpGPSInfoFlags_t GetGPSDirectionOrSpeedChanged( void )
{
    uint32_t         now ;
    vpGPSInfoFlags_t whatChanged ;
    float            speedDiff ;
    float            altitudeDiff ;
    float            degreeDiff ;

    if( !state.settings.gps_enabled )
    {
        return vpGPSNone;
    }

    now = getTick();

    if( ( now - vpGPSLastUpdate ) < 8000 )
    {
        return vpGPSNone;
    }

    whatChanged = vpGPSNone ;

    if( state.gps_data.fix_quality != priorGPSFixQuality )
    {
        whatChanged        |= vpGPSFixQuality ;
        priorGPSFixQuality  = state.gps_data.fix_quality ;
    }

    if( state.gps_data.fix_type != priorGPSFixType )
    {
        whatChanged     |= vpGPSFixType ;
        priorGPSFixType  = state.gps_data.fix_type ;
    }

    speedDiff=fabs( state.gps_data.speed - priorGPSSpeed );
    if( speedDiff >= 1 )
    {
        whatChanged   |= vpGPSSpeed ;
        priorGPSSpeed  = state.gps_data.speed ;
    }

    altitudeDiff = fabs( state.gps_data.altitude - priorGPSAltitude );

    if( altitudeDiff >= 5 )
    {
        whatChanged      |= vpGPSAltitude ;
        priorGPSAltitude  = state.gps_data.altitude ;
    }

    degreeDiff = fabs( state.gps_data.tmg_true - priorGPSDirection );

    if( degreeDiff  >= 1 )
    {
        whatChanged       |= vpGPSDirection ;
        priorGPSDirection  = state.gps_data.tmg_true ;
    }

    if( state.gps_data.satellites_in_view != priorSatellitesInView )
    {
        whatChanged           |= vpGPSSatCount ;
        priorSatellitesInView  = state.gps_data.satellites_in_view ;
    }

    if( whatChanged )
    {
        vpGPSLastUpdate = now ;
    }

    return whatChanged ;
}
#endif // GPS_PRESENT
//@@@KL this fn is far too long - I will need to sort it out - task for next stage
//@@@Kl !!! check coding of switch fns against original coding
void ui_updateFSM( bool* sync_rtx )
{
    GuiState_st guiState ;
    uint8_t     newTail ;
    bool        processEvent = false ;

    // Check for events
    if( evQueue_wrPos != evQueue_rdPos )
    {
        // Pop an event from the queue
        event_t event ;
        newTail       = ( evQueue_rdPos + 1 ) % MAX_NUM_EVENTS ;
        event         = evQueue[ evQueue_rdPos ] ;
        evQueue_rdPos = newTail ;
        processEvent  = true ;

        // There is some event to process, we need an UI redraw.
        // UI redraw request is cancelled if we're in standby mode.
        redraw_needed = true ;
        if( standby )
        {
            redraw_needed = false ;
        }

        // Check if battery has enough charge to operate.
        // Check is skipped if there is an ongoing transmission, since the voltage
        // drop caused by the RF PA power absorption causes spurious triggers of
        // the low battery alert.
        bool txOngoing = platform_getPttStatus();
#if !defined(PLATFORM_TTWRPLUS)
        if( !state.emergency && !txOngoing && ( state.charge <= 0 ) )
        {
            state.ui_screen = PAGE_LOW_BAT ;
            if( ( event.type == EVENT_KBD ) && event.payload )
            {
                state.ui_screen = PAGE_MAIN_VFO ;
                state.emergency = true ;
            }
            processEvent = false ;
        }
#endif // PLATFORM_TTWRPLUS

        if( processEvent )
        {
            long long timeTick = timeTick = getTick();
            switch( event.type )
            {
                // Process pressed keys
                case EVENT_KBD :
                {
                    guiState.msg.value  = event.payload ;
                    guiState.f1Handled  = false ;
                    guiState.queueFlags = vp_getVoiceLevelQueueFlags();
                    // If we get out of standby, we ignore the kdb event
                    // unless is the MONI key for the MACRO functions
                    if( _ui_exitStandby( timeTick ) && !( guiState.msg.keys & KEY_MONI ) )
                    {
                        processEvent = false ;
                    }

                    if( processEvent )
                    {
                        // If MONI is pressed, activate MACRO functions
                        bool moniPressed ;
                        moniPressed = guiState.msg.keys & KEY_MONI ;
                        if( moniPressed || macro_latched )
                        {
                            macro_menu = true ;
                            // long press moni on its own latches function.
                            if( moniPressed && guiState.msg.long_press && !macro_latched )
                            {
                                macro_latched = true ;
                                vp_beep( BEEP_FUNCTION_LATCH_ON , LONG_BEEP );
                            }
                            else if( moniPressed && macro_latched )
                            {
                                macro_latched = false ;
                                vp_beep( BEEP_FUNCTION_LATCH_OFF , LONG_BEEP );
                            }
                            _ui_fsm_menuMacro( guiState.msg , sync_rtx );
                            processEvent = false ;
                        }
                        else
                        {
                            macro_menu = false ;
                        }
                    }

                    if( processEvent )
                    {
#if defined(PLATFORM_TTWRPLUS)
                        // T-TWR Plus has no KEY_MONI, using KEY_VOLDOWN long press instead
                        if( ( guiState.msg.keys & KEY_VOLDOWN ) && guiState.msg.long_press )
                        {
                            macro_menu    = true ;
                            macro_latched = true ;
                        }
#endif // PLA%FORM_TTWRPLUS
                        if( state.tone_enabled && !( guiState.msg.keys & KEY_HASH ) )
                        {
                            state.tone_enabled = false ;
                            *sync_rtx          = true ;
                        }

                        int priorUIScreen ;
                        priorUIScreen = state.ui_screen ;

                        if( state.ui_screen < PAGE_NUM_OF )
                        {
                            *sync_rtx = ui_updateFSM_PAGE_T[ state.ui_screen ]( &guiState , &ui_state );
                        }

                        // Enable Tx only if in PAGE_MAIN_VFO or PAGE_MAIN_MEM states
                        bool inMemOrVfo ;
                        inMemOrVfo = ( state.ui_screen == PAGE_MAIN_VFO ) || ( state.ui_screen == PAGE_MAIN_MEM );
                        if( (   macro_menu == true                                    ) ||
                            ( ( inMemOrVfo == false ) && ( state.txDisable == false ) )    )
                        {
                            state.txDisable = true;
                            *sync_rtx       = true;
                        }
                        if( !guiState.f1Handled                  &&
                             ( guiState.msg.keys & KEY_F1      ) &&
                             ( state.settings.vpLevel > vpBeep )    )
                        {
                            vp_replayLastPrompt();
                        }
                        else if( ( priorUIScreen != state.ui_screen ) &&
                                 ( state.settings.vpLevel > vpLow   )    )
                        {
                            // When we switch to VFO or Channel screen, we need to announce it.
                            // Likewise for information screens.
                            // All other cases are handled as needed.
                            vp_announceScreen( state.ui_screen );
                        }
                        // generic beep for any keydown if beep is enabled.
                        // At vp levels higher than beep, keys will generate voice so no need
                        // to beep or you'll get an unwanted click.
                        if( ( guiState.msg.keys & 0xFFFF ) && ( state.settings.vpLevel == vpBeep ) )
                        {
                            vp_beep( BEEP_KEY_GENERIC , SHORT_BEEP );
                        }
                        // If we exit and re-enter the same menu, we want to ensure it speaks.
                        if( guiState.msg.keys & KEY_ESC )
                        {
                            _ui_reset_menu_anouncement_tracking();
                        }
                    }
                    break ;
                }// case EVENT_KBD :
                case EVENT_STATUS :
                {
#ifdef GPS_PRESENT
                    if( ( state.ui_screen == PAGE_MENU_GPS ) &&
                        !vp_isPlaying()                      &&
                        ( state.settings.vpLevel > vpLow   ) &&
                        !txOngoing                           &&
                        !rtx_rxSquelchOpen()                    )
                    {
                        // automatically read speed and direction changes only!
                        vpGPSInfoFlags_t whatChanged = GetGPSDirectionOrSpeedChanged();
                        if( whatChanged != vpGPSNone )
                        {
                            vp_announceGPSInfo( whatChanged );
                        }
                    }
#endif //            GPS_PRESENT

                    if( txOngoing || rtx_rxSquelchOpen() )
                    {
                        if( txOngoing )
                        {
                            macro_latched = 0 ;
                        }
                        _ui_exitStandby( timeTick );
                        processEvent = false ;
                    }

                    if( processEvent )
                    {
                        if( _ui_checkStandby( timeTick - last_event_tick ) )
                        {
                            _ui_enterStandby();
                        }
                    }
                    break ;
                }// case EVENT_STATUS :
            }// switch( event.type )
        }
    }
}

static bool ui_updateFSM_PAGE_MAIN_VFO( GuiState_st* guiState , ui_state_st* uiState )
{
    bool sync_rtx = false ;

    // Enable Tx in PAGE_MAIN_VFO mode
    if( state.txDisable )
    {
        state.txDisable = false ;
        sync_rtx        = true ;
    }
    // M17 Destination callsign input
    if( !uiState->input_locked )
    {
        if( uiState->edit_mode )
        {
            if( state.channel.mode == OPMODE_M17 )
            {
                if( guiState->msg.keys & KEY_ENTER )
                {
                    _ui_textInputConfirm( uiState->new_callsign );
                    // Save selected dst ID and disable input mode
                    strncpy( state.settings.m17_dest , uiState->new_callsign , 10 );
                    uiState->edit_mode = false ;
                    sync_rtx           = true ;
                    vp_announceM17Info( NULL ,  uiState->edit_mode , guiState->queueFlags );
                }
                else if( guiState->msg.keys & KEY_HASH )
                {
                    // Save selected dst ID and disable input mode
                    strncpy( state.settings.m17_dest , "" , 1 );
                    uiState->edit_mode = false ;
                    sync_rtx           = true ;
                    vp_announceM17Info( NULL , uiState->edit_mode , guiState->queueFlags );
                }
                else if( guiState->msg.keys & KEY_ESC )
                {
                    // Discard selected dst ID and disable input mode
                    uiState->edit_mode = false ;
                }
                else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
                {
                    _ui_textInputDel( uiState->new_callsign );
                }
                else if( input_isNumberPressed( guiState->msg ) )
                {
                    _ui_textInputKeypad( uiState->new_callsign , 9 , guiState->msg , true );
                }
            }
        }
        else
        {
            if( guiState->msg.keys & KEY_ENTER )
            {
                // Save current main state
                uiState->last_main_state = state.ui_screen ;
                // Open Menu
                state.ui_screen          = PAGE_MENU_TOP ;
                // The selected item will be announced when the item is first selected.
            }
            else if( guiState->msg.keys & KEY_ESC )
            {
                // Save VFO channel
                state.vfo_channel = state.channel ;
                int result  = _ui_fsm_loadChannel( state.channel_index , &sync_rtx );
                // Read successful and channel is valid
                if(result != -1)
                {
                    // Switch to MEM screen
                    state.ui_screen = PAGE_MAIN_MEM;
                    // anounce the active channel name.
                    vp_announceChannelName( &state.channel , state.channel_index , guiState->queueFlags );
                }
            }
            else if( guiState->msg.keys & KEY_HASH )
            {
                // Only enter edit mode when using M17
                if( state.channel.mode == OPMODE_M17 )
                {
                    // Enable dst ID input
                    uiState->edit_mode = true ;
                    // Reset text input variables
                    _ui_textInputReset( uiState->new_callsign );
                    vp_announceM17Info( NULL , uiState->edit_mode , guiState->queueFlags );
                }
                else
                {
                    if(!state.tone_enabled)
                    {
                        state.tone_enabled = true ;
                        sync_rtx           = true ;
                    }
                }
            }
            else if( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) )
            {
                // Increment TX and RX frequency of 12.5KHz
                if( _ui_freq_check_limits( state.channel.rx_frequency + freq_steps[ state.step_index ] ) &&
                    _ui_freq_check_limits( state.channel.tx_frequency + freq_steps[ state.step_index ] )    )
                {
                    state.channel.rx_frequency += freq_steps[ state.step_index ];
                    state.channel.tx_frequency += freq_steps[ state.step_index ];
                    sync_rtx                    = true;
                    vp_announceFrequencies( state.channel.rx_frequency , state.channel.tx_frequency , guiState->queueFlags );
                }
            }
            else if( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) )
            {
                // Decrement TX and RX frequency of 12.5KHz
                if( _ui_freq_check_limits( state.channel.rx_frequency - freq_steps[ state.step_index ] ) &&
                    _ui_freq_check_limits( state.channel.tx_frequency - freq_steps[ state.step_index ] )    )
                {
                    state.channel.rx_frequency -= freq_steps[ state.step_index ];
                    state.channel.tx_frequency -= freq_steps[ state.step_index ];
                    sync_rtx                    = true ;
                    vp_announceFrequencies( state.channel.rx_frequency , state.channel.tx_frequency , guiState->queueFlags );
                }
            }
            else if( guiState->msg.keys & KEY_F1 )
            {
                if( state.settings.vpLevel > vpBeep )
                {// quick press repeat vp, long press summary.
                    if( guiState->msg.long_press )
                    {
                        vp_announceChannelSummary( &state.channel , 0 , state.bank , vpAllInfo );
                    }
                    else
                    {
                        vp_replayLastPrompt();
                    }
                    guiState->f1Handled = true;
                }
            }
            else if( input_isNumberPressed( guiState->msg ) )
            {
                // Open Frequency input screen
                state.ui_screen           = PAGE_MAIN_VFO_INPUT ;
                // Reset input position and selection
                uiState->input_position   = 1 ;
                uiState->input_set        = SET_RX ;
                // do not play  because we will also announce the number just entered.
                vp_announceInputReceiveOrTransmit( false , vpqInit );
                vp_queueInteger(input_getPressedNumber( guiState->msg ) );
                vp_play();

                uiState->new_rx_frequency = 0 ;
                uiState->new_tx_frequency = 0 ;
                // Save pressed number to calculare frequency and show in GUI
                uiState->input_number     = input_getPressedNumber( guiState->msg );
                // Calculate portion of the new frequency
                uiState->new_rx_frequency = _ui_freq_add_digit( uiState->new_rx_frequency , uiState->input_position , uiState->input_number );
            }
        }
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MAIN_VFO_INPUT( GuiState_st* guiState , ui_state_st* uiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ENTER )
    {
        _ui_fsm_confirmVFOInput( &sync_rtx );
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        // Cancel frequency input, return to VFO mode
        state.ui_screen = PAGE_MAIN_VFO ;
    }
    else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN ) )
    {
        //@@@KL why is RX \ TX being toggled?
        switch( uiState->input_set )
        {
            case SET_RX :
            {
                uiState->input_set = SET_TX ;
                vp_announceInputReceiveOrTransmit( true , guiState->queueFlags );
                break ;
            }
            case SET_TX :
            {
                uiState->input_set = SET_RX ;
                vp_announceInputReceiveOrTransmit( false , guiState->queueFlags );
                break ;
            }
        }
        // Reset input position
        uiState->input_position = 0 ;
    }
    else if( input_isNumberPressed( guiState->msg ) )
    {
        _ui_fsm_insertVFONumber( guiState->msg , &sync_rtx );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MAIN_MEM( GuiState_st* guiState , ui_state_st* uiState )
{
    bool sync_rtx = false ;

    // Enable Tx in PAGE_MAIN_MEM mode
    if( state.txDisable )
    {
        state.txDisable = false ;
        sync_rtx        = true ;
    }
    if( !uiState->input_locked )
    {
        // M17 Destination callsign input
        if( uiState->edit_mode )
        {
            if( guiState->msg.keys & KEY_ENTER )
            {
                _ui_textInputConfirm( uiState->new_callsign );
                // Save selected dst ID and disable input mode
                strncpy( state.settings.m17_dest , uiState->new_callsign , 10 );
                uiState->edit_mode = false ;
                sync_rtx           = true ;
            }
            else if( guiState->msg.keys & KEY_HASH )
            {
                // Save selected dst ID and disable input mode
                strncpy( state.settings.m17_dest , "" , 1 );
                uiState->edit_mode = false;
                sync_rtx           = true;
            }
            else if( guiState->msg.keys & KEY_ESC )
            {
                // Discard selected dst ID and disable input mode
                uiState->edit_mode = false ;
            }
            else if( guiState->msg.keys & KEY_F1 )
            {
                if( state.settings.vpLevel > vpBeep )
                {
                    // Quick press repeat vp, long press summary.
                    if( guiState->msg.long_press )
                    {
                        vp_announceChannelSummary( &state.channel , state.channel_index , state.bank , vpAllInfo );
                    }
                    else
                    {
                        vp_replayLastPrompt();
                    }

                    guiState->f1Handled = true ;
                }
            }
            else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
            {
                _ui_textInputDel( uiState->new_callsign );
            }
            else if( input_isNumberPressed( guiState->msg ) )
            {
                _ui_textInputKeypad( uiState->new_callsign , 9 , guiState->msg , true );
            }
        }
        else
        {
            if( guiState->msg.keys & KEY_ENTER )
            {
                // Save current main state
                uiState->last_main_state = state.ui_screen ;
                // Open Menu
                state.ui_screen          = PAGE_MENU_TOP ;
            }
            else if( guiState->msg.keys & KEY_ESC )
            {
                // Restore VFO channel
                state.channel   = state.vfo_channel ;
                // Update RTX configuration
                sync_rtx        = true ;
                // Switch to VFO screen
                state.ui_screen = PAGE_MAIN_VFO ;
            }
            else if( guiState->msg.keys & KEY_HASH )
            {
                // Only enter edit mode when using M17
                if( state.channel.mode == OPMODE_M17 )
                {
                    // Enable dst ID input
                    uiState->edit_mode = true ;
                    // Reset text input variables
                    _ui_textInputReset( uiState->new_callsign );
                }
                else
                {
                    if( !state.tone_enabled )
                    {
                        state.tone_enabled = true ;
                        sync_rtx           = true ;
                    }
                }
            }
            else if( guiState->msg.keys & KEY_F1 )
            {
                if( state.settings.vpLevel > vpBeep )
                {
                    // quick press repeat vp, long press summary.
                    if( guiState->msg.long_press )
                    {
                        vp_announceChannelSummary( &state.channel , state.channel_index+1 , state.bank , vpAllInfo );
                    }
                    else
                    {
                        vp_replayLastPrompt();
                    }

                    guiState->f1Handled = true;
                }
            }
            else if( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) )
            {
                _ui_fsm_loadChannel( state.channel_index + 1 , &sync_rtx );
                vp_announceChannelName( &state.channel , state.channel_index + 1 , guiState->queueFlags );
            }
            else if( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) )
            {
                _ui_fsm_loadChannel( state.channel_index - 1 , &sync_rtx );
                vp_announceChannelName( &state.channel , state.channel_index + 1 , guiState->queueFlags );
            }
        }
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MODE_VFO( GuiState_st* guiState , ui_state_st* uiState )
{
    return ui_updateFSM_PAGE_MENU_TOP( guiState , uiState );
}

static bool ui_updateFSM_PAGE_MODE_MEM( GuiState_st* guiState , ui_state_st* uiState )
{
    return ui_updateFSM_PAGE_MENU_TOP( guiState , uiState );
}

static bool ui_updateFSM_PAGE_MENU_TOP( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( uiGetPageNumOf( PAGE_MENU_TOP ) );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        _ui_menuDown( uiGetPageNumOf( PAGE_MENU_TOP ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        switch( uiState->menu_selected )
        {
            case M_BANK :
            {
                state.ui_screen = PAGE_MENU_BANK ;
                break ;
            }
            case M_CHANNEL :
            {
                state.ui_screen = PAGE_MENU_CHANNEL ;
                break ;
            }
            case M_CONTACTS :
            {
                state.ui_screen = PAGE_MENU_CONTACTS ;
                break ;
            }
#ifdef GPS_PRESENT
            case M_GPS :
            {
                state.ui_screen = PAGE_MENU_GPS ;
                break ;
            }
#endif // GPS_PRESENT
            case M_SETTINGS :
            {
                state.ui_screen = PAGE_MENU_SETTINGS ;
                break ;
            }
            case M_INFO :
            {
                state.ui_screen = PAGE_MENU_INFO ;
                break ;
            }
            case M_ABOUT :
            {
                state.ui_screen = PAGE_MENU_ABOUT ;
                break ;
            }
        }
        // Reset menu selection
        uiState->menu_selected = 0 ;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( uiState->last_main_state );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_BANK( GuiState_st* guiState , ui_state_st* uiState )
{
    return ui_updateFSM_PAGE_MENU_CONTACTS( guiState , uiState );
}

static bool ui_updateFSM_PAGE_MENU_CHANNEL( GuiState_st* guiState , ui_state_st* uiState )
{
    return ui_updateFSM_PAGE_MENU_CONTACTS( guiState , uiState );
}

static bool ui_updateFSM_PAGE_MENU_CONTACTS( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        // Using 1 as parameter disables menu wrap around
        _ui_menuUp( 1 );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        if( state.ui_screen == PAGE_MENU_BANK )
        {
            bankHdr_t bank;
            // manu_selected is 0-based
            // bank 0 means "All Channel" mode
            // banks (1, n) are mapped to banks (0, n-1)
            if( cps_readBankHeader( &bank , uiState->menu_selected ) != -1 )
            {
                uiState->menu_selected += 1 ;
            }
        }
        else if( state.ui_screen == PAGE_MENU_CHANNEL )
        {
            channel_t channel ;
            if( cps_readChannel( &channel , uiState->menu_selected + 1 ) != -1 )
            {
                uiState->menu_selected += 1 ;
            }
        }
        else if( state.ui_screen == PAGE_MENU_CONTACTS )
        {
            contact_t contact ;
            if( cps_readContact( &contact , uiState->menu_selected + 1 ) != -1 )
            {
                uiState->menu_selected += 1 ;
            }
        }
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        if( state.ui_screen == PAGE_MENU_BANK )
        {
            bankHdr_t newbank ;
            int       result  = 0 ;
            // If "All channels" is selected, load default bank
            if( uiState->menu_selected == 0 )
            {
                state.bank_enabled = false ;
            }
            else
            {
                state.bank_enabled = true;
                result = cps_readBankHeader( &newbank , uiState->menu_selected - 1 );
            }
            if( result != -1 )
            {
                state.bank = uiState->menu_selected - 1 ;
                // If we were in VFO mode, save VFO channel
                if( uiState->last_main_state == PAGE_MAIN_VFO )
                {
                    state.vfo_channel = state.channel ;
                }
                // Load bank first channel
                _ui_fsm_loadChannel( 0 , &sync_rtx );
                // Switch to MEM screen
                state.ui_screen = PAGE_MAIN_MEM ;
            }
        }
        if( state.ui_screen == PAGE_MENU_CHANNEL )
        {
            // If we were in VFO mode, save VFO channel
            if( uiState->last_main_state == PAGE_MAIN_VFO )
            {
                state.vfo_channel = state.channel;
            }
            _ui_fsm_loadChannel( uiState->menu_selected , &sync_rtx );
            // Switch to MEM screen
            state.ui_screen = PAGE_MAIN_MEM ;
        }
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_TOP );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_GPS( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;
    (void)uiState;

    bool sync_rtx = false ;

    if( ( guiState->msg.keys & KEY_F1 ) && ( state.settings.vpLevel > vpBeep ) )
    {
        // quick press repeat vp, long press summary.
        if( guiState->msg.long_press )
        {
            vp_announceGPSInfo( vpGPSAll );
        }
        else
        {
            vp_replayLastPrompt();
        }
        guiState->f1Handled = true;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_TOP );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_SETTINGS( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( uiGetPageNumOf( PAGE_MENU_SETTINGS ) );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        _ui_menuDown( uiGetPageNumOf( PAGE_MENU_SETTINGS ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        switch( uiState->menu_selected )
        {
            case S_DISPLAY :
            {
                state.ui_screen = PAGE_SETTINGS_DISPLAY ;
                break ;
            }
#ifdef RTC_PRESENT
            case S_TIMEDATE :
            {
                state.ui_screen = PAGE_SETTINGS_TIMEDATE ;
                break ;
            }
#endif
#ifdef GPS_PRESENT
            case S_GPS :
            {
                state.ui_screen = PAGE_SETTINGS_GPS ;
                break ;
            }
#endif
            case S_RADIO :
            {
                state.ui_screen = PAGE_SETTINGS_RADIO ;
                break ;
            }
            case S_M17 :
            {
                state.ui_screen = PAGE_SETTINGS_M17 ;
                break ;
            }
            case S_VOICE :
            {
                state.ui_screen = PAGE_SETTINGS_VOICE ;
                break ;
            }
            case S_RESET2DEFAULTS :
            {
                state.ui_screen = PAGE_SETTINGS_RESET_TO_DEFAULTS ;
                break ;
            }
            default :
            {
                state.ui_screen = PAGE_MENU_SETTINGS ;
            }
        }
        // Reset menu selection
        uiState->menu_selected = 0 ;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_TOP );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_BACKUP_RESTORE( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)uiState ;

    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( uiGetPageNumOf( PAGE_MENU_SETTINGS ) );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        _ui_menuDown( uiGetPageNumOf( PAGE_MENU_SETTINGS ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        switch( ui_state.menu_selected )
        {
            case BR_BACKUP :
            {
                state.ui_screen = PAGE_MENU_BACKUP ;
                break ;
            }
            case BR_RESTORE :
            {
                state.ui_screen = PAGE_MENU_RESTORE ;
                break ;
            }
            default :
            {
                state.ui_screen = PAGE_MENU_BACKUP_RESTORE ;
            }
        }
        // Reset menu selection
        ui_state.menu_selected = 0 ;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_TOP );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_BACKUP( GuiState_st* guiState , ui_state_st* uiState )
{
    return - ui_updateFSM_PAGE_MENU_RESTORE( guiState , uiState );
}
static bool ui_updateFSM_PAGE_MENU_RESTORE( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)uiState ;

    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_TOP );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_INFO( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;
    (void)uiState ;

    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( uiGetPageNumOf( PAGE_MENU_INFO ) );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        _ui_menuDown( uiGetPageNumOf( PAGE_MENU_INFO ) );
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_TOP );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_ABOUT( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;
    (void)uiState ;

    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_TOP );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ENTER )
    {
        // Switch to set Time&Date mode
        state.ui_screen = PAGE_SETTINGS_TIMEDATE_SET ;
        // Reset input position and selection
        uiState->input_position = 0 ;
        memset( &uiState->new_timedate , 0 , sizeof( datetime_t ) );
        vp_announceBuffer( &currentLanguage->timeAndDate , true , false , "dd/mm/yy" );
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_SETTINGS );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE_SET( GuiState_st* guiState , ui_state_st* uiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ENTER )
    {
        // Save time only if all digits have been inserted
        if( uiState->input_position >= TIMEDATE_DIGITS )
        {
            // Return to Time&Date menu, saving values
            // NOTE: The user inserted a local time, we must save an UTC time
            datetime_t utc_time = localTimeToUtc( uiState->new_timedate , state.settings.utc_timezone );
            platform_setTime( utc_time );
            state.time          = utc_time ;
            vp_announceSettingsTimeDate();
            state.ui_screen     = PAGE_SETTINGS_TIMEDATE ;
        }
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_SETTINGS_TIMEDATE );
    }
    else if( input_isNumberPressed( guiState->msg ) )
    {
        // if present - discard excess digits
        if( uiState->input_position <= TIMEDATE_DIGITS )
        {
            uiState->input_position += 1;
            uiState->input_number    = input_getPressedNumber( guiState->msg );
            _ui_timedate_add_digit( &uiState->new_timedate , uiState->input_position , uiState->input_number );
        }
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_DISPLAY( GuiState_st* guiState , ui_state_st* uiState )
{
    bool sync_rtx = false ;

    if( (   guiState->msg.keys & KEY_LEFT                                         ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) && uiState->edit_mode )    )
    {
        switch(uiState->menu_selected)
        {
#ifdef SCREEN_BRIGHTNESS
            case D_BRIGHTNESS :
            {
                _ui_changeBrightness( -5 );
                vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.brightness );
                break ;
            }
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
            case D_CONTRAST :
            {
                _ui_changeContrast( -4 );
                vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.contrast );
                break ;
            }
#endif // SCREEN_CONTRAST
            case D_TIMER :
            {
                _ui_changeTimer( -1 );
                vp_announceDisplayTimer();
                break ;
            }
            default :
            {
                state.ui_screen = PAGE_SETTINGS_DISPLAY ;
            }
        }
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                                       ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) && uiState->edit_mode )    )
    {
        switch(uiState->menu_selected)
        {
#ifdef SCREEN_BRIGHTNESS
            case D_BRIGHTNESS :
            {
                _ui_changeBrightness( +5 );
                vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.brightness );
                break ;
            }
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
            case D_CONTRAST :
            {
                _ui_changeContrast( +4 );
                vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.contrast );
                break ;
            }
#endif
            case D_TIMER :
            {
                _ui_changeTimer( +1 );
                vp_announceDisplayTimer();
                break ;
            }
            default :
            {
                state.ui_screen = PAGE_SETTINGS_DISPLAY ;
            }
        }
    }
    else if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( uiGetPageNumOf( PAGE_SETTINGS_DISPLAY ) );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        _ui_menuDown( uiGetPageNumOf( PAGE_SETTINGS_DISPLAY ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        uiState->edit_mode = !uiState->edit_mode ;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_SETTINGS );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_GPS( GuiState_st* guiState , ui_state_st* uiState )
{
    bool sync_rtx = false ;

    if( (   guiState->msg.keys & ( KEY_LEFT | KEY_RIGHT )                                               ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT | KEY_UP | KNOB_RIGHT ) ) && uiState->edit_mode )    )
    {
        switch( uiState->menu_selected )
        {
            case G_ENABLED :
            {
                if( state.settings.gps_enabled )
                {
                    state.settings.gps_enabled = 0 ;
                }
                else
                {
                    state.settings.gps_enabled = 1 ;
                }
                vp_announceSettingsOnOffToggle( &currentLanguage->gpsEnabled , guiState->queueFlags , state.settings.gps_enabled );
                break ;
            }
            case G_SET_TIME :
            {
                state.gps_set_time = !state.gps_set_time ;
                vp_announceSettingsOnOffToggle( &currentLanguage->gpsSetTime , guiState->queueFlags , state.gps_set_time );
                break ;
            }
            case G_TIMEZONE :
            {
                if( guiState->msg.keys & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
                {
                    state.settings.utc_timezone -= 1 ;
                }
                else if( guiState->msg.keys & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
                {
                    state.settings.utc_timezone += 1 ;
                }
                vp_announceTimeZone( state.settings.utc_timezone , guiState->queueFlags );
                break ;
            }
            default :
            {
                state.ui_screen = PAGE_SETTINGS_GPS ;
            }
        }
    }
    else if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( uiGetPageNumOf( PAGE_SETTINGS_GPS ) );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        _ui_menuDown( uiGetPageNumOf( PAGE_SETTINGS_GPS ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        uiState->edit_mode = !uiState->edit_mode ;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_SETTINGS );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_RADIO( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    // If the entry is selected with enter we are in edit_mode
    if( uiState->edit_mode )
    {
        switch( uiState->menu_selected )
        {
            case R_OFFSET :
            {
                // Handle offset frequency input
#ifdef UI_NO_KEYBOARD
                if( guiState->msg.long_press && ( guiState->msg.keys & KEY_ENTER ) )
                {
                    // Long press on UI_NO_KEYBOARD causes digits to advance by one
                    uiState->new_offset /= 10 ;
#else // UI_NO_KEYBOARD
                if( guiState->msg.keys & KEY_ENTER)
                {
#endif // UI_NO_KEYBOARD
                    // Apply new offset
                    state.channel.tx_frequency = state.channel.rx_frequency + uiState->new_offset ;
                    vp_queueStringTableEntry( &currentLanguage->frequencyOffset );
                    vp_queueFrequency( uiState->new_offset );
                    uiState->edit_mode         = false ;
                }
                else if( guiState->msg.keys & KEY_ESC )
                {
                    // Announce old frequency offset
                    vp_queueStringTableEntry( &currentLanguage->frequencyOffset );
                    vp_queueFrequency( (int32_t)state.channel.tx_frequency - (int32_t)state.channel.rx_frequency );
                }
                else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
                {
                    _ui_numberInputDel( &uiState->new_offset );
                }
#ifdef UI_NO_KEYBOARD
                else if( guiState->msg.keys & ( KNOB_LEFT | KNOB_RIGHT | KEY_ENTER ) )
#else // UI_NO_KEYBOARD
                else if( input_isNumberPressed( guiState->msg ) )
#endif // UI_NO_KEYBOARD
                {
                    _ui_numberInputKeypad( &uiState->new_offset , guiState->msg );
                    uiState->input_position += 1 ;
                }
                else if( guiState->msg.long_press && ( guiState->msg.keys & KEY_F1 ) && ( state.settings.vpLevel > vpBeep ) )
                {
                    vp_queueFrequency( uiState->new_offset );
                    guiState->f1Handled = true ;
                }
                break ;
            }
            case R_DIRECTION :
            {
                if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KNOB_LEFT | KNOB_RIGHT ) )
                {
                    // Invert frequency offset direction
                    if( state.channel.tx_frequency >= state.channel.rx_frequency )
                    {
                        state.channel.tx_frequency -= 2 * ((int32_t)state.channel.tx_frequency - (int32_t)state.channel.rx_frequency );
                    }
                    else // Switch to positive offset
                    {
                        state.channel.tx_frequency -= 2 * ((int32_t)state.channel.tx_frequency - (int32_t)state.channel.rx_frequency );
                    }
                }
                break ;
            }
            case R_STEP :
            {
                if( guiState->msg.keys & ( KEY_UP | KEY_RIGHT | KNOB_RIGHT ) )
                {
                    // Cycle over the available frequency steps
                    state.step_index++ ;
                    state.step_index %= n_freq_steps ;
                }
                else if( guiState->msg.keys & ( KEY_DOWN | KEY_LEFT | KNOB_LEFT ) )
                {
                    state.step_index += n_freq_steps ;
                    state.step_index-- ;
                    state.step_index %= n_freq_steps ;
                }
                break ;
            }
            default :
            {
                state.ui_screen = PAGE_SETTINGS_RADIO ;
                break ;
            }
        }
        // If ENTER or ESC are pressed, exit edit mode, R_OFFSET is managed separately
        if( ( ( guiState->msg.keys & KEY_ENTER ) && ( uiState->menu_selected != R_OFFSET ) ) ||
              ( guiState->msg.keys & KEY_ESC   )                                                )
        {
            uiState->edit_mode = false ;
        }
    }
    else if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( uiGetPageNumOf( PAGE_SETTINGS_RADIO ) );
    }
    else if( ( guiState->msg.keys & KEY_DOWN ) || ( guiState->msg.keys & KNOB_RIGHT ) )
    {
        _ui_menuDown( uiGetPageNumOf( PAGE_SETTINGS_RADIO ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        uiState->edit_mode = true;
        // If we are entering R_OFFSET clear temp offset
        if( uiState->menu_selected == R_OFFSET )
        {
            uiState->new_offset = 0 ;
        }
        // Reset input position
        uiState->input_position = 0 ;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_SETTINGS );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_M17( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    if( uiState->edit_mode )
    {
        switch( uiState->menu_selected )
        {
            case M17_CALLSIGN :
            {
                // Handle text input for M17 callsign
                if( guiState->msg.keys & KEY_ENTER )
                {
                    _ui_textInputConfirm( uiState->new_callsign );
                    // Save selected callsign and disable input mode
                    strncpy( state.settings.callsign , uiState->new_callsign , 10 );
                    uiState->edit_mode = false;
                    vp_announceBuffer( &currentLanguage->callsign , false , true , state.settings.callsign );
                }
                else if( guiState->msg.keys & KEY_ESC )
                {
                    // Discard selected callsign and disable input mode
                    uiState->edit_mode = false ;
                    vp_announceBuffer( &currentLanguage->callsign , false , true , state.settings.callsign );
                }
                else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
                {
                    _ui_textInputDel( uiState->new_callsign );
                }
                else if( input_isNumberPressed( guiState->msg ) )
                {
                    _ui_textInputKeypad( uiState->new_callsign , 9 , guiState->msg , true );
                }
                else if( guiState->msg.long_press && ( guiState->msg.keys & KEY_F1 ) && ( state.settings.vpLevel > vpBeep ) )
                {
                    vp_announceBuffer( &currentLanguage->callsign , true , true , uiState->new_callsign );
                    guiState->f1Handled = true ;
                }
                break ;
            }
            case M17_CAN :
            {
                if( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) )
                {
                    _ui_changeM17Can( -1 );
                }
                else if( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) )
                {
                    _ui_changeM17Can( +1 );
                }
                else if( guiState->msg.keys & KEY_ENTER )
                {
                    uiState->edit_mode = !uiState->edit_mode ;
                }
                else if( guiState->msg.keys & KEY_ESC )
                {
                    uiState->edit_mode = false ;
                }
                break;
            }
            case M17_CAN_RX :
            {
                if( (   guiState->msg.keys & ( KEY_LEFT | KEY_RIGHT                       )                         ) ||
                    ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT | KEY_UP | KNOB_RIGHT ) ) && uiState->edit_mode )    )
                {
                    state.settings.m17_can_rx = !state.settings.m17_can_rx ;
                }
                else if( guiState->msg.keys & KEY_ENTER )
                {
                    uiState->edit_mode = !uiState->edit_mode ;
                }
                else if( guiState->msg.keys & KEY_ESC )
                {
                    uiState->edit_mode = false ;
                }
                break ;
            }
        }
    }
    else
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            // Enable edit mode
            uiState->edit_mode = true;

            // If callsign input, reset text input variables
            if(uiState->menu_selected == M17_CALLSIGN)
            {
                _ui_textInputReset( uiState->new_callsign );
                vp_announceBuffer( &currentLanguage->callsign , true , true , uiState->new_callsign );
            }
        }
        else if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            _ui_menuUp( uiGetPageNumOf( PAGE_SETTINGS_M17 ) );
        }
        else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
        {
            _ui_menuDown( uiGetPageNumOf( PAGE_SETTINGS_M17 ) );
        }
        else if( ( guiState->msg.keys & KEY_RIGHT ) && ( uiState->menu_selected == M17_CAN ) )
        {
            _ui_changeM17Can( +1 );
        }
        else if( ( guiState->msg.keys & KEY_LEFT ) && ( uiState->menu_selected == M17_CAN ) )
        {
            _ui_changeM17Can( -1 );
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            sync_rtx = true ;
            _ui_menuBack( PAGE_MENU_SETTINGS );
        }
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_VOICE( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    if( (   guiState->msg.keys & KEY_LEFT                                         ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) && uiState->edit_mode )    )
    {
        switch( uiState->menu_selected )
        {
            case VP_LEVEL :
            {
                _ui_changeVoiceLevel( -1 );
                break ;
            }
            case VP_PHONETIC :
            {
                _ui_changePhoneticSpell( false );
                break ;
            }
            default :
            {
                state.ui_screen = PAGE_SETTINGS_VOICE ;
            }
        }
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                                       ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) && uiState->edit_mode )    )
    {
        switch( uiState->menu_selected )
        {
            case VP_LEVEL :
            {
                _ui_changeVoiceLevel( 1 );
                break ;
            }
            case VP_PHONETIC :
            {
                _ui_changePhoneticSpell( true );
                break ;
            }
            default :
            {
                state.ui_screen = PAGE_SETTINGS_VOICE ;
            }
        }
    }
    else if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( uiGetPageNumOf( PAGE_SETTINGS_VOICE ) );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        _ui_menuDown( uiGetPageNumOf( PAGE_SETTINGS_VOICE ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        uiState->edit_mode = !uiState->edit_mode ;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( PAGE_MENU_SETTINGS );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_RESET_TO_DEFAULTS( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    if( !uiState->edit_mode )
    {
        //require a confirmation ENTER, then another
        //edit_mode is slightly misused to allow for this
        if( guiState->msg.keys & KEY_ENTER )
        {
            uiState->edit_mode = true ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            _ui_menuBack( PAGE_MENU_SETTINGS );
        }
    }
    else
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            uiState->edit_mode = false ;
            state_resetSettingsAndVfo();
            _ui_menuBack( PAGE_MENU_SETTINGS );
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            uiState->edit_mode = false ;
            _ui_menuBack( PAGE_MENU_SETTINGS );
        }
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_LOW_BAT( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;
    (void)uiState ;

    return false ;
}

static bool ui_updateFSM_PAGE_AUTHORS( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;
    (void)uiState ;

    return false ;
}

static bool ui_updateFSM_PAGE_BLANK( GuiState_st* guiState , ui_state_st* uiState )
{
    (void)guiState ;
    (void)uiState ;

    return false ;
}

//@@@KL this is constantly being called
// implies that something is setting the redraw_needed flag to true
bool ui_updateGUI( void )
{
    if( redraw_needed == true )
    {
        ui_draw( &last_state , &ui_state );
        // If MACRO menu is active draw it
        if( macro_menu )
        {
            _ui_drawDarkOverlay();
            _ui_drawMacroMenu( &ui_state );
        }

#ifdef DISPLAY_DEBUG_MSG
        DisplayDebugMsg();
#endif // DISPLAY_DEBUG_MSG

        redraw_needed = false ;

    }

    return true;
}

bool ui_pushEvent( const uint8_t type , const uint32_t data )
{
    uint8_t newHead = ( evQueue_wrPos + 1 ) % MAX_NUM_EVENTS ;

    // Queue is full
    if( newHead == evQueue_rdPos )
    {
        return false ;
    }
    // Preserve atomicity when writing the new element into the queue.
    event_t event;
    event.type    = type ;
    event.payload = data ;

    evQueue[ evQueue_wrPos ] = event ;
    evQueue_wrPos            = newHead ;

    return true ;
}

void ui_terminate( void )
{
}
