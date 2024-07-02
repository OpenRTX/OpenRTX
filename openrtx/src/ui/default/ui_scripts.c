/***************************************************************************
 *   Copyright (C) 2020 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                Kim Lyon VK6KL                           *
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

#include <ui.h>
#include <ui/ui_default.h>
#include "ui_commands.h"
#include "ui_value_display.h"
#include "ui_value_input.h"
#include "ui_scripts.h"

static const uint8_t Page_MainVFO[] =
{
//    GUI_CMD_GOTO_TEXT_LINE , ST_VAL( GUI_LINE_TOP ) ,
//    GUI_CMD_LOAD_STYLE , ST_VAL( GUI_STYLE_TOP ) ,
//    ALIGN_CENTER ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_TIME ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_BATTERY ) ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_BATTERY_LEVEL ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_DISPLAY_TIME_TICK ) ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_LOCK_STATE ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_MODE_INFO ) ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_FREQUENCY ) ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_RSSI ) ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_RSSI_METER ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MainInput[] =
{
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_TIME ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_BATTERY ) ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_BATTERY_LEVEL ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_DISPLAY_TIME_TICK ) ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_LOCK_STATE ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_VALUE_INP , ST_VAL( GUI_VAL_INP_VFO_MIDDLE_INPUT ) ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_RSSI ) ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_RSSI_METER ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_PAGE_END

};

static const uint8_t Page_MainMem[] =
{
    // Graphics Test
    GOTO_X( 20 ) ,
    GOTO_Y( 20 ) ,
    LINE(  20 ,   0 ) ,
    FG_COLOR( RED ) ,
    LINE(   0 ,  20 ) ,
    FG_COLOR( GREEN ) ,
    LINE( -20 ,   0 ) ,
    FG_COLOR( BLUE ) ,
    LINE(   0 , -20 ) ,
    GOTO_TEXT_LINE( GUI_LINE_3 ) ,
    ADD_TO_X( 40 ) ,
    FG_COLOR( RED ) ,
    LINE( 20 , 20 ) ,
    ADD_TO_X( 10 ) ,
    ADD_TO_Y( -30 ) ,
    FG_COLOR( BLUE ) ,
    RECT( 15 , 20 ) ,
    ADD_TO_X( 20 ) ,
    FG_COLOR( GREEN ) ,
    RECT_FILL( 20 , 15 ) ,

    GUI_CMD_PAGE_END ,//@@@KL

    ALIGN_CENTER ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      GUI_CMD_BG_COLOR , ST_VAL( COLOR_RED ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_TIME ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_BATTERY ) ) ,
//      ALIGN_RIGHT , //@@@KL ensure works
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_BATTERY_LEVEL ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_DISPLAY_TIME_TICK ) ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_LOCK_STATE ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_LINE_END ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_MODE_INFO ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_BANK_CHANNEL ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_FREQUENCY ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_RSSI ) ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_RSSI_METER ) ,
    GUI_CMD_EVENT_END ,
    GUI_CMD_PAGE_END ,//@@@KL
    GUI_CMD_PAGE_END
};

static const uint8_t Page_ModeVFO[] =
{
    GUI_CMD_TITLE , 'W','T','D',':',' ','M','o','d','e','V','F','O', GUI_CMD_NULL ,
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_ModeMem[] =
{
    GUI_CMD_TITLE , 'W','T','D',':',' ','M','o','d','e','M','e','m', GUI_CMD_NULL ,
    GUI_CMD_PAGE_END   //@@@KL if at start of page indicates use the legacy script
};

static const uint8_t Page_MenuTop[] =
{
    GUI_CMD_TITLE , 'M','e','n','u' , GUI_CMD_NULL ,
    GUI_CMD_GOTO_TEXT_LINE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_LOAD_STYLE , ST_VAL( GUI_STYLE_1 ) ,
    ALIGN_LEFT ,
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
    GUI_CMD_LINK , GUI_CMD_PAGE , ST_VAL( PAGE_ABOUT ) ,
    GUI_CMD_TEXT , 'A','b','o','u','t' , GUI_CMD_NULL ,
    GUI_CMD_LINK_END ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuBank[] =
{
    GUI_CMD_TITLE , 'B','a','n','k' , GUI_CMD_NULL ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_BANKS ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuChannel[] =
{
    GUI_CMD_TITLE , 'C','h','a','n','n','e','l' , GUI_CMD_NULL ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_CHANNELS ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuContact[] =
{
    GUI_CMD_TITLE , 'C','o','n','t','a','c','t' , GUI_CMD_NULL ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_CONTACTS ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuGPS[] =
{
    GUI_CMD_TITLE , 'G','P','S' , GUI_CMD_NULL ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_GPS ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuSettings[] =
{
    GUI_CMD_TITLE , 'S','e','t','t','i','n','g','s' , GUI_CMD_NULL ,
    GUI_CMD_GOTO_TEXT_LINE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_LOAD_STYLE , ST_VAL( GUI_STYLE_1 ) ,
    ALIGN_LEFT ,
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

static const uint8_t Page_SettingsTimeDate[] =
{
    GUI_CMD_TITLE , 'T','i','m','e',' ','&',' ','D','a','t','e', GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_LINE_END ,
    ALIGN_CENTER ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_DATE ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_LINE_END ,
    ALIGN_CENTER ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_TIME ) ,
    GUI_CMD_EVENT_END ,


/*
    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line2       = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2      = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
//    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    datetime_t local_time = utcToLocalTime( last_state.time ,
                                            last_state.settings.utc_timezone );
    // Print "Time&Date" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               color_fg , currentLanguage->timeAndDate );
    // Print current time and date
    gfx_print( line2->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
               color_fg , "%02d/%02d/%02d" ,
               local_time.date , local_time.month , local_time.year );
    gfx_print( line3Large->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
               color_fg , "%02d:%02d:%02d" ,
               local_time.hour , local_time.minute , local_time.second );
*/
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_SettingsTimeDateSet[] =
{
    GUI_CMD_TITLE , 'W','T','D',':',' ','S','T','D','S','e','t', GUI_CMD_NULL ,
    GUI_CMD_PAGE_END   //@@@KL indicates use the legacy script
};

static const uint8_t Page_SettingsDisplay[] =
{
    GUI_CMD_TITLE , 'D','i','s','p','l','a','y' , GUI_CMD_NULL ,
    GUI_CMD_GOTO_TEXT_LINE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_LOAD_STYLE , ST_VAL( GUI_STYLE_1 ) ,
#ifdef SCREEN_BRIGHTNESS
    GUI_CMD_LINK ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'B','r','i','g','h','t','n','e','s','s' , GUI_CMD_NULL ,
    ALIGN_RIGHT ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_BRIGHTNESS ) ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
    GUI_CMD_LINK ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'C','o','n','t','r','a','s','t' , GUI_CMD_NULL ,
    ALIGN_RIGHT ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_CONTRAST ) ,
    GUI_CMD_LINK_END ,
    GUI_CMD_LINE_END ,
#endif // SCREEN_CONTRAST
    GUI_CMD_LINK ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'T','i','m','e','r' , GUI_CMD_NULL ,
    ALIGN_RIGHT ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_TIMER ) ,
    GUI_CMD_LINK_END ,
    GUI_CMD_PAGE_END
};

#ifdef GPS_PRESENT
static const uint8_t Page_SettingsGPS[] =
{
    GUI_CMD_TITLE , 'G','P','S',' ','S','e','t','t','i','n','g','s' , GUI_CMD_NULL ,
    GUI_CMD_GOTO_TEXT_LINE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_LOAD_STYLE , ST_VAL( GUI_STYLE_1 ) ,
    ALIGN_LEFT ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'G','P','S',' ','E','n','a','b','l','e','d' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_GPS_ENABLED ) ,
    GUI_CMD_LINE_END ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'G','P','S',' ','S','e','t',' ','T','i','m','e' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_GPS_SET_TIME ) ,
    GUI_CMD_LINE_END ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'U','T','C',' ','T','i','m','e','z','o','n','e', GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_GPS_TIME_ZONE ) ,
    GUI_CMD_PAGE_END
};

#endif // GPS_PRESENT

static const uint8_t Page_SettingsRadio[] =
{
    GUI_CMD_TITLE , 'R','a','d','i','o',' ','S','e','t','t','i','n','g','s' , GUI_CMD_NULL ,
    GUI_CMD_GOTO_TEXT_LINE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_LOAD_STYLE , ST_VAL( GUI_STYLE_1 ) ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'O','f','f','s','e','t', GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_RADIO_OFFSET ) ,
    GUI_CMD_LINE_END ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'D','i','r','e','c','t','i','o','n', GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_RADIO_DIRECTION ) ,
    GUI_CMD_LINE_END ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'S','t','e','p' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_RADIO_STEP ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_SettingsM17[] =
{
    GUI_CMD_TITLE , 'M','1','7',' ','S','e','t','t','i','n','g','s' , GUI_CMD_NULL ,
    GUI_CMD_GOTO_TEXT_LINE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_LOAD_STYLE , ST_VAL( GUI_STYLE_1 ) ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'C','a','l','l','s','i','g','n' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_M17_CALLSIGN ) ,
    GUI_CMD_LINE_END ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'C','A','N' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_M17_CAN ) ,
    GUI_CMD_LINE_END ,
    ALIGN_LEFT , GUI_CMD_TEXT ,
     'C','A','N',' ','R','X',' ','C','h','e','c','k' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_M17_CAN_RX_CHECK ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_SettingsVoice[] =
{
    GUI_CMD_TITLE , 'V','o','i','c','e' , GUI_CMD_NULL ,
    GUI_CMD_GOTO_TEXT_LINE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_LOAD_STYLE , ST_VAL( GUI_STYLE_1 ) ,
    ALIGN_LEFT , GUI_CMD_TEXT , 'V','o','i','c','e' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_LEVEL ) ,
    GUI_CMD_LINE_END ,
    ALIGN_LEFT , GUI_CMD_TEXT , 'P','h','o','n','e','t','i','c' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_PHONETIC ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuBackupRestore[] =
{
    GUI_CMD_TITLE , 'B','a','c','k','u','p',' ','A','n','d',' ','R','e','s','t','o','r','e' , GUI_CMD_NULL ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_BACKUP_RESTORE ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuBackup[] =
{
    GUI_CMD_TITLE , 'F','l','a','s','h',' ','B','a','c','k','u','p' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'C','o','n','n','e','c','t',' ','t','o',' ','R','T','X',' ','T','o','o','l' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'T','o',' ','B','a','c','k','u','p',' ','F','l','a','s','h',' ','A','n','d' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'P','r','e','s','s',' ','P','T','T',' ','t','o',' ','S','t','a','r','t' , GUI_CMD_NULL ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuRestore[] =
{
    GUI_CMD_TITLE , 'F','l','a','s','h',' ','R','e','s','t','o','r','e' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'C','o','n','n','e','c','t',' ','t','o',' ','R','T','X',' ','T','o','o','l' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'T','o',' ','R','e','s','t','o','r','e',' ','F','l','a','s','h',' ','A','n','d' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'P','r','e','s','s',' ','P','T','T',' ','t','o',' ','S','t','a','r','t' , GUI_CMD_NULL ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_MenuInfo[] =
{
    ALIGN_LEFT ,
    GUI_CMD_TEXT , 'B','a','t','.',' ','V','o','l','t','a','g','e' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_BATTERY_VOLTAGE ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'B','a','t','.',' ','C','h','a','r','g','e' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_BATTERY_CHARGE ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'R','S','S','I' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_RSSI ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'U','s','e','d',' ','h','e','a','p' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_USED_HEAP ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'B','a','n','d' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_BAND ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'V','H','F' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_VHF ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'U','H','F' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_UHF ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'H','W',' ','V','e','r','s','i','o','n' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_HW_VERSION ) ,
    GUI_CMD_LINE_END ,
#ifdef PLATFORM_TTWRPLUS
    GUI_CMD_TEXT , 'R','a','d','i','o' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_RADIO ) ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'R','a','d','i','o',' ','F','W' , GUI_CMD_NULL ,
    ALIGN_RIGHT , GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_RADIO_FW ) ,
#endif // PLATFORM_TTWRPLUS
    GUI_CMD_PAGE_END
};

static const uint8_t Page_SettingsResetToDefaults[] =
{
    GUI_CMD_TITLE , 'R','e','s','e','t',' ','t','o',' ','D','e','f','a','u','l','t','s' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'T','o',' ','R','e','s','e','t' , GUI_CMD_NULL ,
    GUI_CMD_LINE_END ,
    GUI_CMD_TEXT , 'P','r','e','s','s',' ','E','n','t','e','r',' ','T','w','i','c','e' , GUI_CMD_NULL ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_LowBat[] =
{
    GUI_CMD_TITLE , 'L','o','w',' ','B' ,'a' ,'t' ,'t' ,'e' ,'r','y' , GUI_CMD_NULL ,
    ALIGN_CENTER ,
    GUI_CMD_GOTO_TEXT_LINE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_LOAD_STYLE , ST_VAL( GUI_STYLE_1 ) ,
    GUI_CMD_TEXT , 'F','o','r',' ','E','m','e','r','g','e','n','c','y',' ','U','s','e' , GUI_CMD_NULL ,
    GUI_CMD_GOTO_TEXT_LINE , ST_VAL( GUI_LINE_2 ) ,
    GUI_CMD_LOAD_STYLE , ST_VAL( GUI_STYLE_2 ) ,
    GUI_CMD_TEXT , 'P','r','e','s','s',' ','A','n','y',' ','B','u','t','t','o','n' , GUI_CMD_NULL ,
    GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_LOW_BATTERY ) ,
    GUI_CMD_PAGE_END
};

static const uint8_t Page_About[] =
{
    GUI_CMD_TITLE , 'A','b','o','u','t' , GUI_CMD_NULL ,
    GUI_CMD_GOTO_TEXT_LINE , ST_VAL( GUI_LINE_1 ) ,
    GUI_CMD_LOAD_STYLE , ST_VAL( GUI_STYLE_1 ) ,
    ALIGN_LEFT ,
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
    ALIGN_LEFT ,
    GUI_CMD_TEXT ,
     'P','a','g','e',' ',
     'S','t','u','b','b','e','d', GUI_CMD_NULL ,
    GUI_CMD_PAGE_END
};

#define PAGE_REF( loc )    loc

const uint8_t* uiPageTable[ PAGE_NUM_OF ] =
{
    PAGE_REF( Page_MainVFO                 ) , // PAGE_MAIN_VFO
    PAGE_REF( Page_MainInput               ) , // PAGE_MAIN_VFO_INPUT
    PAGE_REF( Page_MainMem                 ) , // PAGE_MAIN_MEM
    PAGE_REF( Page_ModeVFO                 ) , // PAGE_MODE_VFO
    PAGE_REF( Page_ModeMem                 ) , // PAGE_MODE_MEM
    PAGE_REF( Page_MenuTop                 ) , // PAGE_MENU_TOP
    PAGE_REF( Page_MenuBank                ) , // PAGE_MENU_BANK
    PAGE_REF( Page_MenuChannel             ) , // PAGE_MENU_CHANNEL
    PAGE_REF( Page_MenuContact             ) , // PAGE_MENU_CONTACTS
    PAGE_REF( Page_MenuGPS                 ) , // PAGE_MENU_GPS
    PAGE_REF( Page_MenuSettings            ) , // PAGE_MENU_SETTINGS
    PAGE_REF( Page_MenuBackupRestore       ) , // PAGE_MENU_BACKUP_RESTORE
    PAGE_REF( Page_MenuBackup              ) , // PAGE_MENU_BACKUP
    PAGE_REF( Page_MenuRestore             ) , // PAGE_MENU_RESTORE
    PAGE_REF( Page_MenuInfo                ) , // PAGE_MENU_INFO
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
