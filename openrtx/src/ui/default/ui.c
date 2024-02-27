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
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <input.h>
#include <hwconfig.h>
#include <voicePromptUtils.h>
#include <ui/ui_default.h>
#include <rtx.h>
#include <interfaces/platform.h>
#include <interfaces/display.h>
#include <interfaces/cps_io.h>
#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include <string.h>
#include <battery.h>
#include <utils.h>
#include <beeps.h>
#include <memory_profiling.h>

#ifdef PLATFORM_TTWRPLUS
#include <SA8x8.h>
#endif

//@@@KL #include "ui_m17.h"

#define ST_VAL( val )   ( val + GUI_CMD_NUM_OF )
#define LD_VAL( val )   (uint8_t)( val - GUI_CMD_NUM_OF )

#ifdef DISPLAY_DEBUG_MSG

static char    counter = 0 ; //@@@KL
static uint8_t trace0  = 0 ; //@@@KL
static uint8_t trace1  = 0 ; //@@@KL
static uint8_t trace2  = 0 ; //@@@KL
static uint8_t trace3  = 0 ; //@@@KL

void Debug_SetTrace0( uint8_t traceVal )
{
    trace0 = traceVal ;
}

void Debug_SetTrace1( uint8_t traceVal )
{
    trace1 = traceVal ;
}

void Debug_SetTrace2( uint8_t traceVal )
{
    trace2 = traceVal ;
}

void Debug_SetTrace3( uint8_t traceVal )
{
    trace3 = traceVal ;
}

static void Debug_DisplayMsg( void )
{
    uint8_t  lineIndex = GUI_LINE_TOP ;
    uint16_t height    = GuiState.layout.lineStyle[ lineIndex ].height ;
    uint16_t width     = 56 ;
    Pos_st   start ;
    Color_st color_bg ;
    Color_st color_fg ;

    start.y = ( GuiState.layout.lineStyle[ lineIndex ].pos.y - height ) + 1 ;
    start.x = 0 ;

    uiColorLoad( &color_bg , COLOR_BG );
    uiColorLoad( &color_fg , COLOR_FG );

    gfx_drawRect( start , width , height , color_bg , true );

    gfx_print( GuiState.layout.lineStyle[ lineIndex ].pos       ,
               GuiState.layout.lineStyle[ lineIndex ].font.size , ALIGN_LEFT , color_fg ,
               "%c%X%X%X%X" , (char)( '0' + counter ) ,
               trace0 & 0x0F , trace1 & 0x0F , trace2 & 0x0F , trace3 & 0x0F );//@@@KL

    if( counter < 10 )
    {
        counter++ ;
    }
    else
    {
        counter = 0 ;
    }

}

#endif // DISPLAY_DEBUG_MSG

static void ui_InitUIState( UI_State_st* uiState );
static void ui_InitGuiState( GuiState_st* guiState );
static void ui_InitGuiStateEvent( Event_st* event );
static void ui_InitGuiStatePage( Page_st* page );
static void ui_InitGuiStateLayout( Layout_st* layout );
/* UI main screen functions, their implementation is in "ui_main.c" */
extern void ui_draw( GuiState_st* guiState , Event_st* sysEvent );
extern bool _ui_drawMacroMenu( GuiState_st* guiState );
extern void _ui_reset_menu_anouncement_tracking( void );

extern void ui_drawMenuItem( GuiState_st* guiState , char* entryBuf );

extern const char* display_timer_values[];

static bool ui_InputValue( GuiState_st* guiState , bool* handled );
       void ui_SetPageNum( GuiState_st* guiState , uint8_t pageNum );

//@@@KL change all strings over to use englishStrings in EnglishStrings.h

static const uint8_t Page_MainVFO[] =
{
//    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) ,
                          ST_VAL( EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      GUI_CMD_VALUE , ST_VAL( GUI_VAL_CURRENT_TIME ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) ,
                          ST_VAL( EVENT_STATUS_BATTERY ) ,
      GUI_CMD_VALUE , ST_VAL( GUI_VAL_BATTERY_LEVEL ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) ,
                          ST_VAL( EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      GUI_CMD_VALUE , ST_VAL( GUI_VAL_LOCK_STATE ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_VALUE , ST_VAL( GUI_VAL_MODE_INFO ) ,
    GUI_CMD_VALUE , ST_VAL( GUI_VAL_FREQUENCY ) ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) ,
                          ST_VAL( EVENT_STATUS_RSSI ) ,
      GUI_CMD_VALUE , ST_VAL( GUI_VAL_RSSI_METER ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MainInput[] =
{
    GUI_CMD_PAGE_END ,   //@@@KL indicates use the legacy script
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) ,
                          ST_VAL( EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      GUI_CMD_VALUE , ST_VAL( GUI_VAL_CURRENT_TIME ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) ,
                          ST_VAL( EVENT_STATUS_BATTERY ) ,
      GUI_CMD_VALUE , ST_VAL( GUI_VAL_BATTERY_LEVEL ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) ,
                          ST_VAL( EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      GUI_CMD_VALUE , ST_VAL( GUI_VAL_LOCK_STATE ) ,
    GUI_CMD_EVENT_END ,
//@@@KL
/*
    ui_drawMainTop( guiState , event );
    if( !update )
    {
        ui_drawVFOMiddleInput( guiState );
    }
    _ui_drawMainBottom( guiState , event );
*/
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) ,
                          ST_VAL( EVENT_STATUS_RSSI ) ,
      GUI_CMD_VALUE , ST_VAL( GUI_VAL_RSSI_METER ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_PAGE_END

};

static const uint8_t Page_MainMem[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_ModeVFO[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_ModeMem[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_MenuItems[] =
{
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_TOP ) ,
    GUI_CMD_ALIGN_CENTER ,
    GUI_CMD_TITLE ,
    'M','e','n','u' , GUI_CMD_NULL ,
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_ALIGN_LEFT ,
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_MENU_BANK ) ,
    GUI_CMD_TEXT , 'B','a','n','k','s' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_MENU_CHANNEL ) ,
    GUI_CMD_TEXT , 'C','h','a','n','n','e','l','s' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_MENU_CONTACTS ) ,
    GUI_CMD_TEXT , 'C','o','n','t','a','c','t','s' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
#ifdef GPS_PRESENT
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_MENU_GPS ) ,
    GUI_CMD_TEXT , 'G','P','S' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
#endif // RTC_PRESENT
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_MENU_SETTINGS ) ,
    GUI_CMD_TEXT , 'S','e','t','t','i','n','g','s' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_MENU_INFO ) ,
    GUI_CMD_TEXT , 'I','n','f','o' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_MENU_ABOUT ) ,
    GUI_CMD_TEXT , 'A','b','o','u','t' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuBank[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_MenuChannel[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_MenuContact[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_MenuGPS[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_MenuSettings[] =
{
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_TOP ) ,
    GUI_CMD_ALIGN_CENTER ,
    GUI_CMD_TITLE ,
    'S','e','t','t','i','n','g','s' , GUI_CMD_NULL ,
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_ALIGN_LEFT ,
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_SETTINGS_DISPLAY ) ,
    GUI_CMD_TEXT , 'D','i','s','p','l','a','y' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
#ifdef RTC_PRESENT
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_SETTINGS_TIMEDATE ) ,
    GUI_CMD_TEXT , 'T','i','m','e',' ','&',' ','D','a','t','e' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
#endif // RTC_PRESENT
#ifdef GPS_PRESENT
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_SETTINGS_GPS ) ,
    GUI_CMD_TEXT , 'G','P','S' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
#endif // GPS_PRESENT
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_SETTINGS_RADIO ) ,
    GUI_CMD_TEXT , 'R','a','d','i','o' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_SETTINGS_M17 ) ,
    GUI_CMD_TEXT , 'M','1','7' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_SETTINGS_VOICE ) ,
    GUI_CMD_TEXT , 'A','c','c','e','s','s','i','b','i','l','i','t','y' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_SETTINGS_RESET_TO_DEFAULTS ) ,
    GUI_CMD_TEXT , 'D','e','f','a','u','l','t',' ','S','e','t','t','i','n','g','s' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuAbout[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_SettingsTimeDate[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_SettingsTimeDateSet[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_SettingsDisplay[] =
{
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_TOP ) ,
    GUI_CMD_ALIGN_CENTER ,
    GUI_CMD_TITLE ,
    'D','i','s','p','l','a','y' , GUI_CMD_NULL ,
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_1 ) ,
#ifdef SCREEN_BRIGHTNESS
    GUI_CMD_LINK ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'B','r','i','g','h','t','n','e','s','s' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT ,
    GUI_CMD_VALUE , ST_VAL( GUI_VAL_BRIGHTNESS ) ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
    GUI_CMD_LINK ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'C','o','n','t','r','a','s','t' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT ,
    GUI_CMD_VALUE , ST_VAL( GUI_VAL_CONTRAST ) ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
#endif // SCREEN_CONTRAST
    GUI_CMD_LINK ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'T','i','m','e','r' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT ,
    GUI_CMD_VALUE , ST_VAL( GUI_VAL_TIMER ) ,
    GUI_CMD_LINK_END ,
    GUI_CMD_PAGE_END
};

#ifdef GPS_PRESENT
static const uint8_t Page_SettingsGPS[] =
{
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_TOP ) ,
    GUI_CMD_ALIGN_CENTER ,
    GUI_CMD_TITLE ,
     'G','P','S',' ','S','e','t','t','i','n','g','s' , GUI_CMD_NULL ,
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_ALIGN_LEFT ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'G','P','S',' ','E','n','a','b','l','e','d' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_GPS_ENABLED ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'G','P','S',' ','S','e','t',' ','T','i','m','e' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_GPS_SET_TIME ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'U','T','C',' ','T','i','m','e','z','o','n','e', GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_GPS_TIME_ZONE ) ,
    GUI_CMD_PAGE_END
};

#endif // GPS_PRESENT

static const uint8_t Page_SettingsRadio[] =
{
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_TOP ) ,
    GUI_CMD_ALIGN_CENTER ,
    GUI_CMD_TITLE ,
     'R','a','d','i','o',' ','S','e','t','t','i','n','g','s' , GUI_CMD_NULL ,
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'O','f','f','s','e','t', GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_RADIO_OFFSET ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'D','i','r','e','c','t','i','o','n', GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_RADIO_DIRECTION ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'S','t','e','p' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_RADIO_STEP ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_SettingsM17[] =
{
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_TOP ) ,
    GUI_CMD_ALIGN_CENTER ,
    GUI_CMD_TITLE ,
     'M','1','7',' ','S','e','t','t','i','n','g','s' , GUI_CMD_NULL ,
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'C','a','l','l','s','i','g','n' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_M17_CALLSIGN ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'C','A','N' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_M17_CAN ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'C','A','N',' ','R','X',' ','C','h','e','c','k' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_M17_CAN_RX_CHECK ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_SettingsVoice[] =
{
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_TOP ) ,
    GUI_CMD_ALIGN_CENTER ,
    GUI_CMD_TITLE ,
     'V','o','i','c','e' , GUI_CMD_NULL ,
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'V','o','i','c','e' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_LEVEL ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'P','h','o','n','e','t','i','c' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_PHONETIC ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuBackupRestore[] =
{
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'B','a','c','k','u','p' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_STUBBED ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'R','e','s','t','o','r','e' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_STUBBED ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuBackup[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_MenuRestore[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_MenuInfo[] =
{
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'B','a','t','.',' ','V','o','l','t','a','g','e' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_BATTERY_VOLTAGE ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'B','a','t','.',' ','C','h','a','r','g','e' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_BATTERY_CHARGE ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'R','S','S','I' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_RSSI ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'U','s','e','d',' ','h','e','a','p' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_USED_HEAP ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'B','a','n','d' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_BAND ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'V','H','F' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_VHF ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'U','H','F' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_UHF ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'H','W',' ','V','e','r','s','i','o','n' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_HW_VERSION ) ,
    GUI_CMD_LINE_END ,
#ifdef PLATFORM_TTWRPLUS
    GUI_CMD_ALIGN_LEFT , GUI_CMD_TEXT ,
     'R','a','d','i','o' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_RADIO ) ,
    GUI_CMD_LINE_END ,
     'R','a','d','i','o',' ','F','W' , GUI_CMD_NULL ,
    GUI_CMD_ALIGN_RIGHT , GUI_CMD_VALUE , ST_VAL( GUI_VAL_RADIO_FW ) ,
#endif // PLATFORM_TTWRPLUS
    GUI_CMD_PAGE_END
};

static const uint8_t Page_SettingsResetToDefaults[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_LowBat[] =
{
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_About[] =
{
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_TOP ) ,
    GUI_CMD_ALIGN_CENTER ,
    GUI_CMD_TITLE ,
     'A','b','o','u','t' , GUI_CMD_NULL ,
    GUI_CMD_LINE_STYLE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_ALIGN_LEFT ,
    GUI_CMD_TEXT ,
     'A','u','t','h','o','r','s' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT ,
     'N','i','c','c','o','l','o',' ',
     'I','U','2','K','I','N' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT ,
     'S','i','l','v','a','n','o',' ',
     'I','U','2','K','W','O' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT ,
     'F','e','d','e','r','i','c','o',' ',
     'I','U','2','N','U','O' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT ,
     'F','r','e','d',' ',
     'I','U','2','N','R','O' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT ,
     'K','i','m',' ',
     'V','K','6','K','L', GUI_CMD_NULL ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_Stubbed[] =
{
    GUI_CMD_ALIGN_LEFT ,
    GUI_CMD_TEXT ,
     'P','a','g','e',' ',
     'S','t','u','b','b','e','d', GUI_CMD_NULL ,
    GUI_CMD_PAGE_END
};

#define PAGE_REF( loc )    loc

static const uint8_t* uiPageTable[] =
{
    PAGE_REF( Page_MainVFO                 ) , // PAGE_MAIN_VFO
    PAGE_REF( Page_MainInput               ) , // PAGE_MAIN_VFO_INPUT
    PAGE_REF( Page_MainMem                 ) , // PAGE_MAIN_MEM
    PAGE_REF( Page_ModeVFO                 ) , // PAGE_MODE_VFO
    PAGE_REF( Page_ModeMem                 ) , // PAGE_MODE_MEM
    PAGE_REF( Page_MenuItems               ) , // PAGE_MENU_TOP
    PAGE_REF( Page_MenuBank                ) , // PAGE_MENU_BANK
    PAGE_REF( Page_MenuChannel             ) , // PAGE_MENU_CHANNEL
    PAGE_REF( Page_MenuContact             ) , // PAGE_MENU_CONTACTS
    PAGE_REF( Page_MenuGPS                 ) , // PAGE_MENU_GPS
    PAGE_REF( Page_MenuSettings            ) , // PAGE_MENU_SETTINGS
    PAGE_REF( Page_MenuBackupRestore       ) , // PAGE_MENU_BACKUP_RESTORE
    PAGE_REF( Page_MenuBackup              ) , // PAGE_MENU_BACKUP
    PAGE_REF( Page_MenuRestore             ) , // PAGE_MENU_RESTORE
    PAGE_REF( Page_MenuInfo                ) , // PAGE_MENU_INFO
    PAGE_REF( Page_MenuAbout               ) , // PAGE_MENU_ABOUT
    PAGE_REF( Page_SettingsTimeDate        ) , // PAGE_SETTINGS_TIMEDATE
    PAGE_REF( Page_SettingsTimeDateSet     ) , // PAGE_SETTINGS_TIMEDATE_SET
    PAGE_REF( Page_SettingsDisplay         ) , // PAGE_SETTINGS_DISPLAY
#ifdef GPS_PRESENT
    PAGE_REF( Page_SettingsGPS             ) , // PAGE_SETTINGS_GPS
#endif // GPS_PRESENT
    PAGE_REF( Page_SettingsRadio           ) , // PAGE_SETTINGS_RADIO
    PAGE_REF( Page_SettingsM17             ) , // PAGE_SETTINGS_M17
    PAGE_REF( Page_SettingsVoice           ) , // PAGE_SETTINGS_VOICE
    PAGE_REF( Page_SettingsResetToDefaults ) , // PAGE_SETTINGS_RESET_TO_DEFAULTS
    PAGE_REF( Page_LowBat                  ) , // PAGE_LOW_BAT
    PAGE_REF( Page_About                   ) , // PAGE_ABOUT
    PAGE_REF( Page_Stubbed                 )   // PAGE_STUBBED
};

GuiState_st GuiState ;

static const char* symbols_ITU_T_E161[] =
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

static const char* symbols_ITU_T_E161_callsign[] =
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

static bool GuiCmd_Null( GuiState_st* guiState );
static bool GuiCmd_EventStart( GuiState_st* guiState );
static bool GuiCmd_EventEnd( GuiState_st* guiState );
static bool GuiCmd_GoToLine( GuiState_st* guiState );
static bool GuiCmd_AlignLeft( GuiState_st* guiState );
static bool GuiCmd_AlignCenter( GuiState_st* guiState );
static bool GuiCmd_AlignRight( GuiState_st* guiState );
static bool GuiCmd_FontSize( GuiState_st* guiState );
static bool GuiCmd_LineEnd( GuiState_st* guiState );
static bool GuiCmd_Link( GuiState_st* guiState );
static bool GuiCmd_LinkEnd( GuiState_st* guiState );
static bool GuiCmd_Page( GuiState_st* guiState );
static bool GuiCmd_Value( GuiState_st* guiState );
static bool GuiCmd_Title( GuiState_st* guiState );
static bool GuiCmd_Text( GuiState_st* guiState );
static bool GuiCmd_PageEnd( GuiState_st* guiState );
static bool GuiCmd_Stubbed( GuiState_st* guiState );

static bool GuiCmd_Select( GuiState_st* guiState );
static void GuiCmd_DisplayValue( GuiState_st* guiState , uint8_t valueNum );
static void GuiCmd_AdvToNextCmd( GuiState_st* guiState );

typedef bool (*ui_GuiCmd_fn)( GuiState_st* guiState );

static const ui_GuiCmd_fn ui_GuiCmd_Table[ GUI_CMD_NUM_OF ] =
{
    GuiCmd_Null        , // 0x00
    GuiCmd_EventStart  , // 0x01
    GuiCmd_EventEnd    , // 0x02
    GuiCmd_GoToLine    , // 0x03
    GuiCmd_AlignLeft   , // 0x04
    GuiCmd_AlignCenter , // 0x05
    GuiCmd_AlignRight  , // 0x06
    GuiCmd_FontSize    , // 0x07
    GuiCmd_Stubbed     , // 0x08
    GuiCmd_Stubbed     , // 0x09
    GuiCmd_LineEnd     , // 0x0A
    GuiCmd_Link        , // 0x0B
    GuiCmd_LinkEnd     , // 0x0C
    GuiCmd_Stubbed     , // 0x0D
    GuiCmd_Page        , // 0x0E
    GuiCmd_Value       , // 0x0F
    GuiCmd_Title       , // 0x10
    GuiCmd_Text        , // 0x11
    GuiCmd_Stubbed     , // 0x12
    GuiCmd_Stubbed     , // 0x13
    GuiCmd_Stubbed     , // 0x14
    GuiCmd_Stubbed     , // 0x15
    GuiCmd_Stubbed     , // 0x16
    GuiCmd_Stubbed     , // 0x17
    GuiCmd_Stubbed     , // 0x18
    GuiCmd_Stubbed     , // 0x19
    GuiCmd_Stubbed     , // 0x1A
    GuiCmd_Stubbed     , // 0x1B
    GuiCmd_Stubbed     , // 0x1C
    GuiCmd_Stubbed     , // 0x1D
    GuiCmd_Stubbed     , // 0x1E
    GuiCmd_PageEnd       // 0x1F
};

bool ui_DisplayPage( GuiState_st* guiState )
{
    uint16_t count ;
    uint8_t  cmd ;
    bool     exit ;
    bool     pageDisplayed = false ;

    guiState->layout.printDisplayOn = true ;
    guiState->layout.inSelect       = false ;

    if( guiState->page.num != guiState->page.levelList[ guiState->page.level ] )
    {
        if( guiState->page.level < MAX_PAGE_DEPTH )
        {
            guiState->page.level++ ;
        }
        guiState->page.levelList[ guiState->page.level ] = guiState->page.num ;
    }

    guiState->page.ptr              = (uint8_t*)uiPageTable[ guiState->page.num ] ;

    guiState->layout.numOfEntries   = 0 ;
    guiState->layout.lineIndex      = 0 ;

    guiState->layout.linkNumOf      = 0 ;
    guiState->layout.linkIndex      = 0 ;

    if( guiState->page.ptr[ 0 ] != GUI_CMD_PAGE_END )
    {
        guiState->layout.line.pos.y   = 0 ;
        guiState->layout.line.pos.x   = 0 ;

        guiState->layout.scrollOffset = 0 ;

	    guiState->page.index          = 0 ;

	    if( !guiState->update )
	    {
            guiState->pageHasEvents = false ;
	    }

	    if( !guiState->pageHasEvents )
	    {
            gfx_clearScreen();
            for( exit = false , count = 256 ; !exit && count ; count-- )
            {
                cmd = guiState->page.ptr[ guiState->page.index ] ;

                if( cmd < GUI_CMD_DATA_AREA )
                {
                    exit = ui_GuiCmd_Table[ cmd ]( guiState );
                }
                else
                {
                    guiState->page.index++ ;
                }
            }
	    }
	    else
	    {
            for( exit = false , count = 256 ; !exit && count ; count-- )
            {
                cmd = guiState->page.ptr[ guiState->page.index ] ;

                if( cmd < GUI_CMD_DATA_AREA )
                {
                    if( ( cmd == GUI_CMD_EVENT_START ) ||
                        guiState->inEventArea             )
                    {
                        exit = ui_GuiCmd_Table[ cmd ]( guiState );
                    }
                    else
                    {
                        guiState->page.index++ ;
                    }
                }
                else
                {
                    guiState->page.index++ ;
                }
            }
	    }

	    pageDisplayed = true ;

	}

    return pageDisplayed ;

}

static bool GuiCmd_Null( GuiState_st* guiState )
{
    bool pageEnd = false ;

    printf( "Cmd NULL" );

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_EventStart( GuiState_st* guiState )
{
    uint8_t  eventType ;
    uint8_t  eventPayload ;
    uint16_t count   = 256 ;
    bool     pageEnd = false ;

    guiState->page.index++ ;
    eventType    = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->page.index++ ;
    eventPayload = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->page.index++ ;

    if( !( ( guiState->event.type    == eventType    ) &&
           ( guiState->event.payload == eventPayload )    ) )
    {
        while( ( guiState->page.ptr[ guiState->page.index ] != GUI_CMD_EVENT_END ) &&
               ( guiState->page.ptr[ guiState->page.index ] != GUI_CMD_PAGE_END  )    )
        {
            count-- ;
            if( count == 0 )
            {
                break ;
            }
            guiState->page.index++ ;
        }
    }
    else
    {
        guiState->inEventArea = true ;
    }

    guiState->pageHasEvents = true ;

    return pageEnd ;

}

static bool GuiCmd_EventEnd( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    guiState->inEventArea = false ;

    return pageEnd ;

}

static bool GuiCmd_GoToLine( GuiState_st* guiState )
{
    uint8_t lineIndex ;
    bool    pageEnd = false ;

    guiState->page.index++ ;

    lineIndex                  = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->layout.lineIndex = lineIndex ;
    guiState->layout.line 	   = guiState->layout.lineStyle[ lineIndex ] ;

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_AlignLeft( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    guiState->layout.line.align = ALIGN_LEFT ;

    return pageEnd ;

}

static bool GuiCmd_AlignCenter( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    guiState->layout.line.align = ALIGN_CENTER ;

    return pageEnd ;

}

static bool GuiCmd_AlignRight( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    guiState->layout.line.align = ALIGN_RIGHT ;

    return pageEnd ;

}

static bool GuiCmd_FontSize( GuiState_st* guiState )
{
    uint8_t fontSize ;
    bool    pageEnd  = false ;

    guiState->page.index++ ;

    fontSize                        = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->layout.line.font.size = fontSize ;

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_LineEnd( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    if( guiState->layout.printDisplayOn )
    {
        guiState->layout.line.pos.y += guiState->layout.menu_h ;
    }

    if( ( guiState->layout.lineIndex + 1 ) < GUI_LINE_NUM_OF )
    {
        guiState->layout.lineIndex++ ;
    }

    return pageEnd ;

}

static bool GuiCmd_Link( GuiState_st* guiState )
{
    bool pageEnd ;

    guiState->page.index++ ;

    if( guiState->layout.linkNumOf < LINK_MAX_NUM_OF )
    {
        guiState->layout.linkNumOf++ ;
    }

    pageEnd = GuiCmd_Select( guiState );

    return pageEnd ;
}

static bool GuiCmd_LinkEnd( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    if( guiState->layout.linkNumOf < LINK_MAX_NUM_OF )
    {
        guiState->layout.linkIndex++ ;
    }

    guiState->layout.inSelect = false ;

    return pageEnd ;

}

static bool GuiCmd_Page( GuiState_st* guiState )
{
    uint8_t linkNum ;
    bool    pageEnd = false ;

    guiState->page.index++ ;

    linkNum = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->layout.links[ guiState->layout.linkIndex ].type = LINK_TYPE_PAGE ;
    guiState->layout.links[ guiState->layout.linkIndex ].num  = linkNum ;
    guiState->layout.links[ guiState->layout.linkIndex ].amt  = 0 ;

    guiState->page.index++ ;

    return pageEnd ;
}

static bool GuiCmd_Value( GuiState_st* guiState )
{
    uint8_t valueNum ;
    bool    pageEnd  = false ;

    guiState->page.index++ ;

    valueNum = LD_VAL( guiState->page.ptr[ guiState->page.index ] );

    if( valueNum >= GUI_VAL_NUM_OF )
    {
        valueNum = GUI_VAL_STUBBED ;
    }

    guiState->layout.links[ guiState->layout.linkIndex ].type = LINK_TYPE_VALUE ;
    guiState->layout.links[ guiState->layout.linkIndex ].num  = valueNum ;
    guiState->layout.links[ guiState->layout.linkIndex ].amt  = 0 ;

    GuiCmd_DisplayValue( guiState , valueNum );

    GuiCmd_AdvToNextCmd( guiState );

    return pageEnd ;

}

static bool GuiCmd_Title( GuiState_st* guiState )
{
    uint8_t* scriptPtr ;
    Color_st color_fg ;
    bool     pageEnd   = false ;

    guiState->page.index++ ;

    scriptPtr = &guiState->page.ptr[ guiState->page.index ] ;

    uiColorLoad( &color_fg , COLOR_FG );

    guiState->layout.lineIndex = GUI_LINE_TOP ;

    // print the title on the top bar
    gfx_print( guiState->layout.line.pos       ,
               guiState->layout.line.font.size ,
               guiState->layout.line.align     ,
               color_fg , (char*)scriptPtr );

    GuiCmd_AdvToNextCmd( guiState );

    return pageEnd ;

}

static bool GuiCmd_Text( GuiState_st* guiState )
{
    uint8_t* scriptPtr ;
    Color_st color_fg ;
    Color_st color_bg ;
    Color_st color_text ;
    bool     pageEnd    = false ;

    uiColorLoad( &color_fg , COLOR_FG );
    uiColorLoad( &color_bg , COLOR_BG );
    color_text = color_fg ;

    while( guiState->page.ptr[ guiState->page.index ] < GUI_CMD_NUM_OF )
    {
        guiState->page.index++ ;
    }

    scriptPtr = &guiState->page.ptr[ guiState->page.index ] ;

    if( guiState->layout.printDisplayOn )
    {
        if( guiState->layout.inSelect )
        {
            color_text      = color_bg ;
            // Draw rectangle under selected item, compensating for text height
            Pos_st rect_pos = { 0 , guiState->layout.line.pos.y - guiState->layout.menu_h + 3 };
            gfx_drawRect( rect_pos , SCREEN_WIDTH , guiState->layout.menu_h , color_fg , true );
        }
//@@@KL            announceMenuItemIfNeeded( entryBuf , NULL , false );
        gfx_print( guiState->layout.line.pos       ,
                   guiState->layout.line.font.size ,
                   guiState->layout.line.align     ,
                   color_text , (char*)scriptPtr );
    }

    GuiCmd_AdvToNextCmd( guiState );

    return pageEnd ;

}

static bool GuiCmd_PageEnd( GuiState_st* guiState )
{
    bool pageEnd = true ;

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_Stubbed( GuiState_st* guiState )
{
    bool pageEnd = false ;

    printf( "Cmd Stubbed" );

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_Select( GuiState_st* guiState )
{
    bool    pageEnd = false ;

    if( guiState->layout.linkIndex == 0 )
    {
        // Number of menu entries that fit in the screen height
        guiState->layout.numOfEntries = ( SCREEN_HEIGHT - 1 - guiState->layout.line.pos.y ) /
                                        guiState->layout.menu_h + 1 ;
    }

    // If selection is off the screen, scroll screen
    if( guiState->uiState.menu_selected >= guiState->layout.numOfEntries )
    {
        guiState->layout.scrollOffset = guiState->uiState.menu_selected -
                                        guiState->layout.numOfEntries + 1 ;
    }

    guiState->layout.printDisplayOn = false ;
    guiState->layout.inSelect       = false ;

    if( (   guiState->layout.linkIndex >= guiState->layout.scrollOffset ) &&
        ( ( guiState->layout.linkIndex  - guiState->layout.scrollOffset )  < guiState->layout.numOfEntries ) )
    {
        if( guiState->layout.linkIndex == guiState->uiState.menu_selected )
        {
            guiState->layout.inSelect = true ;
        }
        guiState->layout.printDisplayOn = true ;
    }

    return pageEnd ;

}

static void GuiCmd_AdvToNextCmd( GuiState_st* guiState )
{
    uint16_t count = 256 ;

    while( ( guiState->page.ptr[ guiState->page.index ] != GUI_CMD_NULL      ) &&
           ( guiState->page.ptr[ guiState->page.index ]  > GUI_CMD_PAGE_END  )    )
    {
        count-- ;
        if( count == 0 )
        {
            break ;
        }
        guiState->page.index++ ;
    }

}

static void GuiVal_CurrentTime( GuiState_st* guiState );
static void GuiVal_BatteryLevel( GuiState_st* guiState );
static void GuiVal_LockState( GuiState_st* guiState );
static void GuiVal_ModeInfo( GuiState_st* guiState );
static void GuiVal_Frequency( GuiState_st* guiState );
static void GuiVal_RSSIMeter( GuiState_st* guiState );

static void GuiVal_Banks( GuiState_st* guiState );
static void GuiVal_Channels( GuiState_st* guiState );
static void GuiVal_Contacts( GuiState_st* guiState );
static void GuiVal_Gps( GuiState_st* guiState );
    // Settings
    // Display
#ifdef SCREEN_BRIGHTNESS
static void GuiVal_ScreenBrightness( GuiState_st* guiState );
#endif
#ifdef SCREEN_CONTRAST
static void GuiVal_ScreenContrast( GuiState_st* guiState );
#endif
static void GuiVal_Timer( GuiState_st* guiState );
    // Time and Date
static void GuiVal_Date( GuiState_st* guiState );
static void GuiVal_Time( GuiState_st* guiState );
    // GPS
static void GuiVal_GpsEnables( GuiState_st* guiState );
static void GuiVal_GpsSetTime( GuiState_st* guiState );
static void GuiVal_GpsTimeZone( GuiState_st* guiState );
    // Radio
static void GuiVal_RadioOffset( GuiState_st* guiState );
static void GuiVal_RadioDirection( GuiState_st* guiState );
static void GuiVal_RadioStep( GuiState_st* guiState );
    // M17
static void GuiVal_M17Callsign( GuiState_st* guiState );
static void GuiVal_M17Can( GuiState_st* guiState );
static void GuiVal_M17CanRxCheck( GuiState_st* guiState );
    // Accessibility - Voice
static void GuiVal_Voice( GuiState_st* guiState );
static void GuiVal_Phonetic( GuiState_st* guiState );
    // Info
static void GuiVal_BatteryVoltage( GuiState_st* guiState );
static void GuiVal_BatteryCharge( GuiState_st* guiState );
static void GuiVal_Rssi( GuiState_st* guiState );
static void GuiVal_UsedHeap( GuiState_st* guiState );
static void GuiVal_Band( GuiState_st* guiState );
static void GuiVal_Vhf( GuiState_st* guiState );
static void GuiVal_Uhf( GuiState_st* guiState );
static void GuiVal_HwVersion( GuiState_st* guiState );
#ifdef PLATFORM_TTWRPLUS
static void GuiVal_Radio( GuiState_st* guiState );
static void GuiVal_RadioFw( GuiState_st* guiState );
#endif // PLATFORM_TTWRPLUS
static void GuiVal_Stubbed( GuiState_st* guiState );

static void GuiVal_DisplayValue( GuiState_st* guiState , char* valueBuffer );

typedef void (*ui_GuiVal_fn)( GuiState_st* guiState );

static const ui_GuiVal_fn ui_GuiVal_Table[ GUI_VAL_NUM_OF ] =
{
    GuiVal_CurrentTime      ,
    GuiVal_BatteryLevel     ,
    GuiVal_LockState        ,
    GuiVal_ModeInfo         ,
    GuiVal_Frequency        ,
    GuiVal_RSSIMeter        ,

    GuiVal_Banks            ,
    GuiVal_Channels         ,
    GuiVal_Contacts         ,
    GuiVal_Gps              ,
    // Settings
    // Display
#ifdef SCREEN_BRIGHTNESS
    GuiVal_ScreenBrightness ,
#endif
#ifdef SCREEN_CONTRAST
    GuiVal_ScreenContrast   ,
#endif
    GuiVal_Timer            ,
    // Time and Date
    GuiVal_Date             ,
    GuiVal_Time             ,
    // GPS
    GuiVal_GpsEnables       ,
    GuiVal_GpsSetTime       ,
    GuiVal_GpsTimeZone      ,
    // Radio
    GuiVal_RadioOffset      ,
    GuiVal_RadioDirection   ,
    GuiVal_RadioStep        ,
    // M17
    GuiVal_M17Callsign      ,
    GuiVal_M17Can           ,
    GuiVal_M17CanRxCheck    ,
    // Accessibility - Voice
    GuiVal_Voice            ,
    GuiVal_Phonetic         ,
    // Info
    GuiVal_BatteryVoltage   ,
    GuiVal_BatteryCharge    ,
    GuiVal_Rssi             ,
    GuiVal_UsedHeap         ,
    GuiVal_Band             ,
    GuiVal_Vhf              ,
    GuiVal_Uhf              ,
    GuiVal_HwVersion        ,
#ifdef PLATFORM_TTWRPLUS
    GuiVal_Radio            ,
    GuiVal_RadioFw          ,
#endif // PLATFORM_TTWRPLUS

    GuiVal_Stubbed
};

static void GuiCmd_DisplayValue( GuiState_st* guiState , uint8_t valueNum )
{
    ui_GuiVal_Table[ valueNum ]( guiState );
}

static void GuiVal_CurrentTime( GuiState_st* guiState )
{
    uint8_t  lineIndex ;
    uint16_t height ;
    uint16_t width ;
    Pos_st   start ;
    Color_st color_bg ;
    Color_st color_fg ;
    uiColorLoad( &color_bg , COLOR_BG );
    uiColorLoad( &color_fg , COLOR_FG );

#ifdef RTC_PRESENT
    //@@@KL needs to be more objectively determined
    lineIndex = GUI_LINE_TOP ;
    height    = GuiState.layout.lineStyle[ lineIndex ].height ;
    width     = 68 ;
    start.y   = ( GuiState.layout.lineStyle[ lineIndex ].pos.y - height ) + 1 ;
    start.x   = 44 ;

    // clear the time display area
    gfx_drawRect( start , width , height , color_bg , true );

    // Print clock on top bar
    datetime_t local_time = utcToLocalTime( last_state.time ,
                                            last_state.settings.utc_timezone );
    gfx_print( guiState->layout.lineStyle[ GUI_LINE_TOP ].pos , guiState->layout.lineStyle[ GUI_LINE_TOP ].font.size , ALIGN_CENTER ,
               color_fg , "%02d:%02d:%02d" , local_time.hour ,
               local_time.minute , local_time.second );
#endif // RTC_PRESENT

}

static void GuiVal_BatteryLevel( GuiState_st* guiState )
{
    Color_st color_bg ;
    Color_st color_fg ;
    uiColorLoad( &color_bg , COLOR_BG );
    uiColorLoad( &color_fg , COLOR_FG );

    // If the radio has no built-in battery, print input voltage
#ifdef BAT_NONE
    gfx_print( guiState->layout.lineStyle[ GUI_LINE_TOP ].pos , guiState->layout.lineStyle[ GUI_LINE_TOP ].font.size , ALIGN_RIGHT ,
               color_fg , "%.1fV" , last_state.v_bat );
#else // BAT_NONE
    // Otherwise print battery icon on top bar, use 4 px padding
    uint16_t bat_width  = SCREEN_WIDTH / 9 ;
    uint16_t bat_height = guiState->layout.lineStyle[ GUI_LINE_TOP ].height - ( guiState->layout.status_v_pad * 2 );
    Pos_st   bat_pos    = { SCREEN_WIDTH - bat_width - guiState->layout.horizontal_pad ,
                            guiState->layout.status_v_pad };
    gfx_drawBattery( bat_pos , bat_width , bat_height , last_state.charge );
#endif // BAT_NONE

}

static void GuiVal_LockState( GuiState_st* guiState )
{
    Color_st color_bg ;
    Color_st color_fg ;
    uiColorLoad( &color_bg , COLOR_BG );
    uiColorLoad( &color_fg , COLOR_FG );

    if( guiState->uiState.input_locked == true )
    {
      gfx_drawSymbol( guiState->layout.lineStyle[ GUI_LINE_TOP ].pos ,
                      guiState->layout.lineStyle[ GUI_LINE_TOP ].symbolSize ,
                      ALIGN_LEFT , color_fg , SYMBOL_LOCK );
    }

}

static void GuiVal_ModeInfo( GuiState_st* guiState )
{
    char bw_str[ 8 ]     = { 0 };
    char encdec_str[ 9 ] = { 0 };
    Color_st color_fg ;
    uiColorLoad( &color_fg , COLOR_FG );

    switch( last_state.channel.mode )
    {
        case OPMODE_FM :
        {
            // Get Bandwidth string
            if( last_state.channel.bandwidth == BW_12_5 )
            {
                snprintf( bw_str , 8 , "NFM" );
            }
            else
            {
                if( last_state.channel.bandwidth == BW_20 )
                {
                    snprintf( bw_str , 8 , "FM20" );
                }
                else
                {
                    if( last_state.channel.bandwidth == BW_25 )
                    {
                        snprintf( bw_str , 8 , "FM" );
                    }
                }
            }

            // Get encdec string
            bool tone_tx_enable = last_state.channel.fm.txToneEn ;
            bool tone_rx_enable = last_state.channel.fm.rxToneEn ;

            if( tone_tx_enable && tone_rx_enable )
            {
                snprintf( encdec_str , 9 , "ED" );
            }
            else
            {
                if( tone_tx_enable && !tone_rx_enable )
                {
                    snprintf( encdec_str , 9 , " E" );
                }
                else
                {
                    if( !tone_tx_enable && tone_rx_enable )
                    {
                        snprintf( encdec_str , 9 , " D" );
                    }
                    else
                    {
                        snprintf( encdec_str , 9 , "  " );
                    }
                }
            }

            // Print Bandwidth, Tone and encdec info
            if( tone_tx_enable || tone_rx_enable )
            {
                gfx_print( guiState->layout.lineStyle[ GUI_LINE_2 ].pos , guiState->layout.lineStyle[ GUI_LINE_2 ].font.size , ALIGN_CENTER ,
                           color_fg , "%s %4.1f %s" , bw_str ,
                           ctcss_tone[ last_state.channel.fm.txTone ] / 10.0f , encdec_str );
            }
            else
            {
                gfx_print( guiState->layout.lineStyle[ GUI_LINE_2 ].pos , guiState->layout.lineStyle[ GUI_LINE_2 ].font.size , ALIGN_CENTER ,
                           color_fg , "%s" , bw_str );
            }
            break ;
        }
        case OPMODE_DMR :
        {
            // Print Contact
            gfx_print( guiState->layout.lineStyle[ GUI_LINE_2 ].pos , guiState->layout.lineStyle[ GUI_LINE_2 ].font.size , ALIGN_CENTER ,
                       color_fg , "%s" , last_state.contact.name );
            break ;
        }
        case OPMODE_M17 :
        {
            // Print M17 Destination ID on line 3 of 3
            rtxStatus_t rtxStatus = rtx_getCurrentStatus();

            if( rtxStatus.lsfOk )
            {
                // Destination address
                gfx_drawSymbol( guiState->layout.lineStyle[ GUI_LINE_2 ].pos , guiState->layout.lineStyle[ GUI_LINE_2 ].symbolSize , ALIGN_LEFT ,
                                color_fg , SYMBOL_CALL_RECEIVED );

                gfx_print( guiState->layout.lineStyle[ GUI_LINE_2 ].pos , guiState->layout.lineStyle[ GUI_LINE_2 ].font.size , ALIGN_CENTER ,
                           color_fg , "%s" , rtxStatus.M17_dst );

                // Source address
                gfx_drawSymbol( guiState->layout.lineStyle[ GUI_LINE_1 ].pos , guiState->layout.lineStyle[ GUI_LINE_1 ].symbolSize , ALIGN_LEFT ,
                                color_fg , SYMBOL_CALL_MADE );

                gfx_print( guiState->layout.lineStyle[ GUI_LINE_1 ].pos , guiState->layout.lineStyle[ GUI_LINE_2 ].font.size , ALIGN_CENTER ,
                           color_fg , "%s" , rtxStatus.M17_src );

                // RF link (if present)
                if( rtxStatus.M17_link[0] != '\0' )
                {
                    gfx_drawSymbol( guiState->layout.lineStyle[ GUI_LINE_4 ].pos , guiState->layout.lineStyle[ GUI_LINE_3 ].symbolSize , ALIGN_LEFT ,
                                    color_fg , SYMBOL_ACCESS_POINT );
                    gfx_print( guiState->layout.lineStyle[ GUI_LINE_4 ].pos , guiState->layout.lineStyle[ GUI_LINE_2 ].font.size , ALIGN_CENTER ,
                               color_fg , "%s" , rtxStatus.M17_link );
                }

                // Reflector (if present)
                if( rtxStatus.M17_refl[0] != '\0' )
                {
                    gfx_drawSymbol( guiState->layout.lineStyle[ GUI_LINE_3 ].pos , guiState->layout.lineStyle[ GUI_LINE_4 ].symbolSize , ALIGN_LEFT ,
                                    color_fg , SYMBOL_NETWORK );
                    gfx_print( guiState->layout.lineStyle[ GUI_LINE_3 ].pos , guiState->layout.lineStyle[ GUI_LINE_2 ].font.size , ALIGN_CENTER ,
                               color_fg , "%s" , rtxStatus.M17_refl );
                }
            }
            else
            {
                const char* dst = NULL ;
                if( guiState->uiState.edit_mode )
                {
                    dst = guiState->uiState.new_callsign ;
                }
                else
                {
                    if( strnlen( rtxStatus.destination_address , 10 ) == 0 )
                    {
                        dst = currentLanguage->broadcast ;
                    }
                    else
                    {
                        dst = rtxStatus.destination_address ;
                    }
                }

                gfx_print( guiState->layout.lineStyle[ GUI_LINE_2 ].pos , guiState->layout.lineStyle[ GUI_LINE_2 ].font.size , ALIGN_CENTER ,
                           color_fg , "M17 #%s" , dst );
            }
            break ;
        }
    }

}

static void GuiVal_Frequency( GuiState_st* guiState )
{
    // Show VFO frequency if the OpMode is not M17 or there is no valid LSF data
    rtxStatus_t status = rtx_getCurrentStatus();
    if( ( status.opMode != OPMODE_M17 ) || ( status.lsfOk == false ) )
    {
        unsigned long frequency = platform_getPttStatus() ? last_state.channel.tx_frequency
                                                          : last_state.channel.rx_frequency;
        Color_st color_fg ;
        uiColorLoad( &color_fg , COLOR_FG );

        // Print big numbers frequency
        gfx_print( guiState->layout.lineStyle[ GUI_LINE_3_LARGE ].pos , guiState->layout.lineStyle[ GUI_LINE_3_LARGE ].font.size , ALIGN_CENTER ,
                   color_fg , "%.7g" , (float)frequency / 1000000.0f );
    }
}

static void GuiVal_RSSIMeter( GuiState_st* guiState )
{
    // Squelch bar
    float    rssi         = last_state.rssi ;
    float    squelch      = last_state.settings.sqlLevel / 16.0f ;
    uint16_t meter_width  = SCREEN_WIDTH - ( 2 * guiState->layout.horizontal_pad ) ;
    uint16_t meter_height = guiState->layout.lineStyle[ GUI_LINE_BOTTOM ].height ;
    Pos_st   meter_pos    = { guiState->layout.horizontal_pad , ( SCREEN_HEIGHT - meter_height ) - guiState->layout.bottom_pad };
    uint8_t  mic_level    = platform_getMicLevel();
    Color_st color_op3 ;

    uiColorLoad( &color_op3 , COLOR_OP3 );

    switch( last_state.channel.mode )
    {
        case OPMODE_FM :
        {
            gfx_drawSmeter( meter_pos , meter_width , meter_height ,
                            rssi , squelch , color_op3 );
            break ;
        }
        case OPMODE_DMR :
        {
            gfx_drawSmeter( meter_pos , meter_width , meter_height ,
                            rssi , squelch , color_op3 );
            break ;
        }
        case OPMODE_M17 :
        {
            gfx_drawSmeterLevel( meter_pos , meter_width , meter_height ,
                                 rssi , mic_level );
            break ;
        }
    }

}

static void GuiVal_Banks( GuiState_st* guiState )
{
    (void)guiState ;

//    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

//            snprintf( valueBuffer , valueBufferSize , "%s" ,
//                      ... );
//    GuiVal_DisplayValue( guiState , valueBuffer );
}

static void GuiVal_Channels( GuiState_st* guiState )
{
    (void)guiState ;

//    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

//            snprintf( valueBuffer , valueBufferSize , "%s" ,
//                      ... );
//    GuiVal_DisplayValue( guiState , valueBuffer );
}

static void GuiVal_Contacts( GuiState_st* guiState )
{
    (void)guiState ;

//    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

//            snprintf( valueBuffer , valueBufferSize , "%s" ,
//                      ... );
//    GuiVal_DisplayValue( guiState , valueBuffer );
}

static void GuiVal_Gps( GuiState_st* guiState )
{
    (void)guiState ;

//    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

//            snprintf( valueBuffer , valueBufferSize , "%s" ,
//                      ... );
//    GuiVal_DisplayValue( guiState , valueBuffer );
}
// Settings
// Display

#ifdef SCREEN_BRIGHTNESS
static void GuiVal_ScreenBrightness( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , last_state.settings.brightness );
    GuiVal_DisplayValue( guiState , valueBuffer );

}
#endif
#ifdef SCREEN_CONTRAST
static void GuiVal_ScreenContrast( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , last_state.settings.contrast );
    GuiVal_DisplayValue( guiState , valueBuffer );

}
#endif
static void GuiVal_Timer( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              display_timer_values[ last_state.settings.display_timer ] );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_Date( GuiState_st* guiState )
{
    char       valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    datetime_t local_time = utcToLocalTime( last_state.time , last_state.settings.utc_timezone );

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%02d/%02d/%02d" ,
              local_time.date , local_time.month , local_time.year );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_Time( GuiState_st* guiState )
{
    char       valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    datetime_t local_time = utcToLocalTime( last_state.time , last_state.settings.utc_timezone );

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%02d:%02d:%02d" ,
              local_time.hour , local_time.minute , local_time.second );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_GpsEnables( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              (last_state.settings.gps_enabled) ? currentLanguage->on : currentLanguage->off );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_GpsSetTime( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              (last_state.gps_set_time) ? currentLanguage->on : currentLanguage->off );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_GpsTimeZone( GuiState_st* guiState )
{
    char   valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    int8_t tz_hr = ( last_state.settings.utc_timezone / 2 ) ;
    int8_t tz_mn = ( last_state.settings.utc_timezone % 2 ) * 5 ;
    char   sign  = ' ';

    if(last_state.settings.utc_timezone > 0)
    {
        sign = '+' ;
    }
    else if(last_state.settings.utc_timezone < 0)
    {
        sign   = '-' ;
        tz_hr *= (-1) ;
        tz_mn *= (-1) ;
    }

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%c%d.%d" , sign , tz_hr , tz_mn );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

    // Radio
static void GuiVal_RadioOffset( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    int32_t offset = 0 ;

    offset = abs( (int32_t)last_state.channel.tx_frequency -
                  (int32_t)last_state.channel.rx_frequency );
    snprintf( valueBuffer , MAX_ENTRY_LEN , "%gMHz" , (float)offset / 1000000.0f );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_RadioDirection( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    valueBuffer[ 0 ] = ( last_state.channel.tx_frequency >= last_state.channel.rx_frequency ) ? '+' : '-';
    valueBuffer[ 1 ] = '\0';
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_RadioStep( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    // Print in kHz if it is smaller than 1MHz
    if( freq_steps[ last_state.step_index ] < 1000000 )
    {
        snprintf( valueBuffer , MAX_ENTRY_LEN , "%gkHz" , (float)freq_steps[last_state.step_index] / 1000.0f );
    }
    else
    {
        snprintf( valueBuffer , MAX_ENTRY_LEN , "%gMHz" , (float)freq_steps[last_state.step_index] / 1000000.0f );
    }
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_M17Callsign( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" , last_state.settings.callsign );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_M17Can( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , last_state.settings.m17_can );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_M17CanRxCheck( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              (last_state.settings.m17_can_rx) ? currentLanguage->on : currentLanguage->off );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

// Accessibility - Voice
static void GuiVal_Voice( GuiState_st* guiState )
{
    char    valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    uint8_t value = last_state.settings.vpLevel ;

    switch( value )
    {
        case VPP_NONE :
        {
            snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
                      currentLanguage->off );
            break ;
        }
        case VPP_BEEP :
        {
            snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
                      currentLanguage->beep );
            break ;
        }
        default :
        {
            snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" ,
                      ( value - VPP_BEEP ) );
            break ;
        }
    }
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_Phonetic( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              last_state.settings.vpPhoneticSpell ? currentLanguage->on : currentLanguage->off );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

// Info
static void GuiVal_BatteryVoltage( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    // Compute integer part and mantissa of voltage value, adding 50mV
    // to mantissa for rounding to nearest integer
    uint16_t volt  = ( last_state.v_bat + 50 ) / 1000 ;
    uint16_t mvolt = ( ( last_state.v_bat - volt * 1000 ) + 50 ) / 100 ;
    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d.%dV" , volt, mvolt );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_BatteryCharge( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d%%" , last_state.charge );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_Rssi( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%.1fdBm" , last_state.rssi );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_UsedHeap( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%dB" , getHeapSize() - getCurrentFreeHeap() );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_Band( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s %s" , hwinfo->vhf_band ? currentLanguage->VHF : "" , hwinfo->uhf_band ? currentLanguage->UHF : "" );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_Vhf( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d - %d" , hwinfo->vhf_minFreq, hwinfo->vhf_maxFreq );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_Uhf( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d - %d" , hwinfo->uhf_minFreq, hwinfo->uhf_maxFreq );
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_HwVersion( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();
    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , hwinfo->hw_version );

    GuiVal_DisplayValue( guiState , valueBuffer );

}

#ifdef PLATFORM_TTWRPLUS
static void GuiVal_Radio( GuiState_st* guiState )
{
    (void)guiState ;
    //@@@KL Populate
//    GuiVal_DisplayValue( guiState , valueBuffer );
}

static void GuiVal_RadioFw( GuiState_st* guiState )
{
    (void)guiState ;
    //@@@KL Populate
//    GuiVal_DisplayValue( guiState , valueBuffer );
}
#endif // PLATFORM_TTWRPLUS

// Default
static void GuiVal_Stubbed( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] ;

    valueBuffer[ 0 ] = '?' ;
    valueBuffer[ 1 ] = '\0' ;
    GuiVal_DisplayValue( guiState , valueBuffer );

}

static void GuiVal_DisplayValue( GuiState_st* guiState , char* valueBuffer )
{
    Color_st color_fg ;
    Color_st color_bg ;
    Color_st color_text ;

    uiColorLoad( &color_fg , COLOR_FG );
    uiColorLoad( &color_bg , COLOR_BG );
    color_text = color_fg ;

    if( guiState->layout.printDisplayOn )
    {
        if( guiState->layout.inSelect )
        {
            color_text = color_bg ;
        }
        gfx_print( guiState->layout.line.pos       ,
                   guiState->layout.line.font.size ,
                   guiState->layout.line.align     ,
                   color_text , valueBuffer );
    }

}

       State_st    last_state ;
       bool        macro_latched ;
static bool        macro_menu      = false ;
static bool        redraw_needed   = true ;

static bool        standby         = false ;
static long long   last_event_tick = 0 ;

// UI event queue
static uint8_t     evQueue_rdPos ;
static uint8_t     evQueue_wrPos ;
static Event_st    evQueue[ MAX_NUM_EVENTS ] ;

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
    SCREEN_INITIAL_ALIGN       	  =  ALIGN_LEFT ,
    // Height and padding shown in diagram at beginning of file
    SCREEN_INITIAL_X              =  0 ,
    SCREEN_INITIAL_Y              =  0 ,
    SCREEN_INITIAL_HEIGHT         = 20 ,
    SCREEN_TOP_ALIGN              = ALIGN_CENTER ,
    SCREEN_TOP_HEIGHT             = 16 ,
    SCREEN_TOP_PAD                =  4 ,
    SCREEN_LINE_ALIGN             =  ALIGN_LEFT ,
    SCREEN_LINE_1_HEIGHT          = 20 ,
    SCREEN_LINE_2_HEIGHT          = 20 ,
    SCREEN_LINE_3_HEIGHT          = 20 ,
    SCREEN_LINE_3_LARGE_HEIGHT    = 40 ,
    SCREEN_LINE_4_HEIGHT          = 20 ,
    SCREEN_MENU_HEIGHT            = 16 ,
    SCREEN_BOTTOM_ALIGN           =  ALIGN_LEFT ,
    SCREEN_BOTTOM_HEIGHT          = 23 ,
    SCREEN_BOTTOM_PAD             = SCREEN_TOP_PAD ,
    SCREEN_STATUS_V_PAD           =  2 ,
    SCREEN_SMALL_LINE_V_PAD       =  2 ,
    SCREEN_BIG_LINE_V_PAD         =  6 ,
    SCREEN_HORIZONTAL_PAD         =  4 ,
    SCREEN_INITIAL_FONT_SIZE      = FONT_SIZE_8PT     ,
    SCREEN_INITIAL_SYMBOL_SIZE    = SYMBOLS_SIZE_8PT  ,
    // Top bar font: 8 pt
    SCREEN_TOP_FONT_SIZE          = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_TOP_SYMBOL_SIZE        = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    // Text line font: 8 pt
    SCREEN_LINE_1_FONT_SIZE       = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_LINE_1_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    SCREEN_LINE_2_FONT_SIZE       = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_LINE_2_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    SCREEN_LINE_3_FONT_SIZE       = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_LINE_3_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    SCREEN_LINE_4_FONT_SIZE       = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_LINE_4_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    // Frequency line font: 16 pt
    SCREEN_LINE_3_LARGE_FONT_SIZE = FONT_SIZE_16PT    , // FontSize_t
    // Bottom bar font: 8 pt
    SCREEN_BOTTOM_FONT_SIZE       = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_BOTTOM_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    // TimeDate/Frequency input font
    SCREEN_INPUT_FONT_SIZE        = FONT_SIZE_12PT    , // FontSize_t
    // Menu font
    SCREEN_MENU_FONT_SIZE         = FONT_SIZE_8PT     , // FontSize_t
    // Mode screen frequency font: 12 pt
    SCREEN_MODE_FONT_SIZE_BIG     = FONT_SIZE_12PT    , // FontSize_t
    // Mode screen details font: 9 pt
    SCREEN_MODE_FONT_SIZE_SMALL   = FONT_SIZE_9PT       // FontSize_t
};

// Radioddity GD-77
#elif SCREEN_HEIGHT > 63
enum
{
    SCREEN_INITIAL_ALIGN          = ALIGN_LEFT ,
    // Height and padding shown in diagram at beginning of file
    SCREEN_INITIAL_X              =  0 ,
    SCREEN_INITIAL_Y              =  0 ,
    SCREEN_INITIAL_HEIGHT         = 10 ,
    SCREEN_TOP_ALIGN              = ALIGN_CENTER ,
    SCREEN_TOP_HEIGHT             = 11 ,
    SCREEN_TOP_PAD                =  1 ,
    SCREEN_LINE_ALIGN             = ALIGN_LEFT ,
    SCREEN_LINE_1_HEIGHT          = 10 ,
    SCREEN_LINE_2_HEIGHT          = 10 ,
    SCREEN_LINE_3_HEIGHT          = 10 ,
    SCREEN_LINE_3_LARGE_HEIGHT    = 16 ,
    SCREEN_LINE_4_HEIGHT          = 10 ,
    SCREEN_MENU_HEIGHT            = 10 ,
    SCREEN_BOTTOM_ALIGN           = ALIGN_LEFT ,
    SCREEN_BOTTOM_HEIGHT          = 15 ,
    SCREEN_BOTTOM_PAD             =  0 ,
    SCREEN_STATUS_V_PAD           =  1 ,
    SCREEN_SMALL_LINE_V_PAD       =  1 ,
    SCREEN_BIG_LINE_V_PAD         =  0 ,
    SCREEN_HORIZONTAL_PAD         =  4 ,
    SCREEN_INITIAL_FONT_SIZE      = FONT_SIZE_6PT     ,
    SCREEN_INITIAL_SYMBOL_SIZE    = SYMBOLS_SIZE_6PT  ,
    // Top bar font: 6 pt
    SCREEN_TOP_FONT_SIZE          = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_TOP_SYMBOL_SIZE        = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    // Middle line fonts: 5, 8, 8 pt
    SCREEN_LINE_1_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_1_SYMBOL_SIZE     = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    SCREEN_LINE_2_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_2_SYMBOL_SIZE     = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    SCREEN_LINE_3_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_3_SYMBOL_SIZE     = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    SCREEN_LINE_3_LARGE_FONT_SIZE = FONT_SIZE_10PT    , // FontSize_t
    SCREEN_LINE_4_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_4_SYMBOL_SIZE     = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    // Bottom bar font: 6 pt
    SCREEN_BOTTOM_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_BOTTOM_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    // TimeDate/Frequency input font
    SCREEN_INPUT_FONT_SIZE        = FONT_SIZE_8PT     , // FontSize_t
    // Menu font
    SCREEN_MENU_FONT_SIZE         = FONT_SIZE_6PT     , // FontSize_t
    // Mode screen frequency font: 9 pt
    SCREEN_MODE_FONT_SIZE_BIG     = FONT_SIZE_9PT     , // FontSize_t
    // Mode screen details font: 6 pt
    SCREEN_MODE_FONT_SIZE_SMALL   = FONT_SIZE_6PT       // FontSize_t
};

// Radioddity RD-5R
#elif SCREEN_HEIGHT > 47
enum
{
    SCREEN_INITIAL_ALIGN          = ALIGN_LEFT ,
    // Height and padding shown in diagram at beginning of file
    SCREEN_INITIAL_HEIGHT         = 10 ,
    // Height and padding shown in diagram at beginning of file
    SCREEN_TOP_ALIGN              = ALIGN_CENTER ,
    SCREEN_TOP_HEIGHT             = 11 ,
    SCREEN_TOP_PAD                =  1 ,
    SCREEN_LINE_ALIGN             = ALIGN_LEFT ,
    SCREEN_LINE_1_HEIGHT          =  0 ,
    SCREEN_LINE_2_HEIGHT          = 10 ,
    SCREEN_LINE_3_HEIGHT          = 10 ,
    SCREEN_LINE_3_LARGE_HEIGHT    = 18 ,
    SCREEN_LINE_4_HEIGHT          = 10 ,
    SCREEN_MENU_HEIGHT            = 10 ,
    SCREEN_BOTTOM_ALIGN           = ALIGN_LEFT ,
    SCREEN_BOTTOM_HEIGHT          =  0 ,
    SCREEN_BOTTOM_PAD             =  0 ,
    SCREEN_STATUS_V_PAD           =  1 ,
    SCREEN_SMALL_LINE_V_PAD       =  1 ,
    SCREEN_BIG_LINE_V_PAD         =  0 ,
    SCREEN_HORIZONTAL_PAD         =  4 ,
    SCREEN_INITIAL_FONT_SIZE      = FONT_SIZE_6PT     ,
    SCREEN_INITIAL_SYMBOL_SIZE    = SYMBOLS_SIZE_6PT  ,
    // Top bar font: 6 pt
    SCREEN_TOP_FONT_SIZE          = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_TOP_SYMBOL_SIZE        = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    // Middle line fonts: 16, 16
    SCREEN_LINE_2_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_3_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_4_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_3_LARGE_FONT_SIZE = FONT_SIZE_12PT    , // FontSize_t
    // TimeDate/Frequency input font
    SCREEN_INPUT_FONT_SIZE        = FONT_SIZE_8PT     , // FontSize_t
    // Menu font
    SCREEN_MENU_FONT_SIZE         = FONT_SIZE_6PT     , // FontSize_t
    // Mode screen frequency font: 9 pt
    SCREEN_MODE_FONT_SIZE_BIG     = FONT_SIZE_9PT     , // FontSize_t
    // Mode screen details font: 6 pt
    SCREEN_MODE_FONT_SIZE_SMALL   = FONT_SIZE_6PT     , // FontSize_t
    // Not present on this resolution
    SCREEN_LINE_1_FONT_SIZE       =  0                , // FontSize_t
    SCREEN_BOTTOM_FONT_SIZE       =  0                , // FontSize_t
    SCREEN_BOTTOM_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT    // SymbolSize_t
};
#else
    #error Unsupported vertical resolution!
#endif

static bool ui_updateFSM_PAGE_MAIN_VFO( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MAIN_VFO_INPUT( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MAIN_MEM( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MODE_VFO( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MODE_MEM( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_TOP( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_BANK( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_CHANNEL( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_CONTACTS( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_GPS( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_SETTINGS( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_BACKUP_RESTORE( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_BACKUP( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_RESTORE( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_INFO( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_ABOUT( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE_SET( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_DISPLAY( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_GPS( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_RADIO( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_M17( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_VOICE( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_RESET_TO_DEFAULTS( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_LOW_BAT( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_ABOUT( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_STUBBED( GuiState_st* guiState );

typedef bool (*ui_updateFSM_PAGE_fn)( GuiState_st* guiState );

static const ui_updateFSM_PAGE_fn ui_updateFSM_PageTable[ PAGE_NUM_OF ] =
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
    ui_updateFSM_PAGE_ABOUT                      ,
    ui_updateFSM_PAGE_STUBBED
};

uint8_t uiGetPageNumOf( GuiState_st* guiState )
{
    (void)guiState ;
    uint8_t* ptr ;
    uint16_t index ;
    uint8_t  numOf ;

    ptr = (uint8_t*)uiPageTable[ guiState->page.num ] ;

    for( numOf = 0 , index = 0 ; ptr[ index ] != GUI_CMD_PAGE_END ; index++ )
    {
        if( ptr[ index ] == GUI_CMD_TEXT )
        {
            numOf++ ;
        }
    }

    return numOf ;
}

char* uiGetPageTextString( uiPageNum_en pageNum , uint8_t textStringIndex )
{
    uiPageNum_en pgNum = pageNum ;
    char*        ptr ;
    uint16_t     index ;
    uint8_t      numOf ;

    if( pgNum >= PAGE_NUM_OF )
    {
        pgNum = PAGE_STUBBED ;
    }

    ptr = (char*)uiPageTable[ pgNum ] ;

    for( numOf = 0 , index = 0 ; ptr[ index ] != GUI_CMD_PAGE_END ; index++ )
    {
        if( ptr[ index ] == GUI_CMD_TEXT )
        {
            if( numOf == textStringIndex )
            {
                ptr = &ptr[ index + 1 ] ;
                break ;
            }
            numOf++ ;
        }
    }

    return ptr ;
}

static bool ui_UpdatePage( GuiState_st* guiState )
{
    bool sync_rtx ;

    sync_rtx = ui_updateFSM_PageTable[ guiState->page.num ]( guiState );

    return sync_rtx ;
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
        vp_queueString( "hh:mm" , VP_ANNOUNCE_COMMON_SYMBOLS | VP_ANNOUNCE_LESS_COMMON_SYMBOLS );
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
    Color_st color_agg ;
    uiColorLoad( &color_agg , COLOR_AGG );
    Pos_st origin     = { 0 , 0 };

    gfx_drawRect( origin , SCREEN_WIDTH , SCREEN_HEIGHT , color_agg , true );

    return true;

}

static int _ui_fsm_loadChannel( int16_t channel_index , bool* sync_rtx )
{
    channel_t channel ;
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

    return result ;
}

static int _ui_fsm_loadContact( int16_t contact_index , bool* sync_rtx )
{
    contact_t contact ;
    int       result = 0 ;

    result = cps_readContact( &contact , contact_index );

    // Read successful and contact is valid
    if( result != -1 )
    {
        // Set new contact index
        state.contact_index = contact_index ;
        // Copy contact read to state
        state.contact       = contact ;
        *sync_rtx           = true;
    }

    return result ;
}

static void _ui_fsm_confirmVFOInput( GuiState_st* guiState , bool* sync_rtx )
{
    vp_flush();

    switch( guiState->uiState.input_set )
    {
        case SET_RX :
        {
            // Switch to TX input
            guiState->uiState.input_set      = SET_TX;
            // Reset input position
            guiState->uiState.input_position = 0;
            // announce the rx frequency just confirmed with Enter.
            vp_queueFrequency( guiState->uiState.new_rx_frequency );
            // defer playing till the end.
            // indicate that the user has moved to the tx freq field.
            vp_announceInputReceiveOrTransmit( true , VPQ_DEFAULT );
            break ;
        }
        case SET_TX :
        {
            // Save new frequency setting
            // If TX frequency was not set, TX = RX
            if( guiState->uiState.new_tx_frequency == 0 )
            {
                guiState->uiState.new_tx_frequency = guiState->uiState.new_rx_frequency ;
            }
            // Apply new frequencies if they are valid
            if( _ui_freq_check_limits( guiState->uiState.new_rx_frequency ) &&
                _ui_freq_check_limits(guiState->uiState.new_tx_frequency  )    )
            {
                state.channel.rx_frequency = guiState->uiState.new_rx_frequency ;
                state.channel.tx_frequency = guiState->uiState.new_tx_frequency ;
                *sync_rtx                  = true ;
                // force init to clear any prompts in progress.
                // defer play because play is called at the end of the function
                //due to above freq queuing.
                vp_announceFrequencies( state.channel.rx_frequency ,
                                        state.channel.tx_frequency , VPQ_INIT );
            }
            else
            {
                vp_announceError( VPQ_INIT );
            }

            ui_SetPageNum( guiState , PAGE_MAIN_VFO );
            break ;
        }
    }

    vp_play();
}

static void _ui_fsm_insertVFONumber( GuiState_st* guiState , kbd_msg_t msg , bool* sync_rtx )
{
    // Advance input position
    guiState->uiState.input_position += 1 ;
    // clear any prompts in progress.
    vp_flush() ;
    // Save pressed number to calculate frequency and show in GUI
    guiState->uiState.input_number = input_getPressedNumber( msg );
    // queue the digit just pressed.
    vp_queueInteger( guiState->uiState.input_number );
    // queue  point if user has entered three digits.
    if( guiState->uiState.input_position == 3 )
    {
        vp_queuePrompt( PROMPT_POINT );
    }

    switch( guiState->uiState.input_set )
    {
        case SET_RX :
        {
            if( guiState->uiState.input_position == 1 )
            {
                guiState->uiState.new_rx_frequency = 0 ;
            }
            // Calculate portion of the new RX frequency
            guiState->uiState.new_rx_frequency = _ui_freq_add_digit( guiState->uiState.new_rx_frequency ,
                                                                     guiState->uiState.input_position   ,
                                                                     guiState->uiState.input_number       );
            if( guiState->uiState.input_position >= FREQ_DIGITS )
            {
                // queue the rx freq just completed.
                vp_queueFrequency( guiState->uiState.new_rx_frequency );
                /// now queue tx as user has changed fields.
                vp_queuePrompt( PROMPT_TRANSMIT );
                // Switch to TX input
                guiState->uiState.input_set        = SET_TX ;
                // Reset input position
                guiState->uiState.input_position   = 0 ;
                // Reset TX frequency
                guiState->uiState.new_tx_frequency = 0 ;
            }
            break ;
        }
        case SET_TX :
        {
            if( guiState->uiState.input_position == 1 )
            {
                guiState->uiState.new_tx_frequency = 0 ;
            }
            // Calculate portion of the new TX frequency
            guiState->uiState.new_tx_frequency = _ui_freq_add_digit( guiState->uiState.new_tx_frequency ,
                                                                     guiState->uiState.input_position   ,
                                                                     guiState->uiState.input_number       );
            if( guiState->uiState.input_position >= FREQ_DIGITS )
            {
                // Save both inserted frequencies
                if( _ui_freq_check_limits( guiState->uiState.new_rx_frequency ) &&
                    _ui_freq_check_limits( guiState->uiState.new_tx_frequency )    )
                {
                    state.channel.rx_frequency = guiState->uiState.new_rx_frequency ;
                    state.channel.tx_frequency = guiState->uiState.new_tx_frequency ;
                    *sync_rtx                  = true;
                    // play is called at end.
                    vp_announceFrequencies( state.channel.rx_frequency ,
                                            state.channel.tx_frequency , VPQ_INIT );
                }

                ui_SetPageNum( guiState , PAGE_MAIN_VFO );
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
    if( !( ( ( state.settings.display_timer == TIMER_OFF ) && ( variation < 0 ) ) ||
           ( ( state.settings.display_timer == TIMER_1H  ) && ( variation > 0 ) )    ) )
    {
        state.settings.display_timer += variation ;
    }
}

static inline void _ui_changeM17Can( int variation )
{
    uint8_t can = state.settings.m17_can ;

    state.settings.m17_can = ( can + variation ) % 16 ;

}

static void _ui_changeVoiceLevel( int variation )
{
    if( ( ( state.settings.vpLevel == VPP_NONE ) && ( variation < 0 ) ) ||
        ( ( state.settings.vpLevel == VPP_HIGH ) && ( variation > 0 ) )    )
    {
        return ;
    }

    state.settings.vpLevel += variation ;

    // Force these flags to ensure the changes are spoken for levels 1 through 3.
    VPQueueFlags_en flags = VPQ_INIT                   |
                            VPQ_ADD_SEPARATING_SILENCE |
                            VPQ_PLAY_IMMEDIATELY         ;

    if( !vp_isPlaying() )
    {
        flags |= VPQ_INCLUDE_DESCRIPTIONS ;
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

static void _ui_fsm_menuMacro( GuiState_st* guiState , kbd_msg_t msg , bool* sync_rtx )
{
    // If there is no keyboard left and right select the menu entry to edit
#ifdef UI_NO_KEYBOARD

    switch( msg.keys )
    {
        case KNOB_LEFT :
        {
            guiState->uiState.macro_menu_selected-- ;
            guiState->uiState.macro_menu_selected += 9 ;
            guiState->uiState.macro_menu_selected %= 9 ;
            break ;
        }
        case KNOB_RIGHT :
        {
            guiState->uiState.macro_menu_selected++ ;
            guiState->uiState.macro_menu_selected %= 9 ;
            break ;
        }
        case KEY_ENTER :
        {
            if( !msg.long_press )
            {
                guiState->uiState.input_number = guiState->uiState.macro_menu_selected + 1 ;
            }
            else
            {
                guiState->uiState.input_number = 0 ;
            }
            break ;
        }
        default :
        {
            guiState->uiState.input_number = 0 ;
            break ;
        }
    }
#else // UI_NO_KEYBOARD
    guiState->uiState.input_number      = input_getPressedNumber( msg );
#endif // UI_NO_KEYBOARD
    // CTCSS Encode/Decode Selection
    bool tone_tx_enable        = state.channel.fm.txToneEn ;
    bool tone_rx_enable        = state.channel.fm.rxToneEn ;
    uint8_t tone_flags         = ( tone_tx_enable << 1 ) | tone_rx_enable ;
    VPQueueFlags_en queueFlags = vp_getVoiceLevelQueueFlags();

    switch( guiState->uiState.input_number )
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
                                  queueFlags |VPQ_INCLUDE_DESCRIPTIONS );
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
            if( !guiState->uiState.input_locked )
            {
                guiState->uiState.input_locked = true ;
            }
            else
            {
                guiState->uiState.input_locked = false ;
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

static void _ui_menuUp( GuiState_st* guiState , uint8_t menu_entries )
{
    if( guiState->uiState.menu_selected > 0 )
    {
        guiState->uiState.menu_selected -= 1 ;
    }
    else
    {
        guiState->uiState.menu_selected = menu_entries - 1 ;
    }
    vp_playMenuBeepIfNeeded( guiState->uiState.menu_selected == 0 );
}

static void _ui_menuDown( GuiState_st* guiState , uint8_t menu_entries )
{
    if( guiState->uiState.menu_selected < ( menu_entries - 1 ) )
    {
        guiState->uiState.menu_selected += 1 ;
    }
    else
    {
        guiState->uiState.menu_selected = 0 ;
    }
    vp_playMenuBeepIfNeeded( guiState->uiState.menu_selected == 0 );
}

static void _ui_menuBack( GuiState_st* guiState )
{
    if( guiState->page.level > 0 )
    {
        guiState->page.level-- ;

        if( guiState->uiState.edit_mode )
        {
            guiState->uiState.edit_mode = false ;
        }
        else
        {
            // Return to previous menu
            ui_SetPageNum( guiState , guiState->page.levelList[ guiState->page.level ] );
            // Reset menu selection
            guiState->uiState.menu_selected = 0 ;
            vp_playMenuBeepIfNeeded( true );
        }
    }

}

static void _ui_textInputReset( GuiState_st* guiState , char* buf )
{
    guiState->uiState.input_number   = 0 ;
    guiState->uiState.input_position = 0 ;
    guiState->uiState.input_set      = 0 ;
    guiState->uiState.last_keypress  = 0 ;
    memset( buf , 0 , 9 );
    buf[ 0 ]                = '_';
}

static void _ui_textInputKeypad( GuiState_st* guiState , char* buf , uint8_t max_len , kbd_msg_t msg , bool callsign )
{
    if( guiState->uiState.input_position < max_len )
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
        if( guiState->uiState.last_keypress != 0 )
        {
            // Same key pressed and timeout not expired: cycle over chars of current key
            if( ( guiState->uiState.input_number == num_key                          ) &&
                ( ( now - guiState->uiState.last_keypress ) < input_longPressTimeout )    )
            {
                guiState->uiState.input_set = ( guiState->uiState.input_set + 1 ) % num_symbols ;
            }
            // Different key pressed: save current char and change key
            else
            {
                guiState->uiState.input_position += 1 ;
                guiState->uiState.input_set       = 0 ;
            }
        }
        // Show current character on buffer
        if( callsign )
        {
            buf[ guiState->uiState.input_position ] = symbols_ITU_T_E161_callsign[ num_key ][ guiState->uiState.input_set ];
        }
        else
        {
            buf[ guiState->uiState.input_position ] = symbols_ITU_T_E161[ num_key ][ guiState->uiState.input_set ];
        }
        // Announce the character
        vp_announceInputChar( buf[ guiState->uiState.input_position ] );
        // Update reference values
        guiState->uiState.input_number  = num_key ;
        guiState->uiState.last_keypress = now ;
    }
}

static void _ui_textInputConfirm( GuiState_st* guiState , char* buf )
{
    buf[ guiState->uiState.input_position + 1 ] = '\0' ;
}

static void _ui_textInputDel( GuiState_st* guiState , char* buf )
{
    // announce the char about to be backspaced.
    // Note this assumes editing callsign.
    // If we edit a different buffer which allows the underline char, we may
    // not want to exclude it, but when editing callsign, we do not want to say
    // underline since it means the field is empty.
    if( buf[ guiState->uiState.input_position ] &&
        buf[ guiState->uiState.input_position ] != '_' )
    {
        vp_announceInputChar( buf[ guiState->uiState.input_position ] );
    }

    buf[ guiState->uiState.input_position ] = '\0' ;
    // Move back input cursor
    if( guiState->uiState.input_position > 0 )
    {
        guiState->uiState.input_position--;
    // If we deleted the initial character, reset starting condition
    }
    else
    {
        guiState->uiState.last_keypress = 0 ;
    }
    guiState->uiState.input_set = 0 ;
}

static void _ui_numberInputKeypad( GuiState_st* guiState , uint32_t* num , kbd_msg_t msg )
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
    guiState->uiState.input_number = *num % 10 ;
#else // UI_NO_KEYBOARD
    // Maximum frequency len is uint32_t max value number of decimal digits
    if( guiState->uiState.input_position >= 10 )
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
    guiState->uiState.input_number  = num_key;
#endif // UI_NO_KEYBOARD

    guiState->uiState.last_keypress = now;
}

static void _ui_numberInputDel( GuiState_st* guiState , uint32_t* num )
{
    // announce the digit about to be backspaced.
    vp_announceInputChar( '0' + ( *num % 10 ) );

    // Move back input cursor
    if( guiState->uiState.input_position > 0 )
    {
        guiState->uiState.input_position-- ;
    }
    else
    {
        guiState->uiState.last_keypress = 0 ;
    }
    guiState->uiState.input_set = 0 ;
}

void ui_init( void )
{
    GuiState_st* guiState = &GuiState ;

    ui_InitUIState( &guiState->uiState );
    ui_InitGuiState( guiState );

    last_event_tick  = getTick();
    redraw_needed    = true ;
}

static void ui_InitUIState( UI_State_st* uiState )
{
    uint16_t size ;
    uint16_t index ;

    size = sizeof( UI_State_st );

    for( index = 0 ; index < size ; index++ )
    {
        ((uint8_t*)uiState)[ index ] = 0 ;
    }

}

static void ui_InitGuiState( GuiState_st* guiState )
{
    ui_InitGuiStateEvent( &guiState->event );
    guiState->update        = false ;
    guiState->pageHasEvents = false ;
    guiState->inEventArea   = false ;
    ui_InitGuiStatePage( &guiState->page );
    ui_InitGuiStateLayout( &guiState->layout );
}

static void ui_InitGuiStateEvent( Event_st* event )
{
    event->type    = EVENT_TYPE_NONE ;
    event->payload = 0 ;
}

static void ui_InitGuiStatePage( Page_st* page )
{
    uint16_t index ;

    page->num   = 0 ;

    for( index = 0 ; index < MAX_PAGE_DEPTH ; index++ )
    {
        page->levelList[ index ] = 0 ;
    }

    page->level = 0 ;
    page->ptr   = (uint8_t*)uiPageTable[ 0 ] ;
    page->index = 0 ;
}

static void ui_InitGuiStateLayout( Layout_st* layout )
{
    uint8_t index ;

    layout->hline_h                                  = SCREEN_HLINE_H ;
    layout->menu_h                                   = SCREEN_MENU_HEIGHT ;
    layout->bottom_pad                               = SCREEN_BOTTOM_PAD ;
    layout->status_v_pad                             = SCREEN_STATUS_V_PAD ;
    layout->horizontal_pad                           = SCREEN_HORIZONTAL_PAD ;
    layout->text_v_offset                            = SCREEN_TEXT_V_OFFSET ;

    layout->line.pos.x              			     = SCREEN_INITIAL_X ;
    layout->line.pos.y              			     = SCREEN_INITIAL_Y ;
    layout->line.height             			     = SCREEN_INITIAL_HEIGHT ;
    layout->line.align                   		     = SCREEN_INITIAL_ALIGN ;
    layout->line.font.size             			     = SCREEN_INITIAL_FONT_SIZE ;
    layout->line.symbolSize         			     = SCREEN_INITIAL_SYMBOL_SIZE ;

    layout->lineStyle[ GUI_LINE_TOP ].pos.x          = SCREEN_HORIZONTAL_PAD ;
    layout->lineStyle[ GUI_LINE_TOP ].pos.y          = SCREEN_TOP_HEIGHT - SCREEN_STATUS_V_PAD - SCREEN_TEXT_V_OFFSET ;
    layout->lineStyle[ GUI_LINE_TOP ].height         = SCREEN_TOP_HEIGHT ;
    layout->lineStyle[ GUI_LINE_TOP ].align          = SCREEN_TOP_ALIGN ;
    layout->lineStyle[ GUI_LINE_TOP ].font.size      = SCREEN_TOP_FONT_SIZE ;
    layout->lineStyle[ GUI_LINE_TOP ].symbolSize     = SCREEN_TOP_SYMBOL_SIZE ;

    layout->lineStyle[ GUI_LINE_1 ].pos.x            = SCREEN_HORIZONTAL_PAD ;
    layout->lineStyle[ GUI_LINE_1 ].pos.y            = layout->lineStyle[ GUI_LINE_TOP ].pos.y + SCREEN_TOP_PAD + SCREEN_LINE_1_HEIGHT ;
    layout->lineStyle[ GUI_LINE_1 ].height           = SCREEN_LINE_1_HEIGHT ;
    layout->lineStyle[ GUI_LINE_1 ].align            = SCREEN_LINE_ALIGN ;
    layout->lineStyle[ GUI_LINE_1 ].font.size        = SCREEN_LINE_1_FONT_SIZE ;
    layout->lineStyle[ GUI_LINE_1 ].symbolSize       = SCREEN_LINE_1_SYMBOL_SIZE ;

    layout->lineStyle[ GUI_LINE_2 ].pos.x            = SCREEN_HORIZONTAL_PAD ;
    layout->lineStyle[ GUI_LINE_2 ].pos.y            = layout->lineStyle[ GUI_LINE_1 ].pos.y + SCREEN_LINE_2_HEIGHT ;
    layout->lineStyle[ GUI_LINE_2 ].height           = SCREEN_LINE_2_HEIGHT ;
    layout->lineStyle[ GUI_LINE_2 ].align            = SCREEN_LINE_ALIGN ;
    layout->lineStyle[ GUI_LINE_2 ].font.size        = SCREEN_LINE_2_FONT_SIZE ;
    layout->lineStyle[ GUI_LINE_2 ].symbolSize       = SCREEN_LINE_2_SYMBOL_SIZE ;

    layout->lineStyle[ GUI_LINE_3 ].pos.x            = SCREEN_HORIZONTAL_PAD ;
    layout->lineStyle[ GUI_LINE_3 ].pos.y            = layout->lineStyle[ GUI_LINE_2 ].pos.y + SCREEN_LINE_3_HEIGHT ;
    layout->lineStyle[ GUI_LINE_3 ].height           = SCREEN_LINE_3_HEIGHT ;
    layout->lineStyle[ GUI_LINE_3 ].align            = SCREEN_LINE_ALIGN ;
    layout->lineStyle[ GUI_LINE_3 ].font.size        = SCREEN_LINE_3_FONT_SIZE ;
    layout->lineStyle[ GUI_LINE_3 ].symbolSize       = SCREEN_LINE_3_SYMBOL_SIZE ;

    layout->lineStyle[ GUI_LINE_3_LARGE ].pos.x      = SCREEN_HORIZONTAL_PAD ;
    layout->lineStyle[ GUI_LINE_3_LARGE ].pos.y      = layout->lineStyle[ GUI_LINE_2 ].pos.y + SCREEN_LINE_3_LARGE_HEIGHT ;
    layout->lineStyle[ GUI_LINE_3_LARGE ].height     = SCREEN_LINE_3_LARGE_HEIGHT ;
    layout->lineStyle[ GUI_LINE_3_LARGE ].align      = SCREEN_LINE_ALIGN ;
    layout->lineStyle[ GUI_LINE_3_LARGE ].font.size  = SCREEN_LINE_3_LARGE_FONT_SIZE ;
    layout->lineStyle[ GUI_LINE_3_LARGE ].symbolSize = SCREEN_LINE_3_SYMBOL_SIZE ;

    layout->lineStyle[ GUI_LINE_4 ].pos.x            = SCREEN_HORIZONTAL_PAD ;
    layout->lineStyle[ GUI_LINE_4 ].pos.y            = layout->lineStyle[ GUI_LINE_3 ].pos.y + SCREEN_LINE_4_HEIGHT ;
    layout->lineStyle[ GUI_LINE_4 ].height           = SCREEN_LINE_4_HEIGHT ;
    layout->lineStyle[ GUI_LINE_4 ].align            = SCREEN_LINE_ALIGN ;
    layout->lineStyle[ GUI_LINE_4 ].font.size        = SCREEN_LINE_4_FONT_SIZE ;
    layout->lineStyle[ GUI_LINE_4 ].symbolSize       = SCREEN_LINE_4_SYMBOL_SIZE ;

    layout->lineStyle[ GUI_LINE_BOTTOM ].pos.x       = SCREEN_HORIZONTAL_PAD ;
    layout->lineStyle[ GUI_LINE_BOTTOM ].pos.y       = SCREEN_HEIGHT - SCREEN_BOTTOM_PAD - SCREEN_STATUS_V_PAD - SCREEN_TEXT_V_OFFSET ;
    layout->lineStyle[ GUI_LINE_BOTTOM ].height      = SCREEN_BOTTOM_HEIGHT ;
    layout->lineStyle[ GUI_LINE_BOTTOM ].align       = SCREEN_LINE_ALIGN ;
    layout->lineStyle[ GUI_LINE_BOTTOM ].font.size   = SCREEN_BOTTOM_FONT_SIZE ;
    layout->lineStyle[ GUI_LINE_BOTTOM ].symbolSize  = SCREEN_BOTTOM_SYMBOL_SIZE ;

    layout->input_font.size                          = SCREEN_INPUT_FONT_SIZE ;
    layout->menu_font.size                           = SCREEN_MENU_FONT_SIZE ;
    layout->mode_font_big.size                       = SCREEN_MODE_FONT_SIZE_BIG ;
    layout->mode_font_small.size                     = SCREEN_MODE_FONT_SIZE_SMALL ;

    layout->printDisplayOn                           = true ;
    layout->inSelect                                 = false ;

    for( index = 0 ; index < LINK_MAX_NUM_OF ; index++ )
    {
        layout->links[ index ].type = LINK_TYPE_NONE ;
        layout->links[ index ].num  = 0 ;
        layout->links[ index ].amt  = 0 ;
    }
    layout->linkNumOf = 0 ;
    layout->linkIndex = 0 ;

}

void ui_drawSplashScreen( void )
{
    Pos_st logo_pos ;
    Pos_st call_pos ;
    Font_st logo_font ;
    Font_st call_font ;
    Color_st color_fg ;
    Color_st color_op3 ;

    uiColorLoad( &color_fg , COLOR_FG );
    uiColorLoad( &color_op3 , COLOR_OP3 );

    gfx_clearScreen();

    #if SCREEN_HEIGHT > 64
    logo_pos.x     = 0 ;
    logo_pos.y     = ( SCREEN_HEIGHT / 2 ) ;
    call_pos.x     = 0 ;
    call_pos.y     = SCREEN_HEIGHT - 8 ;
    logo_font.size = FONT_SIZE_12PT ;
    call_font.size = FONT_SIZE_8PT ;
    #else
    logo_pos.x     = 0 ;
    logo_pos.y     = 19 ;
    call_pos.x     = 0 ;
    call_pos.y     = SCREEN_HEIGHT - 8 ;
    logo_font.size = FONT_SIZE_8PT ;
    call_font.size = FONT_SIZE_6PT ;
    #endif

    gfx_print( logo_pos , logo_font.size , ALIGN_CENTER , color_op3 , "O P N\nR T X" );
    gfx_print( call_pos , call_font.size , ALIGN_CENTER , color_fg  , state.settings.callsign );

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

static VPGPSInfoFlags_t GetGPSDirectionOrSpeedChanged( void )
{
    uint32_t         now ;
    VPGPSInfoFlags_t whatChanged ;
    float            speedDiff ;
    float            altitudeDiff ;
    float            degreeDiff ;

    if( !state.settings.gps_enabled )
    {
        return VPGPS_NONE;
    }

    now = getTick();

    if( ( now - vpGPSLastUpdate ) < 8000 )
    {
        return VPGPS_NONE;
    }

    whatChanged = VPGPS_NONE ;

    if( state.gps_data.fix_quality != priorGPSFixQuality )
    {
        whatChanged        |= VPGPS_FIX_QUALITY ;
        priorGPSFixQuality  = state.gps_data.fix_quality ;
    }

    if( state.gps_data.fix_type != priorGPSFixType )
    {
        whatChanged     |= VPGPS_FIX_TYPE ;
        priorGPSFixType  = state.gps_data.fix_type ;
    }

    speedDiff=fabs( state.gps_data.speed - priorGPSSpeed );
    if( speedDiff >= 1 )
    {
        whatChanged   |= VPGPS_SPEED ;
        priorGPSSpeed  = state.gps_data.speed ;
    }

    altitudeDiff = fabs( state.gps_data.altitude - priorGPSAltitude );

    if( altitudeDiff >= 5 )
    {
        whatChanged      |= VPGPS_ALTITUDE ;
        priorGPSAltitude  = state.gps_data.altitude ;
    }

    degreeDiff = fabs( state.gps_data.tmg_true - priorGPSDirection );

    if( degreeDiff  >= 1 )
    {
        whatChanged       |= VPGPS_DIRECTION ;
        priorGPSDirection  = state.gps_data.tmg_true ;
    }

    if( state.gps_data.satellites_in_view != priorSatellitesInView )
    {
        whatChanged           |= VPGPS_SAT_COUNT ;
        priorSatellitesInView  = state.gps_data.satellites_in_view ;
    }

    if( whatChanged )
    {
        vpGPSLastUpdate = now ;
    }

    return whatChanged ;
}
#endif // GPS_PRESENT

void ui_updateFSM( bool* sync_rtx , Event_st* event )
{
    GuiState_st* guiState     = &GuiState ;
    bool         processEvent = false ;

    if( event->type )
    {
        processEvent = true ;
    }

    if( processEvent )
    {
        // Check if battery has enough charge to operate.
        // Check is skipped if there is an ongoing transmission, since the voltage
        // drop caused by the RF PA power absorption causes spurious triggers of
        // the low battery alert.
        bool txOngoing = platform_getPttStatus();
#if !defined(PLATFORM_TTWRPLUS)
        if( !state.emergency && !txOngoing && ( state.charge <= 0 ) )
        {
            ui_SetPageNum( guiState , PAGE_LOW_BAT );
            if( ( event->type == EVENT_TYPE_KBD ) && event->payload )
            {
                ui_SetPageNum( guiState , PAGE_MAIN_VFO );
                state.emergency = true ;
            }
            processEvent = false ;
        }
#endif // PLATFORM_TTWRPLUS

        long long timeTick = timeTick = getTick();
        switch( event->type )
        {
            // Process pressed keys
            case EVENT_TYPE_KBD :
            {
                guiState->msg.value  = event->payload ;
                guiState->f1Handled  = false ;
                guiState->queueFlags = vp_getVoiceLevelQueueFlags();
                // If we get out of standby, we ignore the kdb event
                // unless is the MONI key for the MACRO functions
                if( _ui_exitStandby( timeTick ) && !( guiState->msg.keys & KEY_MONI ) )
                {
                    processEvent = false ;
                }

                if( processEvent )
                {
                    // If MONI is pressed, activate MACRO functions
                    bool moniPressed ;
                    moniPressed = guiState->msg.keys & KEY_MONI ;
                    if( moniPressed || macro_latched )
                    {
                        macro_menu = true ;
                        // long press moni on its own latches function.
                        if( moniPressed && guiState->msg.long_press && !macro_latched )
                        {
                            macro_latched = true ;
                            vp_beep( BEEP_FUNCTION_LATCH_ON , LONG_BEEP );
                        }
                        else if( moniPressed && macro_latched )
                        {
                            macro_latched = false ;
                            vp_beep( BEEP_FUNCTION_LATCH_OFF , LONG_BEEP );
                        }
                        _ui_fsm_menuMacro( guiState , guiState->msg , sync_rtx );
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
                    if( ( guiState->msg.keys & KEY_VOLDOWN ) && guiState->msg.long_press )
                    {
                        macro_menu    = true ;
                        macro_latched = true ;
                    }
#endif // PLA%FORM_TTWRPLUS
                    if( state.tone_enabled && !( guiState->msg.keys & KEY_HASH ) )
                    {
                        state.tone_enabled = false ;
                        *sync_rtx          = true ;
                    }

                    int priorUIScreen = guiState->page.num ;

                    *sync_rtx = ui_UpdatePage( &GuiState );

                    // Enable Tx only if in PAGE_MAIN_VFO or PAGE_MAIN_MEM states
                    bool inMemOrVfo ;
                    inMemOrVfo = ( guiState->page.num == PAGE_MAIN_VFO ) || ( guiState->page.num == PAGE_MAIN_MEM );
                    if( (   macro_menu == true                                    ) ||
                        ( ( inMemOrVfo == false ) && ( state.txDisable == false ) )    )
                    {
                        state.txDisable = true;
                        *sync_rtx       = true;
                    }
                    if( !guiState->f1Handled                   &&
                         ( guiState->msg.keys & KEY_F1       ) &&
                         ( state.settings.vpLevel > VPP_BEEP )    )
                    {
                        vp_replayLastPrompt();
                    }
                    else if( ( priorUIScreen != guiState->page.num ) &&
                             ( state.settings.vpLevel > VPP_LOW    )    )
                    {
                        // When we switch to VFO or Channel screen, we need to announce it.
                        // Likewise for information screens.
                        // All other cases are handled as needed.
                        vp_announceScreen( guiState->page.num );
                    }
                    // generic beep for any keydown if beep is enabled.
                    // At vp levels higher than beep, keys will generate voice so no need
                    // to beep or you'll get an unwanted click.
                    if( ( guiState->msg.keys & 0xFFFF ) && ( state.settings.vpLevel == VPP_BEEP ) )
                    {
                        vp_beep( BEEP_KEY_GENERIC , SHORT_BEEP );
                    }
                    // If we exit and re-enter the same menu, we want to ensure it speaks.
                    if( guiState->msg.keys & KEY_ESC )
                    {
                        _ui_reset_menu_anouncement_tracking();
                    }
                }
                redraw_needed = true ;
                break ;
            }// case EVENT_TYPE_KBD :
            case EVENT_TYPE_STATUS :
            {
                redraw_needed = true ;
#ifdef GPS_PRESENT
                if( ( guiState->page.num == PAGE_MENU_GPS ) &&
                    !vp_isPlaying()                         &&
                    ( state.settings.vpLevel > VPP_LOW    ) &&
                    !txOngoing                              &&
                    !rtx_rxSquelchOpen()                       )
                {
                    // automatically read speed and direction changes only!
                    VPGPSInfoFlags_t whatChanged = GetGPSDirectionOrSpeedChanged();
                    if( whatChanged != VPGPS_NONE )
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
            }// case EVENT_TYPE_STATUS :
        }// switch( event.type )
    }

    // There is some event to process, we need an UI redraw.
    // UI redraw request is cancelled if we're in standby mode.
    if( standby )
    {
        redraw_needed = false ;
    }

}

static bool ui_updateFSM_PAGE_MAIN_VFO( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    // Enable Tx in PAGE_MAIN_VFO mode
    if( state.txDisable )
    {
        state.txDisable = false ;
        sync_rtx        = true ;
    }
    // M17 Destination callsign input
    if( !guiState->uiState.input_locked )
    {
        if( guiState->uiState.edit_mode )
        {
            if( state.channel.mode == OPMODE_M17 )
            {
                if( guiState->msg.keys & KEY_ENTER )
                {
                    _ui_textInputConfirm( guiState , guiState->uiState.new_callsign );
                    // Save selected dst ID and disable input mode
                    strncpy( state.settings.m17_dest , guiState->uiState.new_callsign , 10 );
                    guiState->uiState.edit_mode = false ;
                    sync_rtx           = true ;
                    vp_announceM17Info( NULL , guiState->uiState.edit_mode , guiState->queueFlags );
                }
                else if( guiState->msg.keys & KEY_HASH )
                {
                    // Save selected dst ID and disable input mode
                    strncpy( state.settings.m17_dest , "" , 1 );
                    guiState->uiState.edit_mode = false ;
                    sync_rtx           = true ;
                    vp_announceM17Info( NULL , guiState->uiState.edit_mode , guiState->queueFlags );
                }
                else if( guiState->msg.keys & KEY_ESC )
                {
                    // Discard selected dst ID and disable input mode
                    guiState->uiState.edit_mode = false ;
                }
                else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
                {
                    _ui_textInputDel( guiState , guiState->uiState.new_callsign );
                }
                else if( input_isNumberPressed( guiState->msg ) )
                {
                    _ui_textInputKeypad( guiState , guiState->uiState.new_callsign , 9 , guiState->msg , true );
                }
            }
        }
        else
        {
            if( guiState->msg.keys & KEY_ENTER )
            {
                // Open Menu
                ui_SetPageNum( guiState , PAGE_MENU_TOP );
                // The selected item will be announced when the item is first selected.
            }
            else if( guiState->msg.keys & KEY_ESC )
            {
                // Save VFO channel
                state.vfo_channel = state.channel ;
                int result        = _ui_fsm_loadChannel( state.channel_index , &sync_rtx );
                // Read successful and channel is valid
                if(result != -1)
                {
                    // Switch to MEM screen
                    ui_SetPageNum( guiState , PAGE_MAIN_MEM );
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
                    guiState->uiState.edit_mode = true ;
                    // Reset text input variables
                    _ui_textInputReset( guiState , guiState->uiState.new_callsign );
                    vp_announceM17Info( NULL , guiState->uiState.edit_mode , guiState->queueFlags );
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
                if( state.settings.vpLevel > VPP_BEEP )
                {// quick press repeat vp, long press summary.
                    if( guiState->msg.long_press )
                    {
                        vp_announceChannelSummary( &state.channel , 0 , state.bank , VPSI_ALL_INFO );
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
                ui_SetPageNum( guiState , PAGE_MAIN_VFO_INPUT );
                // Reset input position and selection
                guiState->uiState.input_position = 1 ;
                guiState->uiState.input_set      = SET_RX ;
                // do not play  because we will also announce the number just entered.
                vp_announceInputReceiveOrTransmit( false , VPQ_INIT );
                vp_queueInteger(input_getPressedNumber( guiState->msg ) );
                vp_play();

                guiState->uiState.new_rx_frequency = 0 ;
                guiState->uiState.new_tx_frequency = 0 ;
                // Save pressed number to calculare frequency and show in GUI
                guiState->uiState.input_number     = input_getPressedNumber( guiState->msg );
                // Calculate portion of the new frequency
                guiState->uiState.new_rx_frequency = _ui_freq_add_digit( guiState->uiState.new_rx_frequency , guiState->uiState.input_position , guiState->uiState.input_number );
            }
        }
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MAIN_VFO_INPUT( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ENTER )
    {
        _ui_fsm_confirmVFOInput( guiState , &sync_rtx );
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        // Cancel frequency input, return to VFO mode
        ui_SetPageNum( guiState , PAGE_MAIN_VFO );
    }
    else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN ) )
    {
        //@@@KL why is RX \ TX being toggled?
        switch( guiState->uiState.input_set )
        {
            case SET_RX :
            {
                guiState->uiState.input_set = SET_TX ;
                vp_announceInputReceiveOrTransmit( true , guiState->queueFlags );
                break ;
            }
            case SET_TX :
            {
                guiState->uiState.input_set = SET_RX ;
                vp_announceInputReceiveOrTransmit( false , guiState->queueFlags );
                break ;
            }
        }
        // Reset input position
        guiState->uiState.input_position = 0 ;
    }
    else if( input_isNumberPressed( guiState->msg ) )
    {
        _ui_fsm_insertVFONumber( guiState , guiState->msg , &sync_rtx );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MAIN_MEM( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    // Enable Tx in PAGE_MAIN_MEM mode
    if( state.txDisable )
    {
        state.txDisable = false ;
        sync_rtx        = true ;
    }
    if( !guiState->uiState.input_locked )
    {
        // M17 Destination callsign input
        if( guiState->uiState.edit_mode )
        {
            if( guiState->msg.keys & KEY_ENTER )
            {
                _ui_textInputConfirm( guiState , guiState->uiState.new_callsign );
                // Save selected dst ID and disable input mode
                strncpy( state.settings.m17_dest , guiState->uiState.new_callsign , 10 );
                guiState->uiState.edit_mode = false ;
                sync_rtx                    = true ;
            }
            else if( guiState->msg.keys & KEY_HASH )
            {
                // Save selected dst ID and disable input mode
                strncpy( state.settings.m17_dest , "" , 1 );
                guiState->uiState.edit_mode = false;
                sync_rtx                    = true;
            }
            else if( guiState->msg.keys & KEY_ESC )
            {
                // Discard selected dst ID and disable input mode
                guiState->uiState.edit_mode = false ;
            }
            else if( guiState->msg.keys & KEY_F1 )
            {
                if( state.settings.vpLevel > VPP_BEEP )
                {
                    // Quick press repeat vp, long press summary.
                    if( guiState->msg.long_press )
                    {
                        vp_announceChannelSummary( &state.channel , state.channel_index , state.bank , VPSI_ALL_INFO );
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
                _ui_textInputDel( guiState , guiState->uiState.new_callsign );
            }
            else if( input_isNumberPressed( guiState->msg ) )
            {
                _ui_textInputKeypad( guiState , guiState->uiState.new_callsign , 9 , guiState->msg , true );
            }
        }
        else
        {
            if( guiState->msg.keys & KEY_ENTER )
            {
                // Open Menu
                ui_SetPageNum( guiState , PAGE_MENU_TOP );
            }
            else if( guiState->msg.keys & KEY_ESC )
            {
                // Restore VFO channel
                state.channel   = state.vfo_channel ;
                // Update RTX configuration
                sync_rtx        = true ;
                // Switch to VFO screen
                ui_SetPageNum( guiState , PAGE_MAIN_VFO );
            }
            else if( guiState->msg.keys & KEY_HASH )
            {
                // Only enter edit mode when using M17
                if( state.channel.mode == OPMODE_M17 )
                {
                    // Enable dst ID input
                    guiState->uiState.edit_mode = true ;
                    // Reset text input variables
                    _ui_textInputReset( guiState , guiState->uiState.new_callsign );
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
                if( state.settings.vpLevel > VPP_BEEP )
                {
                    // quick press repeat vp, long press summary.
                    if( guiState->msg.long_press )
                    {
                        vp_announceChannelSummary( &state.channel , state.channel_index+1 , state.bank , VPSI_ALL_INFO );
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

static bool ui_updateFSM_PAGE_MODE_VFO( GuiState_st* guiState )
{
    return ui_updateFSM_PAGE_MENU_TOP( guiState );
}

static bool ui_updateFSM_PAGE_MODE_MEM( GuiState_st* guiState )
{
    return ui_updateFSM_PAGE_MENU_TOP( guiState );
}

static bool ui_updateFSM_PAGE_MENU_TOP( GuiState_st* guiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        _ui_menuDown( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        ui_SetPageNum( guiState , guiState->layout.links[ guiState->uiState.menu_selected ].num );
        // Reset menu selection
        guiState->uiState.menu_selected = 0 ;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_BANK( GuiState_st* guiState )
{
    return ui_updateFSM_PAGE_MENU_CONTACTS( guiState );
}

static bool ui_updateFSM_PAGE_MENU_CHANNEL( GuiState_st* guiState )
{
    return ui_updateFSM_PAGE_MENU_CONTACTS( guiState );
}

static bool ui_updateFSM_PAGE_MENU_CONTACTS( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        // Using 1 as parameter disables menu wrap around
        _ui_menuUp( guiState , 1 );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        if( guiState->page.num == PAGE_MENU_BANK )
        {
            bankHdr_t bank;
            // manu_selected is 0-based
            // bank 0 means "All Channel" mode
            // banks (1, n) are mapped to banks (0, n-1)
            if( cps_readBankHeader( &bank , guiState->uiState.menu_selected ) != -1 )
            {
                guiState->uiState.menu_selected += 1 ;
            }
        }
        else if( guiState->page.num == PAGE_MENU_CHANNEL )
        {
            channel_t channel ;
            if( cps_readChannel( &channel , guiState->uiState.menu_selected + 1 ) != -1 )
            {
                guiState->uiState.menu_selected += 1 ;
            }
        }
        else if( guiState->page.num == PAGE_MENU_CONTACTS )
        {
            contact_t contact ;
            if( cps_readContact( &contact , guiState->uiState.menu_selected + 1 ) != -1 )
            {
                guiState->uiState.menu_selected += 1 ;
            }
        }
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        switch( guiState->page.num )
        {
            case PAGE_MENU_BANK :
            {
                bankHdr_t newbank ;
                int       result  = 0 ;
                // If "All channels" is selected, load default bank
                if( guiState->uiState.menu_selected == 0 )
                {
                    state.bank_enabled = false ;
                }
                else
                {
                    state.bank_enabled = true;
                    result = cps_readBankHeader( &newbank , guiState->uiState.menu_selected - 1 );
                }
                if( result != -1 )
                {
                    state.bank = guiState->uiState.menu_selected - 1 ;
                    // If we were in VFO mode, save VFO channel
                    if( guiState->page.num == PAGE_MAIN_VFO )
                    {
                        state.vfo_channel = state.channel ;
                    }
                    // Load bank first channel
                    _ui_fsm_loadChannel( 0 , &sync_rtx );
                    // Switch to MEM screen
                    ui_SetPageNum( guiState , PAGE_MAIN_MEM );
                }
                break ;
            }
            case PAGE_MENU_CHANNEL :
            {
                // If we were in VFO mode, save VFO channel
                if( guiState->page.num == PAGE_MAIN_VFO )
                {
                    state.vfo_channel = state.channel;
                }
                _ui_fsm_loadChannel( guiState->uiState.menu_selected , &sync_rtx );
                // Switch to MEM screen
                ui_SetPageNum( guiState , PAGE_MAIN_MEM );
                break ;
            }
            case PAGE_MENU_CONTACTS :
            {
                _ui_fsm_loadContact( guiState->uiState.menu_selected , &sync_rtx );
                // Switch to MEM screen
                ui_SetPageNum( guiState , PAGE_MAIN_MEM );
                break ;
            }
        }
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_GPS( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( ( guiState->msg.keys & KEY_F1 ) && ( state.settings.vpLevel > VPP_BEEP ) )
    {
        // quick press repeat vp, long press summary.
        if( guiState->msg.long_press )
        {
            vp_announceGPSInfo( VPGPS_ALL );
        }
        else
        {
            vp_replayLastPrompt();
        }
        guiState->f1Handled = true;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_SETTINGS( GuiState_st* guiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        _ui_menuDown( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        ui_SetPageNum( guiState , guiState->layout.links[ guiState->uiState.menu_selected ].num );
        // Reset menu selection
        guiState->uiState.menu_selected = 0 ;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_BACKUP_RESTORE( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        _ui_menuDown( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        ui_SetPageNum( guiState , guiState->layout.links[ guiState->uiState.menu_selected ].num );
        // Reset menu selection
        guiState->uiState.menu_selected = 0 ;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_BACKUP( GuiState_st* guiState )
{
    return - ui_updateFSM_PAGE_MENU_RESTORE( guiState );
}

static bool ui_updateFSM_PAGE_MENU_RESTORE( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_INFO( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        _ui_menuDown( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_ABOUT( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE( GuiState_st* guiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ENTER )
    {
        // Switch to set Time&Date mode
        ui_SetPageNum( guiState , PAGE_SETTINGS_TIMEDATE_SET );
        // Reset input position and selection
        guiState->uiState.input_position = 0 ;
        memset( &guiState->uiState.new_timedate , 0 , sizeof( datetime_t ) );
        vp_announceBuffer( &currentLanguage->timeAndDate , true , false , "dd/mm/yy" );
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE_SET( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ENTER )
    {
        // Save time only if all digits have been inserted
        if( guiState->uiState.input_position >= TIMEDATE_DIGITS )
        {
            // Return to Time&Date menu, saving values
            // NOTE: The user inserted a local time, we must save an UTC time
            datetime_t utc_time = localTimeToUtc( guiState->uiState.new_timedate , state.settings.utc_timezone );
            platform_setTime( utc_time );
            state.time          = utc_time ;
            vp_announceSettingsTimeDate();
            ui_SetPageNum( guiState , PAGE_SETTINGS_TIMEDATE_SET );
        }
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        _ui_menuBack( guiState );
    }
    else if( input_isNumberPressed( guiState->msg ) )
    {
        // if present - discard excess digits
        if( guiState->uiState.input_position <= TIMEDATE_DIGITS )
        {
            guiState->uiState.input_position += 1;
            guiState->uiState.input_number    = input_getPressedNumber( guiState->msg );
            _ui_timedate_add_digit( &guiState->uiState.new_timedate , guiState->uiState.input_position , guiState->uiState.input_number );
        }
    }

    return sync_rtx ;

}

// D_BRIGHTNESS , D_CONTRAST , D_TIMER
static bool ui_updateFSM_PAGE_SETTINGS_DISPLAY( GuiState_st* guiState )
{
    bool handled ;
    bool sync_rtx ;

    sync_rtx = ui_InputValue( guiState , &handled );

    if( !handled )
    {
        if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            _ui_menuUp( guiState , uiGetPageNumOf( guiState ) );
        }
        else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
        {
            _ui_menuDown( guiState , uiGetPageNumOf( guiState ) );
        }
        else if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            _ui_menuBack( guiState );
        }
    }

    return sync_rtx ;

}

// G_ENABLED , G_SET_TIME , G_TIMEZONE
static bool ui_updateFSM_PAGE_SETTINGS_GPS( GuiState_st* guiState )
{
    bool handled ;
    bool sync_rtx ;

    sync_rtx = ui_InputValue( guiState , &handled );

    if( !handled )
    {
        if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            _ui_menuUp( guiState , uiGetPageNumOf( guiState ) );
        }
        else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
        {
            _ui_menuDown( guiState , uiGetPageNumOf( guiState ) );
        }
        else if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            _ui_menuBack( guiState );
        }
    }

    return sync_rtx ;

}

// R_OFFSET , R_DIRECTION , R_STEP
static bool ui_updateFSM_PAGE_SETTINGS_RADIO( GuiState_st* guiState )
{
    bool handled ;
    bool sync_rtx ;

    sync_rtx = ui_InputValue( guiState , &handled );

    if( !handled )
    {
        if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            _ui_menuUp( guiState , uiGetPageNumOf( guiState ) );
        }
        else if( ( guiState->msg.keys & KEY_DOWN ) || ( guiState->msg.keys & KNOB_RIGHT ) )
        {
            _ui_menuDown( guiState , uiGetPageNumOf( guiState ) );
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            _ui_menuBack( guiState );
        }
    }

    return sync_rtx ;

}

// M17_CALLSIGN , M17_CAN , M17_CAN_RX
static bool ui_updateFSM_PAGE_SETTINGS_M17( GuiState_st* guiState )
{
    bool handled ;
    bool sync_rtx ;

    sync_rtx = ui_InputValue( guiState , &handled );

    if( !handled )
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            // Enable edit mode
            guiState->uiState.edit_mode = true;
        }
        else if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            _ui_menuUp( guiState , uiGetPageNumOf( guiState ) );
        }
        else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
        {
            _ui_menuDown( guiState , uiGetPageNumOf( guiState ) );
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            sync_rtx = true ;
            _ui_menuBack( guiState );
        }
    }

    return sync_rtx ;

}

// VP_LEVEL , VP_PHONETIC
static bool ui_updateFSM_PAGE_SETTINGS_VOICE( GuiState_st* guiState )
{
    bool handled ;
    bool sync_rtx ;

    sync_rtx = ui_InputValue( guiState , &handled );

    if( !handled )
    {
        if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            _ui_menuUp( guiState , uiGetPageNumOf( guiState ) );
        }
        else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
        {
            _ui_menuDown( guiState , uiGetPageNumOf( guiState ) );
        }
        else if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            _ui_menuBack( guiState );
        }
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_RESET_TO_DEFAULTS( GuiState_st* guiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    if( !guiState->uiState.edit_mode )
    {
        //require a confirmation ENTER, then another
        //edit_mode is slightly misused to allow for this
        if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = true ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            _ui_menuBack( guiState );
        }
    }
    else
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = false ;
            state_resetSettingsAndVfo();
            _ui_menuBack( guiState );
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            guiState->uiState.edit_mode = false ;
            _ui_menuBack( guiState );
        }
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_LOW_BAT( GuiState_st* guiState )
{
    (void)guiState ;
    return false ;
}

static bool ui_updateFSM_PAGE_ABOUT( GuiState_st* guiState )
{
    (void)guiState ;
    return false ;
}

static bool ui_updateFSM_PAGE_STUBBED( GuiState_st* guiState )
{
    (void)guiState ;
    return false ;
}

#ifdef SCREEN_BRIGHTNESS
static bool ui_InputValue_BRIGHTNESS( GuiState_st* guiState , bool* handled );
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
static bool ui_InputValue_CONTRAST( GuiState_st* guiState , bool* handled );
#endif // SCREEN_CONTRAST
static bool ui_InputValue_TIMER( GuiState_st* guiState , bool* handled );
#ifdef GPS_PRESENT
static bool ui_InputValue_ENABLED( GuiState_st* guiState , bool* handled );
static bool ui_InputValue_SET_TIME( GuiState_st* guiState , bool* handled );
static bool ui_InputValue_TIMEZONE( GuiState_st* guiState , bool* handled );
#endif // GPS_PRESENT
static bool ui_InputValue_LEVEL( GuiState_st* guiState , bool* handled );
static bool ui_InputValue_PHONETIC( GuiState_st* guiState , bool* handled );
static bool ui_InputValue_OFFSET( GuiState_st* guiState , bool* handled );
static bool ui_InputValue_DIRECTION( GuiState_st* guiState , bool* handled );
static bool ui_InputValue_STEP( GuiState_st* guiState , bool* handled );
static bool ui_InputValue_CALLSIGN( GuiState_st* guiState , bool* handled );
static bool ui_InputValue_CAN( GuiState_st* guiState , bool* handled );
static bool ui_InputValue_CAN_RX( GuiState_st* guiState , bool* handled );
static bool ui_InputValue_STUBBED( GuiState_st* guiState , bool* handled );

typedef bool (*ui_InputValue_fn)( GuiState_st* guiState , bool* handled );

// GUI Values - Set
static const ui_InputValue_fn ui_InputValue_Table[ GUI_VAL_NUM_OF ] =
{
    ui_InputValue_STUBBED     , // GUI_VAL_CURRENT_TIME
    ui_InputValue_STUBBED     , // GUI_VAL_BATTERY_LEVEL
    ui_InputValue_STUBBED     , // GUI_VAL_LOCK_STATE
    ui_InputValue_STUBBED     , // GUI_VAL_MODE_INFO
    ui_InputValue_STUBBED     , // GUI_VAL_FREQUENCY
    ui_InputValue_STUBBED     , // GUI_VAL_RSSI_METER

    ui_InputValue_STUBBED     , // GUI_VAL_BANKS
    ui_InputValue_STUBBED     , // GUI_VAL_CHANNELS
    ui_InputValue_STUBBED     , // GUI_VAL_CONTACTS
    ui_InputValue_STUBBED     , // GUI_VAL_GPS
#ifdef SCREEN_BRIGHTNESS
    ui_InputValue_BRIGHTNESS  , // GUI_VAL_BRIGHTNESS       , D_BRIGHTNESS
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
    ui_InputValue_CONTRAST    , // GUI_VAL_CONTRAST         , D_CONTRAST
#endif // SCREEN_CONTRAST
    ui_InputValue_TIMER       , // GUI_VAL_TIMER            , D_TIMER
    ui_InputValue_STUBBED     , // GUI_VAL_DATE
    ui_InputValue_STUBBED     , // GUI_VAL_TIME
    ui_InputValue_ENABLED     , // GUI_VAL_GPS_ENABLED      , G_ENABLED
    ui_InputValue_SET_TIME    , // GUI_VAL_GPS_SET_TIME     , G_SET_TIME
    ui_InputValue_TIMEZONE    , // GUI_VAL_GPS_TIME_ZONE    , G_TIMEZONE
    ui_InputValue_OFFSET      , // GUI_VAL_RADIO_OFFSET     , R_OFFSET
    ui_InputValue_DIRECTION   , // GUI_VAL_RADIO_DIRECTION  , R_DIRECTION
    ui_InputValue_STEP        , // GUI_VAL_RADIO_STEP       , R_STEP
    ui_InputValue_CALLSIGN    , // GUI_VAL_M17_CALLSIGN     , M17_CALLSIGN
    ui_InputValue_CAN         , // GUI_VAL_M17_CAN          , M17_CAN
    ui_InputValue_CAN_RX      , // GUI_VAL_M17_CAN_RX_CHECK , M17_CAN_RX
    ui_InputValue_LEVEL       , // GUI_VAL_LEVEL            , VP_LEVEL
    ui_InputValue_PHONETIC    , // GUI_VAL_PHONETIC         , VP_PHONETIC
    ui_InputValue_STUBBED     , // GUI_VAL_BATTERY_VOLTAGE
    ui_InputValue_STUBBED     , // GUI_VAL_BATTERY_CHARGE
    ui_InputValue_STUBBED     , // GUI_VAL_RSSI
    ui_InputValue_STUBBED     , // GUI_VAL_USED_HEAP
    ui_InputValue_STUBBED     , // GUI_VAL_BAND
    ui_InputValue_STUBBED     , // GUI_VAL_VHF
    ui_InputValue_STUBBED     , // GUI_VAL_UHF
    ui_InputValue_STUBBED     , // GUI_VAL_HW_VERSION
#ifdef PLATFORM_TTWRPLUS
    ui_InputValue_STUBBED     , // GUI_VAL_RADIO
    ui_InputValue_STUBBED     , // GUI_VAL_RADIO_FW
#endif // PLATFORM_TTWRPLUS
    ui_InputValue_STUBBED     , // GUI_VAL_STUBBED
};

static bool ui_InputValue( GuiState_st* guiState , bool* handled )
{
    uint8_t linkSelected = guiState->uiState.menu_selected ;
    uint8_t valueNum     = guiState->layout.links[ linkSelected ].num ;

    if( valueNum >= GUI_VAL_NUM_OF )
    {
        valueNum = GUI_VAL_STUBBED ;
    }

    *handled = true ;

    return ui_InputValue_Table[ valueNum ]( guiState , handled );

}

#ifdef SCREEN_BRIGHTNESS
// D_BRIGHTNESS
static bool ui_InputValue_BRIGHTNESS( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    if( (   guiState->msg.keys & KEY_LEFT                                         ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) && guiState->uiState.edit_mode )    )
    {
        _ui_changeBrightness( -5 );
        vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.brightness );
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                                       ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) && guiState->uiState.edit_mode )    )
    {
        _ui_changeBrightness( +5 );
        vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.brightness );
    }
    else
    {
        *handled = false ;
    }

    return sync_rtx ;

}
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
// D_CONTRAST
static bool ui_InputValue_CONTRAST( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    if( (   guiState->msg.keys & KEY_LEFT                      ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) &&
        guiState->uiState.edit_mode                            )    )
    {
        _ui_changeContrast( -4 );
        vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.contrast );
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                    ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) &&
             guiState->uiState.edit_mode                           )    )
    {
        _ui_changeContrast( +4 );
        vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.contrast );
    }
    else
    {
        *handled = false ;
    }

    return sync_rtx ;

}
#endif // SCREEN_CONTRAST
// D_TIMER
static bool ui_InputValue_TIMER( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    if( (   guiState->msg.keys & KEY_LEFT                      ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) &&
        guiState->uiState.edit_mode                            )    )
    {
        _ui_changeTimer( -1 );
        vp_announceDisplayTimer();
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                    ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) &&
             guiState->uiState.edit_mode                           )    )
    {
        _ui_changeTimer( +1 );
        vp_announceDisplayTimer();
    }
    else
    {
        *handled = false ;
    }

    return sync_rtx ;

}
#ifdef GPS_PRESENT
// G_ENABLED
static bool ui_InputValue_ENABLED( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    if( (   guiState->msg.keys & ( KEY_LEFT | KEY_RIGHT )                            ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT | KEY_UP | KNOB_RIGHT ) ) &&
        guiState->uiState.edit_mode                                                  )    )
    {
        if( state.settings.gps_enabled )
        {
            state.settings.gps_enabled = 0 ;
        }
        else
        {
            state.settings.gps_enabled = 1 ;
        }
        vp_announceSettingsOnOffToggle( &currentLanguage->gpsEnabled ,
                                        guiState->queueFlags , state.settings.gps_enabled );
    }
    else
    {
        *handled = false ;
    }

    return sync_rtx ;

}

// G_SET_TIME
static bool ui_InputValue_SET_TIME( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    if( (   guiState->msg.keys & ( KEY_LEFT | KEY_RIGHT )                            ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT | KEY_UP | KNOB_RIGHT ) ) &&
        guiState->uiState.edit_mode                                                  )    )
    {
        state.gps_set_time = !state.gps_set_time ;
        vp_announceSettingsOnOffToggle( &currentLanguage->gpsSetTime ,
                                        guiState->queueFlags , state.gps_set_time );
    }
    else
    {
        *handled = false ;
    }

    return sync_rtx ;

}

// G_TIMEZONE
static bool ui_InputValue_TIMEZONE( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    if( (   guiState->msg.keys & ( KEY_LEFT | KEY_RIGHT )                            ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT | KEY_UP | KNOB_RIGHT ) ) &&
        guiState->uiState.edit_mode                                                  )    )
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
    }
    else
    {
        *handled = false ;
    }

    return sync_rtx ;

}
#endif // GPS_PRESENT

// R_OFFSET
static bool ui_InputValue_OFFSET( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    // If the entry is selected with enter we are in edit_mode
    if( guiState->uiState.edit_mode )
    {
#ifdef UI_NO_KEYBOARD
        if( guiState->msg.long_press && ( guiState->msg.keys & KEY_ENTER ) )
        {
            // Long press on UI_NO_KEYBOARD causes digits to advance by one
            guiState->uiState.new_offset /= 10 ;
#else // UI_NO_KEYBOARD
        if( guiState->msg.keys & KEY_ENTER )
        {
#endif // UI_NO_KEYBOARD
            // Apply new offset
            state.channel.tx_frequency  = state.channel.rx_frequency + guiState->uiState.new_offset ;
            vp_queueStringTableEntry( &currentLanguage->frequencyOffset );
            vp_queueFrequency( guiState->uiState.new_offset );
            guiState->uiState.edit_mode = false ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            // Announce old frequency offset
            vp_queueStringTableEntry( &currentLanguage->frequencyOffset );
            vp_queueFrequency( (int32_t)state.channel.tx_frequency -
                               (int32_t)state.channel.rx_frequency );
        }
        else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
        {
            _ui_numberInputDel( guiState , &guiState->uiState.new_offset );
        }
#ifdef UI_NO_KEYBOARD
        else if( guiState->msg.keys & ( KNOB_LEFT | KNOB_RIGHT | KEY_ENTER ) )
#else // UI_NO_KEYBOARD
        else if( input_isNumberPressed( guiState->msg ) )
#endif // UI_NO_KEYBOARD
        {
            _ui_numberInputKeypad( guiState , &guiState->uiState.new_offset , guiState->msg );
            guiState->uiState.input_position += 1 ;
        }
        else if( guiState->msg.long_press              &&
                 ( guiState->msg.keys     & KEY_F1   ) &&
                 ( state.settings.vpLevel > VPP_BEEP )    )
        {
            vp_queueFrequency( guiState->uiState.new_offset );
            guiState->f1Handled = true ;
        }
        // If ENTER or ESC are pressed, exit edit mode, R_OFFSET is managed separately
        if( !( guiState->msg.keys & KEY_ENTER ) && ( guiState->msg.keys & KEY_ESC ) )
        {
            guiState->uiState.edit_mode = false ;
        }
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        guiState->uiState.edit_mode      = true;
        guiState->uiState.new_offset     = 0 ;
        // Reset input position
        guiState->uiState.input_position = 0 ;
    }
    else
    {
        *handled = false ;
    }

    return sync_rtx ;

}

// R_DIRECTION
static bool ui_InputValue_DIRECTION( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    // If the entry is selected with enter we are in edit_mode
    if( guiState->uiState.edit_mode )
    {
        if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KNOB_LEFT | KNOB_RIGHT ) )
        {
            // Invert frequency offset direction
            if( state.channel.tx_frequency >= state.channel.rx_frequency )
            {
                state.channel.tx_frequency -= 2 * ( (int32_t)state.channel.tx_frequency -
                                                    (int32_t)state.channel.rx_frequency );
            }
            else // Switch to positive offset
            {
                state.channel.tx_frequency -= 2 * ( (int32_t)state.channel.tx_frequency -
                                                    (int32_t)state.channel.rx_frequency );
            }
        }
        // If ENTER or ESC are pressed, exit edit mode
        if( ( guiState->msg.keys & KEY_ENTER ) ||
            ( guiState->msg.keys & KEY_ESC   )    )
        {
            guiState->uiState.edit_mode = false ;
        }
    }
    else if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( ( guiState->msg.keys & KEY_DOWN ) || ( guiState->msg.keys & KNOB_RIGHT ) )
    {
        _ui_menuDown( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        guiState->uiState.edit_mode = true;
        // If we are entering R_OFFSET clear temp offset
        if( guiState->uiState.menu_selected == R_OFFSET )
        {
            guiState->uiState.new_offset = 0 ;
        }
        // Reset input position
        guiState->uiState.input_position = 0 ;
    }
    else
    {
        *handled = false ;
    }

    return sync_rtx ;

}

// R_STEP
static bool ui_InputValue_STEP( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    // If the entry is selected with enter we are in edit_mode
    if( guiState->uiState.edit_mode )
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
        // If ENTER or ESC are pressed, exit edit mode, R_OFFSET is managed separately
        if( ( ( guiState->msg.keys & KEY_ENTER ) && ( guiState->uiState.menu_selected != R_OFFSET ) ) ||
              ( guiState->msg.keys & KEY_ESC   )                                                )
        {
            guiState->uiState.edit_mode = false ;
        }
    }
    else if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        _ui_menuUp( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( ( guiState->msg.keys & KEY_DOWN ) || ( guiState->msg.keys & KNOB_RIGHT ) )
    {
        _ui_menuDown( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        guiState->uiState.edit_mode = true;
        // If we are entering R_OFFSET clear temp offset
        if( guiState->uiState.menu_selected == R_OFFSET )
        {
            guiState->uiState.new_offset = 0 ;
        }
        // Reset input position
        guiState->uiState.input_position = 0 ;
    }
    else
    {
        *handled = false ;
    }

    return sync_rtx ;

}

// M17_CALLSIGN
static bool ui_InputValue_CALLSIGN( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    if( guiState->uiState.edit_mode )
    {
        // Handle text input for M17 callsign
        if( guiState->msg.keys & KEY_ENTER )
        {
            _ui_textInputConfirm( guiState , guiState->uiState.new_callsign );
            // Save selected callsign and disable input mode
            strncpy( state.settings.callsign , guiState->uiState.new_callsign , 10 );
            guiState->uiState.edit_mode = false;
            vp_announceBuffer( &currentLanguage->callsign , false , true , state.settings.callsign );
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            // Discard selected callsign and disable input mode
            guiState->uiState.edit_mode = false ;
            vp_announceBuffer( &currentLanguage->callsign , false , true , state.settings.callsign );
        }
        else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
        {
            _ui_textInputDel( guiState , guiState->uiState.new_callsign );
        }
        else if( input_isNumberPressed( guiState->msg ) )
        {
            _ui_textInputKeypad( guiState , guiState->uiState.new_callsign , 9 , guiState->msg , true );
        }
        else if( guiState->msg.long_press              &&
                 ( guiState->msg.keys     & KEY_F1   ) &&
                 ( state.settings.vpLevel > VPP_BEEP )    )
        {
            vp_announceBuffer( &currentLanguage->callsign , true , true , guiState->uiState.new_callsign );
            guiState->f1Handled = true ;
        }
        else
        {
            *handled = false ;
        }
    }
    else
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            // Enable edit mode
            guiState->uiState.edit_mode = true;
            _ui_textInputReset( guiState , guiState->uiState.new_callsign );
            vp_announceBuffer( &currentLanguage->callsign , true , true , guiState->uiState.new_callsign );
        }
	    else
	    {
	        *handled = false ;
    	}
    }

    return sync_rtx ;

}

// M17_CAN
static bool ui_InputValue_CAN( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    if( guiState->uiState.edit_mode )
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
            guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            guiState->uiState.edit_mode = false ;
        }
        else
        {
            *handled = false ;
        }
    }
    else
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            // Enable edit mode
            guiState->uiState.edit_mode = true;
        }
        else if( guiState->msg.keys & KEY_RIGHT )
        {
            _ui_changeM17Can( +1 );
        }
        else if( guiState->msg.keys & KEY_LEFT )
        {
            _ui_changeM17Can( -1 );
        }
	    else
	    {
	        *handled = false ;
	    }
    }

    return sync_rtx ;

}

// M17_CAN_RX
static bool ui_InputValue_CAN_RX( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    if( guiState->uiState.edit_mode )
    {
        if( (   guiState->msg.keys & ( KEY_LEFT | KEY_RIGHT                       )      ) ||
            ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT | KEY_UP | KNOB_RIGHT ) ) &&
            guiState->uiState.edit_mode                                                  )    )
        {
            state.settings.m17_can_rx = !state.settings.m17_can_rx ;
        }
        else if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            guiState->uiState.edit_mode = false ;
        }
        else
        {
            *handled = false ;
        }
    }
    else
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            // Enable edit mode
            guiState->uiState.edit_mode = true;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            sync_rtx = true ;
            _ui_menuBack( guiState );
        }
        else
        {
            *handled = false ;
        }
    }

    return sync_rtx ;

}

// VP_LEVEL
static bool ui_InputValue_LEVEL( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    if( (   guiState->msg.keys & KEY_LEFT                      ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) &&
          guiState->uiState.edit_mode                          )    )
    {
        _ui_changeVoiceLevel( -1 );
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                    ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) &&
             guiState->uiState.edit_mode                           )    )
    {
        _ui_changeVoiceLevel( 1 );
    }
    else
    {
        *handled = false ;
    }

    return sync_rtx ;

}

// VP_PHONETIC
static bool ui_InputValue_PHONETIC( GuiState_st* guiState , bool* handled )
{
    bool sync_rtx = false ;

    *handled = true ;

    if( (   guiState->msg.keys & KEY_LEFT                      ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) &&
        guiState->uiState.edit_mode                            )    )
    {
        _ui_changePhoneticSpell( false );
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                    ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) &&
             guiState->uiState.edit_mode                           )    )
    {
        _ui_changePhoneticSpell( true );
    }
    else
    {
        *handled = false ;
    }

    return sync_rtx ;

}

static bool ui_InputValue_STUBBED( GuiState_st* guiState , bool* handled )
{
    (void)guiState;

    *handled = true ;

    return false ;
}

bool ui_updateGUI( Event_st* event )
{
    bool render = false ;

    if( redraw_needed == true )
    {
        ui_draw( &GuiState , event );
        // If MACRO menu is active draw it
        if( macro_menu )
        {
            _ui_drawDarkOverlay();
            _ui_drawMacroMenu( &GuiState );
        }
#ifdef DISPLAY_DEBUG_MSG
        Debug_DisplayMsg();
#endif // DISPLAY_DEBUG_MSG
        redraw_needed = false ;
        render        = true ;
    }

    return render ;
}

bool ui_pushEvent( const uint8_t type , const uint32_t data )
{
    uint8_t  newHead = ( evQueue_wrPos + 1 ) % MAX_NUM_EVENTS ;
    Event_st event ;
    bool     result  = false ;

    // Queue is not full
    if( newHead != evQueue_rdPos )
    {
        // Preserve atomicity when writing the new element into the queue.
        event.type               = type ;
        event.payload            = data ;
        evQueue[ evQueue_wrPos ] = event ;
        evQueue_wrPos            = newHead ;
        result                   = true ;
    }

    return result ;
}

bool ui_popEvent( Event_st* event )
{
    bool eventPresent = false ;

    event->type    = EVENT_TYPE_NONE ;
    event->payload = 0 ;

    // Check for events
    if( evQueue_rdPos != evQueue_wrPos )
    {
        // Pop an event from the queue
        *event        = evQueue[ evQueue_rdPos ] ;
        evQueue_rdPos = ( evQueue_rdPos + 1 ) % MAX_NUM_EVENTS ;
        eventPresent  = true ;
    }

    return eventPresent ;

}

static const uint32_t ColorTable[ COLOR_NUM_OF ] =
{
    COLOR_TABLE
};

void uiColorLoad( Color_st* color , ColorSelector_en colorSelector )
{
    ColorSelector_en colorSel = colorSelector ;

    if( colorSel > COLOR_NUM_OF )
    {
        colorSel = COLOR_AL ;
    }

    COLOR_LD( color , ColorTable[ colorSel ] )
}

void ui_SetPageNum( GuiState_st* guiState , uint8_t pageNum )
{
    uint8_t pgNum = pageNum ;

    if( pgNum >= PAGE_NUM_OF )
    {
        pgNum = PAGE_STUBBED ;
    }

    guiState->page.num = pgNum ;
    state.ui_page      = pgNum ;
}

void ui_terminate( void )
{
}
