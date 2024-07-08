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
//    GOTO_TEXT_LINE( GUI_LINE_TOP ) ,
//    LOAD_STYLE( GUI_STYLE_TOP ) ,
//    ALIGN_CENTER ,
    EVENT_START( EVENT_TYPE_STATUS , EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      VALUE_DSP( TIME ) ,
    EVENT_END ,
    EVENT_START( EVENT_TYPE_STATUS , ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_BATTERY ) ) ,
      VALUE_DSP( BATTERY_LEVEL ) ,
    EVENT_END ,
    EVENT_START( EVENT_TYPE_STATUS , ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_DISPLAY_TIME_TICK ) ) ,
      VALUE_DSP( LOCK_STATE ) ,
    EVENT_END ,
    VALUE_DSP( MODE_INFO ) ,
    VALUE_DSP( FREQUENCY ) ,
    EVENT_START( EVENT_TYPE_STATUS , ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_RSSI ) ) ,
      VALUE_DSP( RSSI_METER ) ,
    EVENT_END ,
    PAGE_END
};

static const uint8_t Page_MainInput[] =
{
    EVENT_START( EVENT_TYPE_STATUS , EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      VALUE_DSP( TIME ) ,
    EVENT_END ,
    EVENT_START( EVENT_TYPE_STATUS , ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_BATTERY ) ) ,
      VALUE_DSP( BATTERY_LEVEL ) ,
    EVENT_END ,
    EVENT_START( EVENT_TYPE_STATUS , ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_DISPLAY_TIME_TICK ) ) ,
      VALUE_DSP( LOCK_STATE ) ,
    EVENT_END ,
    VALUE_INP( VFO_MIDDLE_INPUT ) ,
    EVENT_START( EVENT_TYPE_STATUS , ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_RSSI ) ) ,
      VALUE_DSP( RSSI_METER ) ,
    EVENT_END ,
    PAGE_END

};

static const uint8_t Page_MainMem[] =
{
/*
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
    FG_COLOR( WHITE ) ,
    ADD_TO_X( 20 ) ,
    ADD_TO_Y( 20 ) ,
    CIRCLE( 15 ) ,

    FG_COLOR( WHITE ) ,
    ADD_TO_X( -30 ) ,
    ADD_TO_Y( -60 ) ,
    RECT( 25 , 20 ) ,
//    ADD_TO_Y( -20 ) ,
    FG_COLOR( RED ) ,
    ADD_TO_X( 1 ) ,
    ADD_TO_Y( 1 ) ,
    RECT_FILL( 23 , 18 ) ,

    GUI_CMD_PAGE_END ,//@@@KL
*/
    ALIGN_CENTER ,
    GUI_CMD_EVENT_START , ST_VAL( EVENT_TYPE_STATUS ) , ST_VAL( EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      BG_COLOR( RED ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_DSP_TIME ) ,
    GUI_CMD_EVENT_END ,
    EVENT_START( EVENT_TYPE_STATUS , ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_BATTERY ) ) ,
      ALIGN_RIGHT ,
      VALUE_DSP( BATTERY_LEVEL ) ,
    EVENT_END ,
    EVENT_START( EVENT_TYPE_STATUS , ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_DISPLAY_TIME_TICK ) ) ,
      VALUE_DSP( LOCK_STATE ) ,
    EVENT_END ,
    LINE_END ,
    ALIGN_CENTER ,
    VALUE_DSP( MODE_INFO ) ,
    LINE_END ,
    VALUE_DSP( BANK_CHANNEL ) ,
    LINE_END ,
    VALUE_DSP( FREQUENCY ) ,
    LINE_END ,
    EVENT_START( EVENT_TYPE_STATUS , ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_RSSI ) ) ,
      VALUE_DSP( RSSI_METER ) ,
    EVENT_END ,
    ON_EVENT_KBD_ENTER_GOTO_PAGE( PAGE_MENU_TOP ) ,
    PAGE_END
};

static const uint8_t Page_ModeVFO[] =
{
    TITLE , 'W','T','D',':',' ','M','o','d','e','V','F','O', NULL_CH ,
    PAGE_END
};

static const uint8_t Page_ModeMem[] =
{
    TITLE , 'W','T','D',':',' ','M','o','d','e','M','e','m', NULL_CH ,
    PAGE_END
};

static const uint8_t Page_MenuTop[] =
{
    TITLE , 'M','e','n','u' , NULL_CH ,
    GOTO_TEXT_LINE( GUI_LINE_1 ) ,
    LOAD_STYLE( GUI_STYLE_1 ) ,
    ALIGN_LEFT ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_MENU_TOP_LIST , 3 ) ,
    EVENT_END ,
    PAGE_END
};

static const uint8_t Page_MenuTopList[] =
{
    TEXT , 'B','a','n','k','s' , NULL_CH ,             LINK( PAGE_MENU_BANK )     , LINE_END ,
    TEXT , 'C','h','a','n','n','e','l','s' , NULL_CH , LINK( PAGE_MENU_CHANNEL )  , LINE_END ,
    TEXT , 'C','o','n','t','a','c','t','s' , NULL_CH , LINK( PAGE_MENU_CONTACTS ) , LINE_END ,
#ifdef GPS_PRESENT
    TEXT , 'G','P','S' , NULL_CH ,                     LINK( PAGE_MENU_GPS )      , LINE_END ,
#endif // RTC_PRESENT
    TEXT , 'S','e','t','t','i','n','g','s' , NULL_CH , LINK( PAGE_MENU_SETTINGS ) , LINE_END ,
    TEXT , 'I','n','f','o' , NULL_CH ,                 LINK( PAGE_MENU_INFO )     , LINE_END ,
    TEXT , 'A','b','o','u','t' , NULL_CH ,             LINK( PAGE_ABOUT )         , PAGE_END
};

static const uint8_t Page_MenuBank[] =
{
    TITLE , 'B','a','n','k' , NULL_CH ,
    VALUE_DSP( BANKS ) ,
    PAGE_END
};

static const uint8_t Page_MenuChannel[] =
{
    TITLE , 'C','h','a','n','n','e','l' , NULL_CH ,
    VALUE_DSP( CHANNELS ) ,
    PAGE_END
};

static const uint8_t Page_MenuContact[] =
{
    TITLE , 'C','o','n','t','a','c','t' , NULL_CH ,
    VALUE_DSP( CONTACTS ) ,
    PAGE_END
};

static const uint8_t Page_MenuGPS[] =
{
    TITLE , 'G','P','S' , NULL_CH ,
    VALUE_DSP( GPS ) ,
    PAGE_END
};

static const uint8_t Page_MenuSettings[] =
{
    TITLE , 'S','e','t','t','i','n','g','s' , NULL_CH ,
    GOTO_TEXT_LINE( GUI_LINE_1 ) ,
    LOAD_STYLE( GUI_STYLE_1 ) ,
    ALIGN_LEFT ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_MENU_SETTINGS_LIST , 5 ) ,
    EVENT_END ,
    PAGE_END
};

static const uint8_t Page_MenuSettingsList[] =
{
    TEXT , 'D','i','s','p','l','a','y' , NULL_CH ,                                     LINK( PAGE_SETTINGS_DISPLAY )           , LINE_END ,
#ifdef RTC_PRESENT
    TEXT , 'T','i','m','e',' ','&',' ','D','a','t','e' , NULL_CH ,                     LINK( PAGE_SETTINGS_TIMEDATE )          , LINE_END ,
#endif // RTC_PRESENT
#ifdef GPS_PRESENT
    TEXT , 'G','P','S' , NULL_CH ,                                                     LINK( PAGE_SETTINGS_GPS )               , LINE_END ,
#endif // GPS_PRESENT
    TEXT , 'R','a','d','i','o' , NULL_CH ,                                             LINK( PAGE_SETTINGS_RADIO )             , LINE_END ,
    TEXT , 'M','1','7' , NULL_CH ,                                                     LINK( PAGE_SETTINGS_M17 )               , LINE_END ,
    TEXT , 'A','c','c','e','s','s','i','b','i','l','i','t','y' , NULL_CH ,             LINK( PAGE_SETTINGS_VOICE )             , LINE_END ,
    TEXT , 'D','e','f','a','u','l','t',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINK( PAGE_SETTINGS_RESET_TO_DEFAULTS ) , PAGE_END
};

static const uint8_t Page_SettingsTimeDate[] =
{
    TITLE , 'T','i','m','e',' ','&',' ','D','a','t','e', NULL_CH ,
    LINE_END ,
    LINE_END ,
    ALIGN_CENTER ,
      VALUE_DSP( DATE ) ,
    LINE_END ,
    LINE_END ,
    ALIGN_CENTER ,
    EVENT_START( EVENT_TYPE_STATUS , EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      VALUE_DSP( TIME ) ,
    EVENT_END ,


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
    PAGE_END
};

static const uint8_t Page_SettingsTimeDateSet[] =
{
    TITLE , 'W','T','D',':',' ','S','T','D','S','e','t', NULL_CH ,
    PAGE_END
};

static const uint8_t Page_SettingsDisplay[] =
{
    TITLE , 'D','i','s','p','l','a','y' , NULL_CH ,
    GOTO_TEXT_LINE( GUI_LINE_1 ) ,
    LOAD_STYLE( GUI_STYLE_1 ) ,
#ifdef SCREEN_BRIGHTNESS
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_SETTINGS_DISPLAY_LIST , 5 ) ,
    EVENT_END ,
    PAGE_END
};

static const uint8_t Page_SettingsDisplayList[] =
{
    ALIGN_LEFT , TEXT , 'B','r','i','g','h','t','n','e','s','s' , NULL_CH , ALIGN_RIGHT , VALUE_DSP( BRIGHTNESS ) , LINE_END ,
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
    ALIGN_LEFT , TEXT , 'C','o','n','t','r','a','s','t' , NULL_CH ,         ALIGN_RIGHT , VALUE_DSP( CONTRAST )   , LINE_END ,
#endif // SCREEN_CONTRAST
    ALIGN_LEFT , TEXT , 'T','i','m','e','r' , NULL_CH ,                     ALIGN_RIGHT , VALUE_DSP( TIMER )      , PAGE_END
};

#ifdef GPS_PRESENT
static const uint8_t Page_SettingsGPS[] =
{
    TITLE , 'G','P','S',' ','S','e','t','t','i','n','g','s' , NULL_CH ,
    GOTO_TEXT_LINE( GUI_LINE_1 ) ,
    LOAD_STYLE( GUI_STYLE_1 ) ,
    ALIGN_LEFT ,
    ALIGN_LEFT , TEXT ,
     'G','P','S',' ','E','n','a','b','l','e','d' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( GPS_ENABLED ) ,
    LINE_END ,
    ALIGN_LEFT , TEXT ,
     'G','P','S',' ','S','e','t',' ','T','i','m','e' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( GPS_SET_TIME ) ,
    LINE_END ,
    ALIGN_LEFT , TEXT ,
     'U','T','C',' ','T','i','m','e','z','o','n','e', NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( GPS_TIME_ZONE ) ,
    PAGE_END
};

#endif // GPS_PRESENT

static const uint8_t Page_SettingsRadio[] =
{
    TITLE , 'R','a','d','i','o',' ','S','e','t','t','i','n','g','s' , NULL_CH ,
    GOTO_TEXT_LINE( GUI_LINE_1 ) ,
    LOAD_STYLE( GUI_STYLE_1 ) ,
    ALIGN_LEFT , TEXT ,
     'O','f','f','s','e','t', NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( RADIO_OFFSET ) ,
    LINE_END ,
    ALIGN_LEFT , TEXT ,
     'D','i','r','e','c','t','i','o','n', NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( RADIO_DIRECTION ) ,
    LINE_END ,
    ALIGN_LEFT , TEXT ,
     'S','t','e','p' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( RADIO_STEP ) ,
    PAGE_END
};

static const uint8_t Page_SettingsM17[] =
{
    TITLE , 'M','1','7',' ','S','e','t','t','i','n','g','s' , NULL_CH ,
    GOTO_TEXT_LINE( GUI_LINE_1 ) ,
    LOAD_STYLE( GUI_STYLE_1 ) ,
    ALIGN_LEFT , TEXT ,
     'C','a','l','l','s','i','g','n' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( M17_CALLSIGN ) ,
    LINE_END ,
    ALIGN_LEFT , TEXT ,
     'C','A','N' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( M17_CAN ) ,
    LINE_END ,
    ALIGN_LEFT , TEXT ,
     'C','A','N',' ','R','X',' ','C','h','e','c','k' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( M17_CAN_RX_CHECK ) ,
    PAGE_END
};

static const uint8_t Page_SettingsVoice[] =
{
    TITLE , 'V','o','i','c','e' , NULL_CH ,
    GOTO_TEXT_LINE( GUI_LINE_1 ) ,
    LOAD_STYLE( GUI_STYLE_1 ) ,
    ALIGN_LEFT , TEXT , 'V','o','i','c','e' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( LEVEL ) ,
    LINE_END ,
    ALIGN_LEFT , TEXT , 'P','h','o','n','e','t','i','c' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( PHONETIC ) ,
    PAGE_END
};

static const uint8_t Page_MenuBackupRestore[] =
{
    TITLE , 'B','a','c','k','u','p',' ','A','n','d',' ','R','e','s','t','o','r','e' , NULL_CH ,
    VALUE_DSP( BACKUP_RESTORE ) ,
    PAGE_END
};

static const uint8_t Page_MenuBackup[] =
{
    TITLE , 'F','l','a','s','h',' ','B','a','c','k','u','p' , NULL_CH ,
    LINE_END ,
    TEXT , 'C','o','n','n','e','c','t',' ','t','o',' ','R','T','X',' ','T','o','o','l' , NULL_CH ,
    LINE_END ,
    TEXT , 'T','o',' ','B','a','c','k','u','p',' ','F','l','a','s','h',' ','A','n','d' , NULL_CH ,
    LINE_END ,
    TEXT , 'P','r','e','s','s',' ','P','T','T',' ','t','o',' ','S','t','a','r','t' , NULL_CH ,
    PAGE_END
};

static const uint8_t Page_MenuRestore[] =
{
    TITLE , 'F','l','a','s','h',' ','R','e','s','t','o','r','e' , NULL_CH ,
    LINE_END ,
    TEXT , 'C','o','n','n','e','c','t',' ','t','o',' ','R','T','X',' ','T','o','o','l' , NULL_CH ,
    LINE_END ,
    TEXT , 'T','o',' ','R','e','s','t','o','r','e',' ','F','l','a','s','h',' ','A','n','d' , NULL_CH ,
    LINE_END ,
    TEXT , 'P','r','e','s','s',' ','P','T','T',' ','t','o',' ','S','t','a','r','t' , NULL_CH ,
    PAGE_END
};

static const uint8_t Page_MenuInfo[] =
{
    ALIGN_LEFT ,
    TEXT , 'B','a','t','.',' ','V','o','l','t','a','g','e' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( BATTERY_VOLTAGE ) ,
    LINE_END ,
    TEXT , 'B','a','t','.',' ','C','h','a','r','g','e' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( BATTERY_CHARGE ) ,
    LINE_END ,
    TEXT , 'R','S','S','I' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( RSSI ) ,
    LINE_END ,
    TEXT , 'U','s','e','d',' ','h','e','a','p' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( USED_HEAP ) ,
    LINE_END ,
    TEXT , 'B','a','n','d' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( BAND ) ,
    LINE_END ,
    TEXT , 'V','H','F' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( VHF ) ,
    LINE_END ,
    TEXT , 'U','H','F' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( UHF ) ,
    LINE_END ,
    TEXT , 'H','W',' ','V','e','r','s','i','o','n' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( HW_VERSION ) ,
    LINE_END ,
#ifdef PLATFORM_TTWRPLUS
    TEXT , 'R','a','d','i','o' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( RADIO ) ,
    LINE_END ,
    TEXT , 'R','a','d','i','o',' ','F','W' , NULL_CH ,
    ALIGN_RIGHT , VALUE_DSP( RADIO_FW ) ,
#endif // PLATFORM_TTWRPLUS
    PAGE_END
};

static const uint8_t Page_SettingsResetToDefaults[] =
{
    TITLE , 'R','e','s','e','t',' ','t','o',' ','D','e','f','a','u','l','t','s' , NULL_CH ,
    LINE_END ,
    TEXT , 'T','o',' ','R','e','s','e','t' , NULL_CH ,
    LINE_END ,
    TEXT , 'P','r','e','s','s',' ','E','n','t','e','r',' ','T','w','i','c','e' , NULL_CH ,
    PAGE_END
};

static const uint8_t Page_LowBat[] =
{
    TITLE , 'L','o','w',' ','B' ,'a' ,'t' ,'t' ,'e' ,'r','y' , NULL_CH ,
    ALIGN_CENTER ,
    GOTO_TEXT_LINE( GUI_LINE_1 ) ,
    LOAD_STYLE( GUI_STYLE_1 ) ,
    TEXT , 'F','o','r',' ','E','m','e','r','g','e','n','c','y',' ','U','s','e' , NULL_CH ,
    GOTO_TEXT_LINE( GUI_LINE_2 ) ,
    LOAD_STYLE( GUI_STYLE_2 ) ,
    TEXT , 'P','r','e','s','s',' ','A','n','y',' ','B','u','t','t','o','n' , NULL_CH ,
    VALUE_DSP( LOW_BATTERY ) ,
    PAGE_END
};

static const uint8_t Page_About[] =
{
    TITLE , 'A','b','o','u','t' , NULL_CH ,
    GOTO_TEXT_LINE( GUI_LINE_1 ) ,
    LOAD_STYLE( GUI_STYLE_1 ) ,
    ALIGN_LEFT ,
    TEXT ,
     'A','u','t','h','o','r','s' , NULL_CH ,
    LINE_END ,
    TEXT ,
     'N','i','c','c','o','l','o',' ',
     'I','U','2','K','I','N' , NULL_CH ,
    LINE_END ,
    TEXT ,
     'S','i','l','v','a','n','o',' ',
     'I','U','2','K','W','O' , NULL_CH ,
    LINE_END ,
    TEXT ,
     'F','e','d','e','r','i','c','o',' ',
     'I','U','2','N','U','O' , NULL_CH ,
    LINE_END ,
    TEXT ,
     'F','r','e','d',' ',
     'I','U','2','N','R','O' , NULL_CH ,
    LINE_END ,
    TEXT ,
     'K','i','m',' ',
     'V','K','6','K','L', NULL_CH ,
    PAGE_END
};

static const uint8_t Page_Stubbed[] =
{
    ALIGN_LEFT ,
    TEXT ,
     'P','a','g','e',' ',
     'S','t','u','b','b','e','d', NULL_CH ,
    PAGE_END
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
    PAGE_REF( Page_MenuTopList             ) , // PAGE_MENU_TOP_LIST
    PAGE_REF( Page_MenuBank                ) , // PAGE_MENU_BANK
    PAGE_REF( Page_MenuChannel             ) , // PAGE_MENU_CHANNEL
    PAGE_REF( Page_MenuContact             ) , // PAGE_MENU_CONTACTS
    PAGE_REF( Page_MenuGPS                 ) , // PAGE_MENU_GPS
    PAGE_REF( Page_MenuSettings            ) , // PAGE_MENU_SETTINGS
    PAGE_REF( Page_MenuSettingsList        ) , // PAGE_MENU_SETTINGS_LIST
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
