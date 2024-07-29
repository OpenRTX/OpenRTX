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
#include "ui_list_display.h"
#include "ui_value_display.h"
#include "ui_value_input.h"
#include "ui_scripts.h"

// PAGE_MAIN_VFO 0x00
static const uint8_t Page_MainVFO[] =
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
    VALUE_DSP( MODE_INFO ) ,
    VALUE_DSP( FREQUENCY ) ,
    EVENT_START( EVENT_TYPE_STATUS , ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_RSSI ) ) ,
      VALUE_DSP( RSSI_METER ) ,
    EVENT_END ,
    PAGE_END
};

// PAGE_MAIN_VFO_INPUT 0x01
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
    VALUE_INP( FREQUENCY ) ,
    EVENT_START( EVENT_TYPE_STATUS , ( EVENT_STATUS_DEVICE_TIME_TICK | EVENT_STATUS_RSSI ) ) ,
      VALUE_DSP( RSSI_METER ) ,
    EVENT_END ,
    PAGE_END
};

// PAGE_MAIN_MEM 0x02
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
    PAGE_TREE_TOP ,
    ALIGN_CENTER ,
    EVENT_START( EVENT_TYPE_STATUS , EVENT_STATUS_DISPLAY_TIME_TICK ) ,
      BG_COLOR( RED ) ,
      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_TIME ) ,
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
    ON_EVENT_KEY_ENTER_GOTO_PAGE( PAGE_MENU_TOP ) ,
    PAGE_END
};

// PAGE_MODE_VFO 0x03
static const uint8_t Page_ModeVFO[] =
{
    TITLE , 'W','T','D',':',' ','M','o','d','e','V','F','O', NULL_CH ,
    PAGE_END
};

// PAGE_MODE_MEM 0x04
static const uint8_t Page_ModeMem[] =
{
    TITLE , 'W','T','D',':',' ','M','o','d','e','M','e','m', NULL_CH ,
    PAGE_END
};

// PAGE_MENU_TOP 0x05
static const uint8_t Page_MenuTop[] =
{
    TITLE , 'M','e','n','u' , NULL_CH , LINE_END ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_MENU_TOP_LIST , LIST_DATA_SOURCE_SCRIPT , 5 ) ,
    EVENT_END ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_MENU_TOP_LIST 0x06
static const uint8_t Page_MenuTop_List[] =
{
    LIST_ELEMENT , TEXT , 'B','a','n','k','s' , NULL_CH ,             LINK( PAGE_MENU_BANK )     , LINE_END ,
    LIST_ELEMENT , TEXT , 'C','h','a','n','n','e','l','s' , NULL_CH , LINK( PAGE_MENU_CHANNEL )  , LINE_END ,
    LIST_ELEMENT , TEXT , 'C','o','n','t','a','c','t','s' , NULL_CH , LINK( PAGE_MENU_CONTACTS ) , LINE_END ,
#ifdef GPS_PRESENT
    LIST_ELEMENT , TEXT , 'G','P','S' , NULL_CH ,                     LINK( PAGE_MENU_GPS )      , LINE_END ,
#endif // RTC_PRESENT
    LIST_ELEMENT , TEXT , 'S','e','t','t','i','n','g','s' , NULL_CH , LINK( PAGE_MENU_SETTINGS ) , LINE_END ,
    LIST_ELEMENT , TEXT , 'I','n','f','o' , NULL_CH ,                 LINK( PAGE_MENU_INFO )     , LINE_END ,
    LIST_ELEMENT , TEXT , 'A','b','o','u','t' , NULL_CH ,             LINK( PAGE_ABOUT )         , PAGE_END
};

// PAGE_MENU_BANK 0x07
static const uint8_t Page_MenuBank[] =
{
    TITLE , 'B','a','n','k' , NULL_CH , LINE_END ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_MENU_BANK_LIST , LIST_DATA_SOURCE_BANKS , 5 ) ,
    EVENT_END ,
    ON_EVENT_KEY_ENTER_GOTO_PAGE( PAGE_MAIN_MEM ) ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_MENU_BANK_LIST 0x08
static const uint8_t Page_MenuBank_List[] =
{
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , PAGE_END
};

// PAGE_MENU_CHANNEL 0x09
static const uint8_t Page_MenuChannel[] =
{
    TITLE , 'C','h','a','n','n','e','l' , NULL_CH , LINE_END ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_MENU_CHANNEL_LIST , LIST_DATA_SOURCE_CHANNELS , 5 ) ,
    EVENT_END ,
    ON_EVENT_KEY_ENTER_GOTO_PAGE( PAGE_MAIN_MEM ) ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_MENU_CHANNEL_LIST 0x0A
static const uint8_t Page_MenuChannel_List[] =
{
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , PAGE_END
};

// PAGE_MENU_CONTACTS 0x0B
static const uint8_t Page_MenuContact[] =
{
    TITLE , 'C','o','n','t','a','c','t' , NULL_CH , LINE_END ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_MENU_CONTACTS_LIST , LIST_DATA_SOURCE_CONTACTS , 5 ) ,
    EVENT_END ,
    ON_EVENT_KEY_ENTER_GOTO_PAGE( PAGE_MAIN_MEM ) ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_MENU_CONTACTS_LIST 0x0C
static const uint8_t Page_MenuContact_List[] =
{
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , LINE_END ,
    LIST_ELEMENT , PAGE_END
};

// PAGE_MENU_GPS 0x0D
static const uint8_t Page_MenuGPS[] =
{
    TITLE , 'G','P','S' , NULL_CH , LINE_END ,
    VALUE_DSP( GPS ) ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_MENU_SETTINGS 0x0E
static const uint8_t Page_MenuSettings[] =
{
    TITLE , 'S','e','t','t','i','n','g','s' , NULL_CH ,
    GOTO_TEXT_LINE( GUI_LINE_1 ) ,
    LOAD_STYLE( GUI_STYLE_1 ) ,
    ALIGN_LEFT ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_MENU_SETTINGS_LIST , LIST_DATA_SOURCE_SCRIPT , 5 ) ,
    EVENT_END ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_MENU_SETTINGS_LIST 0x0F
static const uint8_t Page_MenuSettings_List[] =
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

// PAGE_SETTINGS_DISPLAY 0x10
static const uint8_t Page_SettingsDisplay[] =
{
    TITLE , 'D','i','s','p','l','a','y' , NULL_CH , LINE_END ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_SETTINGS_DISPLAY_LIST , LIST_DATA_SOURCE_SCRIPT , 3 ) ,
    EVENT_END ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_DISPLAY_LIST 0x11
static const uint8_t Page_SettingsDisplay_List[] =
{
#ifdef SCREEN_BRIGHTNESS
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'B','r','i','g','h','t','n','e','s','s' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( BRIGHTNESS ) ,
      LINK( PAGE_SETTINGS_SET_BRIGHTNESS ) , LINE_END ,
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'C','o','n','t','r','a','s','t' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( CONTRAST ) ,
      LINK( PAGE_SETTINGS_SET_CONTRAST ) , LINE_END ,
#endif // SCREEN_CONTRAST
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'T','i','m','e','r' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( TIMER ) ,
      LINK( PAGE_SETTINGS_SET_TIMER ) ,
    PAGE_END
};

#ifdef SCREEN_BRIGHTNESS
// PAGE_SETTINGS_SET_BRIGHTNESS 0x--
static const uint8_t Page_Settings_Set_Brightness[] =
{
    TITLE , 'D','i','s','p','l','a','y' , NULL_CH , LINE_END ,
    ALIGN_LEFT , TEXT , 'B','r','i','g','h','t','n','e','s','s' , NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( BRIGHTNESS ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};
#endif // SCREEN_BRIGHTNESS

#ifdef SCREEN_CONTRAST
// PAGE_SETTINGS_SET_CONTRAST 0x--
static const uint8_t Page_Settings_Set_Contrast[] =
{
    TITLE , 'D','i','s','p','l','a','y' , NULL_CH , LINE_END ,
    ALIGN_LEFT , TEXT , 'C','o','n','t','r','a','s','t' , NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( CONTRAST ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};
#endif // SCREEN_CONTRAST

// PAGE_SETTINGS_SET_TIMER 0x12
static const uint8_t Page_Settings_Set_Timer[] =
{
    TITLE , 'D','i','s','p','l','a','y' , NULL_CH , LINE_END ,
    ALIGN_LEFT , TEXT , 'T','i','m','e','r' , NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( TIMER ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_TIMEDATE 0x13
static const uint8_t Page_SettingsTimeDate[] =
{
    TITLE , 'T','i','m','e',' ','&',' ','D','a','t','e', NULL_CH , LINE_END ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_SETTINGS_TIMEDATE_LIST , LIST_DATA_SOURCE_SCRIPT , 2 ) ,
    EVENT_END ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_TIMEDATE_LIST 0x14
static const uint8_t Page_SettingsTimeDate_List[] =
{
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'D','a','t','e' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( DATE ) ,
      LINK( PAGE_SETTINGS_SET_DATE ) ,
      LINE_END ,
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'T','i','m','e' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( TIME ) ,
      LINK( PAGE_SETTINGS_SET_TIME ) ,
    PAGE_END
};

// PAGE_SETTINGS_SET_DATE 0x15
static const uint8_t Page_Settings_Set_Date[] =
{
    TITLE , 'D','a','t','e', NULL_CH , LINE_END ,
    ALIGN_CENTER , VALUE_INP( DATE ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_SET_TIME 0x16
static const uint8_t Page_Settings_Set_Time[] =
{
    TITLE , 'T','i','m','e', NULL_CH , LINE_END ,
    ALIGN_CENTER , VALUE_INP( TIME ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

#ifdef GPS_PRESENT
// PAGE_SETTINGS_GPS 0x17
static const uint8_t Page_SettingsGPS[] =
{
    TITLE , 'G','P','S',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_SETTINGS_GPS_LIST , LIST_DATA_SOURCE_SCRIPT , 3 ) ,
    EVENT_END ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_GPS_LIST 0x18
static const uint8_t Page_SettingsGPS_List[] =
{
    LIST_ELEMENT ,
      ALIGN_LEFT  , TEXT , 'G','P','S',' ','E','n','a','b','l','e','d' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( GPS_ENABLED ) ,
      LINK( PAGE_SETTINGS_GPS_SET_ENABLED ) ,
      LINE_END ,
    LIST_ELEMENT ,
      ALIGN_LEFT  , TEXT , 'G','P','S',' ','S','e','t',' ','T','i','m','e' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( GPS_TIME ) ,
      LINK( PAGE_SETTINGS_GPS_SET_TIME ) ,
      LINE_END ,
    LIST_ELEMENT ,
      ALIGN_LEFT  , TEXT , 'U','T','C',' ','T','i','m','e','z','o','n','e', NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( GPS_TIME_ZONE ) ,
      LINK( PAGE_SETTINGS_GPS_SET_TIMEZONE ) ,
    PAGE_END
};

// PAGE_SETTINGS_GPS_SET_ENABLED 0x19
static const uint8_t Page_SettingsGPS_Set_Enabled[] =
{
    TITLE , 'G','P','S',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    ALIGN_LEFT ,
    TEXT , 'G','P','S',' ','E','n','a','b','l','e','d' , NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( GPS_ENABLED ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_GPS_SET_TIME 0x1A
static const uint8_t Page_SettingsGPS_Set_Time[] =
{
    TITLE , 'G','P','S',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    ALIGN_LEFT ,
    TEXT , 'G','P','S',' ','S','e','t',' ','T','i','m','e' , NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( GPS_TIME ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_GPS_SET_TIMEZONE 0x1B
static const uint8_t Page_SettingsGPS_Set_TimeZone[] =
{
    TITLE , 'G','P','S',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    ALIGN_LEFT ,
    TEXT , 'U','T','C',' ','T','i','m','e','z','o','n','e', NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( GPS_TIME_ZONE ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

#endif // GPS_PRESENT

// PAGE_SETTINGS_RADIO 0x1C
static const uint8_t Page_SettingsRadio[] =
{
    TITLE , 'R','a','d','i','o',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_SETTINGS_RADIO_LIST , LIST_DATA_SOURCE_SCRIPT , 3 ) ,
    EVENT_END ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_RADIO_LIST 0x1D
static const uint8_t Page_SettingsRadio_List[] =
{
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'O','f','f','s','e','t', NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( RADIO_OFFSET ) ,
      LINK( PAGE_SETTINGS_RADIO_SET_OFFSET ) ,
      LINE_END ,
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'D','i','r','e','c','t','i','o','n', NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( RADIO_DIRECTION ) ,
      LINK( PAGE_SETTINGS_RADIO_SET_DIRECTION ) ,
      LINE_END ,
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'S','t','e','p' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( RADIO_STEP ) ,
      LINK( PAGE_SETTINGS_RADIO_SET_STEP ) ,
    PAGE_END
};

// PAGE_SETTINGS_RADIO_SET_OFFSET 0x1E
static const uint8_t Page_SettingsRadio_Set_Offset[] =
{
    TITLE , 'R','a','d','i','o',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    ALIGN_LEFT , TEXT , 'O','f','f','s','e','t', NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( RADIO_OFFSET ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_RADIO_SET_DIRECTION 0x1F
static const uint8_t Page_SettingsRadio_Set_Direction[] =
{
    TITLE , 'R','a','d','i','o',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    ALIGN_LEFT , TEXT , 'D','i','r','e','c','t','i','o','n', NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( RADIO_DIRECTION ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_RADIO_SET_STEP 0x20
static const uint8_t Page_SettingsRadio_Set_Step[] =
{
    TITLE , 'R','a','d','i','o',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    ALIGN_LEFT , TEXT , 'S','t','e','p' , NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( RADIO_STEP ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_M17 0x21
static const uint8_t Page_SettingsM17[] =
{
    TITLE , 'M','1','7',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_SETTINGS_M17_LIST , LIST_DATA_SOURCE_SCRIPT , 3 ) ,
    EVENT_END ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_M17_LIST 0x22
static const uint8_t Page_SettingsM17_List[] =
{
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'C','a','l','l','s','i','g','n' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( M17_CALLSIGN ) ,
      LINK( PAGE_SETTINGS_M17_SET_CALLSIGN ) ,
      LINE_END ,
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'C','A','N' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( M17_CAN ) ,
      LINK( PAGE_SETTINGS_M17_SET_CAN ) ,
      LINE_END ,
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'C','A','N',' ','R','X',' ','C','h','e','c','k' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( M17_CAN_RX_CHECK ) ,
      LINK( PAGE_SETTINGS_M17_SET_CAN_RX_CHECK ) ,
    PAGE_END
};

// PAGE_SETTINGS_M17_SET_CALLSIGN 0x23
static const uint8_t Page_SettingsM17_Set_Callsign[] =
{
    TITLE , 'M','1','7',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    ALIGN_LEFT , TEXT , 'C','a','l','l','s','i','g','n' , NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( M17_CALLSIGN ) , LINE_END , LINE_END ,
    TEXT , '*',' ',' ',' ','i','n','s','e','r','t',' ','c','h', NULL_CH , LINE_END ,
    TEXT , '#',' ',' ',' ','d','e','l','e','t','e',' ','c','h', NULL_CH , LINE_END ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_M17_SET_CAN 0x24
static const uint8_t Page_SettingsM17_Set_CAN[] =
{
    TITLE , 'M','1','7',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    ALIGN_LEFT , TEXT , 'C','A','N' , NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( M17_CAN ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_M17_SET_CAN_RX_CHECK 0x25
static const uint8_t Page_SettingsM17_Set_CAN_Rx_Check[] =
{
    TITLE , 'M','1','7',' ','S','e','t','t','i','n','g','s' , NULL_CH , LINE_END ,
    ALIGN_LEFT , TEXT , 'C','A','N',' ','R','X',' ','C','h','e','c','k' , NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( M17_CAN_RX_CHECK ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_VOICE 0x26
static const uint8_t Page_SettingsVoice[] =
{
    TITLE , 'V','o','i','c','e' , NULL_CH , LINE_END ,
    EVENT_START( EVENT_TYPE_KBD , EVENT_STATUS_ALL_KEYS ) ,
      LIST( PAGE_SETTINGS_M17_LIST , LIST_DATA_SOURCE_SCRIPT , 2 ) ,
    EVENT_END ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_VOICE_LIST 0x27
static const uint8_t Page_SettingsVoice_List[] =
{
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'V','o','i','c','e' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( LEVEL ) ,
      LINK( PAGE_SETTINGS_VOICE_SET_LEVEL ) ,
      LINE_END ,
    LIST_ELEMENT ,
      ALIGN_LEFT , TEXT , 'P','h','o','n','e','t','i','c' , NULL_CH ,
      ALIGN_RIGHT , VALUE_DSP( PHONETIC ) ,
      LINK( PAGE_SETTINGS_VOICE_SET_PHONETIC ) ,
    PAGE_END
};

// PAGE_SETTINGS_VOICE_SET_LEVEL 0x28
static const uint8_t Page_SettingsVoice_Set_Level[] =
{
    TITLE , 'V','o','i','c','e' , NULL_CH , LINE_END ,
    ALIGN_LEFT , TEXT , 'V','o','i','c','e' , NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( LEVEL ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_VOICE_SET_PHONETIC 0x29
static const uint8_t Page_SettingsVoice_Set_Phonetic[] =
{
    TITLE , 'V','o','i','c','e' , NULL_CH , LINE_END ,
    ALIGN_LEFT , TEXT , 'P','h','o','n','e','t','i','c' , NULL_CH ,
    ALIGN_RIGHT , VALUE_INP( PHONETIC ) ,
    ON_EVENT_KEY_ENTER_GO_BACK ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_SETTINGS_RESET_TO_DEFAULTS 0x2A
static const uint8_t Page_SettingsResetToDefaults[] =
{
    TITLE , 'R','e','s','e','t',' ','t','o',' ','D','e','f','a','u','l','t','s' , NULL_CH ,
    LINE_END ,
    TEXT , 'T','o',' ','R','e','s','e','t' , NULL_CH ,
    LINE_END ,
    TEXT , 'P','r','e','s','s',' ','E','n','t','e','r',' ','T','w','i','c','e' , NULL_CH ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_MENU_BACKUP_RESTORE 0x2B
static const uint8_t Page_MenuBackupRestore[] =
{
    TITLE , 'B','a','c','k','u','p',' ','A','n','d',' ','R','e','s','t','o','r','e' , NULL_CH ,
    VALUE_DSP( BACKUP_RESTORE ) ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_MENU_BACKUP 0x2C
static const uint8_t Page_MenuBackup[] =
{
    TITLE , 'F','l','a','s','h',' ','B','a','c','k','u','p' , NULL_CH ,
    LINE_END ,
    TEXT , 'C','o','n','n','e','c','t',' ','t','o',' ','R','T','X',' ','T','o','o','l' , NULL_CH ,
    LINE_END ,
    TEXT , 'T','o',' ','B','a','c','k','u','p',' ','F','l','a','s','h',' ','A','n','d' , NULL_CH ,
    LINE_END ,
    TEXT , 'P','r','e','s','s',' ','P','T','T',' ','t','o',' ','S','t','a','r','t' , NULL_CH ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_MENU_RESTORE 0x2D
static const uint8_t Page_MenuRestore[] =
{
    TITLE , 'F','l','a','s','h',' ','R','e','s','t','o','r','e' , NULL_CH ,
    LINE_END ,
    TEXT , 'C','o','n','n','e','c','t',' ','t','o',' ','R','T','X',' ','T','o','o','l' , NULL_CH ,
    LINE_END ,
    TEXT , 'T','o',' ','R','e','s','t','o','r','e',' ','F','l','a','s','h',' ','A','n','d' , NULL_CH ,
    LINE_END ,
    TEXT , 'P','r','e','s','s',' ','P','T','T',' ','t','o',' ','S','t','a','r','t' , NULL_CH ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_MENU_INFO 0x2E
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
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_LOW_BAT 0x2F
static const uint8_t Page_LowBat[] =
{
    TITLE , 'L','o','w',' ','B' ,'a' ,'t' ,'t' ,'e' ,'r','y' , NULL_CH ,
    LINE_END ,
    ALIGN_CENTER ,
    TEXT , 'F','o','r',' ','E','m','e','r','g','e','n','c','y',' ','U','s','e' , NULL_CH ,
    LINE_END ,
    TEXT , 'P','r','e','s','s',' ','A','n','y',' ','B','u','t','t','o','n' , NULL_CH ,
    VALUE_DSP( LOW_BATTERY ) ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_ABOUT 0x30
static const uint8_t Page_About[] =
{
    TITLE , 'A','b','o','u','t' , NULL_CH ,
    LINE_END ,
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
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

// PAGE_STUBBED 0x31
static const uint8_t Page_Stubbed[] =
{
    ALIGN_LEFT ,
    TEXT ,
     'P','a','g','e',' ',
     'S','t','u','b','b','e','d', NULL_CH ,
    ON_EVENT_KEY_ESC_GO_BACK ,
    PAGE_END
};

#define PAGE_REF( loc )    loc

const uint8_t* uiPageTable[ PAGE_NUM_OF ] =
{
    PAGE_REF( Page_MainVFO                      ) , // PAGE_MAIN_VFO                        0x00
    PAGE_REF( Page_MainInput                    ) , // PAGE_MAIN_VFO_INPUT                  0x01
    PAGE_REF( Page_MainMem                      ) , // PAGE_MAIN_MEM                        0x02
    PAGE_REF( Page_ModeVFO                      ) , // PAGE_MODE_VFO                        0x03
    PAGE_REF( Page_ModeMem                      ) , // PAGE_MODE_MEM                        0x04
    PAGE_REF( Page_MenuTop                      ) , // PAGE_MENU_TOP                        0x05
    PAGE_REF( Page_MenuTop_List                 ) , // PAGE_MENU_TOP_LIST                   0x06
    PAGE_REF( Page_MenuBank                     ) , // PAGE_MENU_BANK                       0x07
    PAGE_REF( Page_MenuBank_List                ) , // PAGE_MENU_BANK_LIST                  0x08
    PAGE_REF( Page_MenuChannel                  ) , // PAGE_MENU_CHANNEL                    0x09
    PAGE_REF( Page_MenuChannel_List             ) , // PAGE_MENU_CHANNEL_LIST               0x0A
    PAGE_REF( Page_MenuContact                  ) , // PAGE_MENU_CONTACTS                   0x0B
    PAGE_REF( Page_MenuContact_List             ) , // PAGE_MENU_CONTACTS_LIST              0x0C
    PAGE_REF( Page_MenuGPS                      ) , // PAGE_MENU_GPS                        0x0D
    PAGE_REF( Page_MenuSettings                 ) , // PAGE_MENU_SETTINGS                   0x0E
    PAGE_REF( Page_MenuSettings_List            ) , // PAGE_MENU_SETTINGS_LIST              0x0F
    PAGE_REF( Page_SettingsDisplay              ) , // PAGE_SETTINGS_DISPLAY                0x10
    PAGE_REF( Page_SettingsDisplay_List         ) , // PAGE_SETTINGS_DISPLAY_LIST           0x11
#ifdef SCREEN_BRIGHTNESS
    PAGE_REF( Page_Settings_Set_Brightness      ) , // PAGE_SETTINGS_SET_BRIGHTNESS         0x--
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
    PAGE_REF( Page_Settings_Set_Contrast        ) , // PAGE_SETTINGS_SET_CONTRAST           0x--
#endif // SCREEN_CONTRAST
    PAGE_REF( Page_Settings_Set_Timer           ) , // PAGE_SETTINGS_SET_TIMER              0x12
    PAGE_REF( Page_SettingsTimeDate             ) , // PAGE_SETTINGS_TIMEDATE               0x13
    PAGE_REF( Page_SettingsTimeDate_List        ) , // PAGE_SETTINGS_TIMEDATE_LIST          0x14
    PAGE_REF( Page_Settings_Set_Date            ) , // PAGE_SETTINGS_SET_DATE               0x15
    PAGE_REF( Page_Settings_Set_Time            ) , // PAGE_SETTINGS_SET_TIME               0x16
#ifdef GPS_PRESENT
    PAGE_REF( Page_SettingsGPS                  ) , // PAGE_SETTINGS_GPS                    0x17
    PAGE_REF( Page_SettingsGPS_List             ) , // PAGE_SETTINGS_GPS_LIST               0x18
    PAGE_REF( Page_SettingsGPS_Set_Enabled      ) , // PAGE_SETTINGS_GPS_SET_ENABLED        0x19
    PAGE_REF( Page_SettingsGPS_Set_Time         ) , // PAGE_SETTINGS_GPS_SET_TIME           0x1A
    PAGE_REF( Page_SettingsGPS_Set_TimeZone     ) , // PAGE_SETTINGS_GPS_SET_TIMEZONE       0x1B
#endif // GPS_PRESENT
    PAGE_REF( Page_SettingsRadio                ) , // PAGE_SETTINGS_RADIO                  0x1C
    PAGE_REF( Page_SettingsRadio_List           ) , // PAGE_SETTINGS_RADIO_LIST             0x1D
    PAGE_REF( Page_SettingsRadio_Set_Offset     ) , // PAGE_SETTINGS_RADIO_SET_OFFSET       0x1E
    PAGE_REF( Page_SettingsRadio_Set_Direction  ) , // PAGE_SETTINGS_RADIO_SET_DIRECTION    0x1F
    PAGE_REF( Page_SettingsRadio_Set_Step       ) , // PAGE_SETTINGS_RADIO_SET_STEP         0x20
    PAGE_REF( Page_SettingsM17                  ) , // PAGE_SETTINGS_M17                    0x21
    PAGE_REF( Page_SettingsM17_List             ) , // PAGE_SETTINGS_M17_LIST               0x22
    PAGE_REF( Page_SettingsM17_Set_Callsign     ) , // PAGE_SETTINGS_M17_SET_CALLSIGN       0x23
    PAGE_REF( Page_SettingsM17_Set_CAN          ) , // PAGE_SETTINGS_M17_SET_CAN            0x24
    PAGE_REF( Page_SettingsM17_Set_CAN_Rx_Check ) , // PAGE_SETTINGS_M17_SET_CAN_RX_CHECK   0x25
    PAGE_REF( Page_SettingsVoice                ) , // PAGE_SETTINGS_VOICE                  0x26
    PAGE_REF( Page_SettingsVoice_List           ) , // PAGE_SETTINGS_VOICE_LIST             0x27
    PAGE_REF( Page_SettingsVoice_Set_Level      ) , // PAGE_SETTINGS_VOICE_SET_LEVEL        0x28
    PAGE_REF( Page_SettingsVoice_Set_Phonetic   ) , // PAGE_SETTINGS_VOICE_SET_PHONETIC     0x29
    PAGE_REF( Page_SettingsResetToDefaults      ) , // PAGE_SETTINGS_RESET_TO_DEFAULTS      0x2A
    PAGE_REF( Page_MenuBackupRestore            ) , // PAGE_MENU_BACKUP_RESTORE             0x2B
    PAGE_REF( Page_MenuBackup                   ) , // PAGE_MENU_BACKUP                     0x2C
    PAGE_REF( Page_MenuRestore                  ) , // PAGE_MENU_RESTORE                    0x2D
    PAGE_REF( Page_MenuInfo                     ) , // PAGE_MENU_INFO                       0x2E
    PAGE_REF( Page_LowBat                       ) , // PAGE_LOW_BAT                         0x2F
    PAGE_REF( Page_About                        ) , // PAGE_ABOUT                           0x30
    PAGE_REF( Page_Stubbed                      )   // PAGE_STUBBED                         0x31
};
