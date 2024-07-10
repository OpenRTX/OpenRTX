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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <utils.h>
#include <ui.h>
#include <ui/ui_default.h>
#include "ui_states.h"
#include "ui_commands.h"
#include "ui_list_display.h"
#include <interfaces/nvmem.h>
#include <interfaces/cps_io.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <memory_profiling.h>
#include <ui/ui_strings.h>
#include <core/voicePromptUtils.h>
#include "ui_value_display.h"
#include "ui_value_arrays.h"

extern char* uiGetPageTextString( uiPageNum_en pageNum , uint8_t textStringIndex );

static void ui_Draw_MenuItem( GuiState_st* guiState , char* entryBuf );

static int ui_Get_MenuTopEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_BankName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_ChannelName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_ContactName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_SettingsEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_BackupRestoreEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );

static int ui_Get_InfoEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_DisplayEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_SettingsGPSEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_M17EntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_VoiceEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_RadioEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );

static int ui_Get_InfoValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_DisplayValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_SettingsGPSValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_M17ValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_VoiceValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
static int ui_Get_RadioValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );

static int ui_Get_StubbedName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );

static bool DidSelectedMenuItemChange( char* menuName , char* menuValue );
static bool ScreenContainsReadOnlyEntries( int menuScreen );
static void announceMenuItemIfNeeded( GuiState_st* guiState , char* name , char* value , bool editMode );

static char     priorSelectedMenuName[ MAX_ENTRY_LEN ]  = "\0" ;
static char     priorSelectedMenuValue[ MAX_ENTRY_LEN ] = "\0" ;
static bool     priorEditMode                           = false ;
static uint32_t lastValueUpdate                         = 0 ;

static const GetMenuList_fn GetEntryName_table[ PAGE_NUM_OF ] =
{
    ui_Get_StubbedName            , // PAGE_MAIN_VFO
    ui_Get_StubbedName            , // PAGE_MAIN_VFO_INPUT
    ui_Get_StubbedName            , // PAGE_MAIN_MEM
    ui_Get_StubbedName            , // PAGE_MODE_VFO
    ui_Get_StubbedName            , // PAGE_MODE_MEM
    ui_Get_MenuTopEntryName       , // PAGE_MENU_TOP
    ui_Get_BankName               , // PAGE_MENU_BANK
    ui_Get_ChannelName            , // PAGE_MENU_CHANNEL
    ui_Get_ContactName            , // PAGE_MENU_CONTACTS
    ui_Get_StubbedName            , // PAGE_MENU_GPS
    ui_Get_SettingsEntryName      , // PAGE_MENU_SETTINGS
    ui_Get_BackupRestoreEntryName , // PAGE_MENU_BACKUP_RESTORE
    ui_Get_StubbedName            , // PAGE_MENU_BACKUP
    ui_Get_StubbedName            , // PAGE_MENU_RESTORE
    ui_Get_InfoEntryName          , // PAGE_MENU_INFO
    ui_Get_StubbedName            , // PAGE_SETTINGS_TIMEDATE
    ui_Get_StubbedName            , // PAGE_SETTINGS_TIMEDATE_SET
    ui_Get_DisplayEntryName       , // PAGE_SETTINGS_DISPLAY
    ui_Get_SettingsGPSEntryName   , // PAGE_SETTINGS_GPS
    ui_Get_RadioEntryName         , // PAGE_SETTINGS_RADIO
    ui_Get_M17EntryName           , // PAGE_SETTINGS_M17
    ui_Get_VoiceEntryName         , // PAGE_SETTINGS_VOICE
    ui_Get_StubbedName            , // PAGE_SETTINGS_RESET_TO_DEFAULTS
    ui_Get_StubbedName            , // PAGE_LOW_BAT
    ui_Get_StubbedName            , // PAGE_ABOUT
    ui_Get_StubbedName              // PAGE_STUBBED
};

void _ui_Draw_MenuList( GuiState_st* guiState , uiPageNum_en currentEntry )
{
    Line_st*  line1  = &guiState->layout.lines[ GUI_LINE_1 ] ;
//    Style_st* style1 = &guiState->layout.styles[ GUI_STYLE_1 ] ;
    Line_st*  line   = &guiState->layout.line ;
//    Style_st* style  = &guiState->layout.style ;
    char      entryBuf[ MAX_ENTRY_LEN ] = "" ;
    int       result ;

    *line            = *line1 ;
//    *style           = *style1 ;

    // Number of menu entries that fit in the screen height
    guiState->layout.numOfEntries = ( SCREEN_HEIGHT - 1 - line->pos.y ) /
                                    guiState->layout.menu_h + 1 ;
    guiState->layout.scrollOffset = 0 ;
/*
    for( guiState->layout.linkIndex = 0 , result = 0 ;
         ( result == 0 ) && ( line->pos.y < SCREEN_HEIGHT );
         guiState->layout.linkIndex++ )
    {
        // If selection is off the screen, scroll screen
        if( guiState->uiState.entrySelected >= guiState->layout.numOfEntries )
        {
            guiState->layout.scrollOffset = guiState->uiState.entrySelected -
                                            guiState->layout.numOfEntries + 1 ;
        }

        GetMenuList_fn getCurrentEntry = GetEntryName_table[ currentEntry ];

        // Call function pointer to get current menu entry string
        result = (*getCurrentEntry)( guiState ,
                                     entryBuf , sizeof( entryBuf ) ,
                                     guiState->layout.linkIndex +
                                     guiState->layout.scrollOffset );

        if( result != -1 )
        {
            ui_Draw_MenuItem( guiState , entryBuf );
        }

    }
*/
}

static void ui_Draw_MenuItem( GuiState_st* guiState , char* entryBuf )
{
/*
    Line_st*  line   = &guiState->layout.line ;
//    Style_st* style  = &guiState->layout.style ;
    Color_st  color_fg ;
    Color_st  color_bg ;
    Color_st  Color_text ;

    ui_ColorLoad( &color_fg , COLOR_FG );
    ui_ColorLoad( &color_bg , COLOR_BG );
    Color_text = color_fg ;

    if( ( guiState->layout.linkIndex + guiState->layout.scrollOffset ) ==
        guiState->uiState.entrySelected )
    {
        Color_text = color_bg ;
        // Draw rectangle under selected item, compensating for text height
        Pos_st rect_pos = { 0 , line->pos.y - guiState->layout.menu_h + 3 , SCREEN_WIDTH , guiState->layout.menu_h };
        gfx_drawRect( &rect_pos , &color_fg , true );
        announceMenuItemIfNeeded( guiState , entryBuf , NULL , false );
    }
    gfx_print( &line->pos , guiState->layout.menu_font.size ,
               GFX_ALIGN_LEFT , &Color_text , entryBuf );
    line->pos.y += guiState->layout.menu_h ;
*/
}

static const GetMenuList_fn GetEntryValue_table[ PAGE_NUM_OF ] =
{
    ui_Get_StubbedName          , // PAGE_MAIN_VFO
    ui_Get_StubbedName          , // PAGE_MAIN_VFO_INPUT
    ui_Get_StubbedName          , // PAGE_MAIN_MEM
    ui_Get_StubbedName          , // PAGE_MODE_VFO
    ui_Get_StubbedName          , // PAGE_MODE_MEM
    ui_Get_StubbedName          , // PAGE_MENU_TOP
    ui_Get_StubbedName          , // PAGE_MENU_BANK
    ui_Get_StubbedName          , // PAGE_MENU_CHANNEL
    ui_Get_StubbedName          , // PAGE_MENU_CONTACTS
    ui_Get_StubbedName          , // PAGE_MENU_GPS
    ui_Get_StubbedName          , // PAGE_MENU_SETTINGS
    ui_Get_StubbedName          , // PAGE_MENU_BACKUP_RESTORE
    ui_Get_StubbedName          , // PAGE_MENU_BACKUP
    ui_Get_StubbedName          , // PAGE_MENU_RESTORE
    ui_Get_InfoValueName        , // PAGE_MENU_INFO
    ui_Get_StubbedName          , // PAGE_SETTINGS_TIMEDATE
    ui_Get_StubbedName          , // PAGE_SETTINGS_TIMEDATE_SET
    ui_Get_DisplayValueName     , // PAGE_SETTINGS_DISPLAY
    ui_Get_SettingsGPSValueName , // PAGE_SETTINGS_GPS
    ui_Get_RadioValueName       , // PAGE_SETTINGS_RADIO
    ui_Get_M17ValueName         , // PAGE_SETTINGS_M17
    ui_Get_VoiceValueName       , // PAGE_SETTINGS_VOICE
    ui_Get_StubbedName          , // PAGE_SETTINGS_RESET_TO_DEFAULTS
    ui_Get_StubbedName          , // PAGE_LOW_BAT
    ui_Get_StubbedName          , // PAGE_ABOUT
    ui_Get_StubbedName            // PAGE_STUBBED
};

void _ui_Draw_MenuListValue( GuiState_st* guiState ,
                             uiPageNum_en currentEntry , uiPageNum_en currentEntryValue )
{
    Line_st*  line1  = &guiState->layout.lines[ GUI_LINE_1 ] ;
//    Style_st* style1 = &guiState->layout.styles[ GUI_STYLE_1 ] ;
    Line_st*  line   = &guiState->layout.line ;
//    Style_st* style  = &guiState->layout.style ;
    // Number of menu entries that fit in the screen height
    uint8_t   numOfEntries ;
    uint8_t   scroll = 0 ;
    char      entry_buf[ MAX_ENTRY_LEN ] = "" ;
    char      value_buf[ MAX_ENTRY_LEN ] = "" ;
    Color_st  color_fg ;
    Color_st  color_bg ;
    Color_st  text_color ;

    ui_ColorLoad( &color_fg , COLOR_FG );
    ui_ColorLoad( &color_bg , COLOR_BG );
    text_color = color_fg ;

    GetMenuList_fn getCurrentEntry = GetEntryName_table[ currentEntry ];
    GetMenuList_fn getCurrentValue = GetEntryValue_table[ currentEntryValue ];

    guiState->layout.line = *line1 ;

    // Number of menu entries that fit in the screen height
    numOfEntries 		  = ( SCREEN_HEIGHT - 1 - line1->pos.y ) / guiState->layout.menu_h + 1 ;

    for( int item = 0 , result = 0 ;
         ( result == 0 ) && ( line->pos.y < SCREEN_HEIGHT ) ;
         item++ )
    {
        // If selection is off the screen, scroll screen
        if( guiState->uiState.entrySelected >= numOfEntries )
        {
            scroll = ( guiState->uiState.entrySelected - numOfEntries ) + 1 ; //@@@KL check this
        }
        // Call function pointer to get current menu entry string
        result = (*getCurrentEntry)( guiState , entry_buf , sizeof( entry_buf ) , item + scroll );
        // Call function pointer to get current entry value string
        result = (*getCurrentValue)( guiState , value_buf , sizeof( value_buf ) , item + scroll );
        if( result != -1 )
        {
            text_color = color_fg;
            if( ( item + scroll ) == guiState->uiState.entrySelected )
            {
                // Draw rectangle under selected item, compensating for text height
                // If we are in edit mode, draw a hollow rectangle
                text_color = color_bg ;
                bool full_rect  = true ;

                if( guiState->uiState.edit_mode )
                {
                    text_color = color_fg ;
                    full_rect  = false ;
                }
                Pos_st rect_pos = { 0 , line->pos.y - guiState->layout.menu_h + 3 , SCREEN_WIDTH , guiState->layout.menu_h };
                gfx_drawRect( &rect_pos , &color_fg , full_rect );
                bool editModeChanged = priorEditMode != guiState->uiState.edit_mode ;
                priorEditMode = guiState->uiState.edit_mode ;
                // force the menu item to be spoken  when the edit mode changes.
                // E.g. when pressing Enter on Display Brightness etc.
                if( editModeChanged )
                {
                    priorSelectedMenuName[ 0 ] = '\0' ;
                }
                if( !guiState->uiState.edit_mode || editModeChanged )
                {// If in edit mode, only want to speak the char being entered,,
            //not repeat the entire display.
                    announceMenuItemIfNeeded( guiState , entry_buf , value_buf , guiState->uiState.edit_mode );
                }
            }
            gfx_print( &line->pos , guiState->layout.menu_font.size , GFX_ALIGN_LEFT , &text_color , entry_buf );
            gfx_print( &line->pos , guiState->layout.menu_font.size , GFX_ALIGN_RIGHT , &text_color , value_buf );
            line1->pos.y += guiState->layout.menu_h ;
        }
    }
}

static int ui_Get_MenuTopEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >=  ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_MENU_TOP , index ) );

    return 0;
}

static int ui_Get_SettingsEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_MENU_SETTINGS , index ) );

    return 0;
}

static int ui_Get_DisplayEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_SETTINGS_DISPLAY , index ) );

    return 0;
}

static int ui_Get_DisplayValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1;
    }
    uint8_t value = 0 ;
    switch( index )
    {
#ifdef SCREEN_BRIGHTNESS
        case D_BRIGHTNESS :
        {
            value = last_state.settings.brightness ;
            break ;
        }
#endif
#ifdef SCREEN_CONTRAST
        case D_CONTRAST :
        {
            value = last_state.settings.contrast ;
            break ;
        }
#endif
        case D_TIMER :
        {
            snprintf( buf , max_len , "%s" , display_timer_values[ last_state.settings.display_timer ] );
            return 0 ;
        }
    }
    snprintf( buf , max_len , "%d" , value );
    return 0 ;
}

#ifdef GPS_PRESENT
static int ui_Get_SettingsGPSEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1 ;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_SETTINGS_GPS , index ) );

    return 0 ;
}

static int ui_Get_SettingsGPSValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1 ;
    }
    switch( index )
    {
        case G_ENABLED :
        {
            snprintf( buf , max_len , "%s" ,
                      (last_state.settings.gps_enabled) ? currentLanguage->on : currentLanguage->off );
            break ;
        }
        case G_SET_TIME :
        {
            snprintf( buf , max_len , "%s" ,
                      (last_state.gps_set_time) ? currentLanguage->on : currentLanguage->off );
            break ;
        }
        case G_TIMEZONE :
        {
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

            snprintf( buf , max_len , "%c%d.%d" , sign , tz_hr , tz_mn );
            break ;
        }
    }
    return 0;
}
#endif

static int ui_Get_RadioEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1 ;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_SETTINGS_RADIO , index ) );

    return 0 ;
}

static int ui_Get_RadioValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1;
    }
    int32_t offset = 0;
    switch( index )
    {
        case R_OFFSET :
        {
            offset = abs( (int32_t)last_state.channel.tx_frequency -
                          (int32_t)last_state.channel.rx_frequency );
            snprintf( buf , max_len , "%gMHz" , (float)offset / 1000000.0f );
            break ;
        }
        case R_DIRECTION :
        {
            buf[ 0 ] = ( last_state.channel.tx_frequency >= last_state.channel.rx_frequency ) ? '+' : '-';
            buf[ 1 ] = '\0';
            break ;
        }
        case R_STEP :
        {
            // Print in kHz if it is smaller than 1MHz
            if( freq_steps[ last_state.step_index ] < 1000000 )
            {
                snprintf( buf , max_len , "%gkHz" , (float)freq_steps[last_state.step_index] / 1000.0f );
            }
            else
            {
                snprintf( buf , max_len , "%gMHz" , (float)freq_steps[last_state.step_index] / 1000000.0f );
            }
            break ;
        }
    }

    return 0;
}

static int ui_Get_M17EntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
       return -1 ;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_SETTINGS_M17 , index ) );

    return 0;
}

static int ui_Get_M17ValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1;
    }
    switch( index )
    {
        case M17_CALLSIGN :
        {
            snprintf( buf , max_len , "%s" , last_state.settings.callsign );
            break ;
        }
        case M17_CAN :
        {
            snprintf( buf , max_len , "%d" , last_state.settings.m17_can );
            break ;
        }
        case M17_CAN_RX :
        {
            snprintf( buf , max_len , "%s" ,
                      (last_state.settings.m17_can_rx) ? currentLanguage->on : currentLanguage->off );
            break ;
        }
    }

    return 0;
}

static int ui_Get_VoiceEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1 ;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_SETTINGS_VOICE , index ) );

    return 0 ;
}

static int ui_Get_VoiceValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1 ;
    }
    uint8_t value = 0 ;
    switch( index )
    {
        case VP_LEVEL:
        {
            value = last_state.settings.vpLevel;
            switch( value )
            {
                case VPP_NONE :
                {
                    snprintf( buf , max_len , "%s" , currentLanguage->off );
                    break ;
                }
                case VPP_BEEP :
                {
                    snprintf( buf , max_len , "%s" , currentLanguage->beep );
                    break ;
                }
                default :
                {
                    snprintf( buf , max_len , "%d" , ( value - VPP_BEEP ) ); //@@@KL ? check this
                    break ;
                }
            }
            break ;
        }
        case VP_PHONETIC:
        {
            snprintf( buf , max_len , "%s" , last_state.settings.vpPhoneticSpell ? currentLanguage->on : currentLanguage->off );
            break ;
        }
    }
    return 0;
}

static int ui_Get_BackupRestoreEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_MENU_BACKUP_RESTORE , index ) );

    return 0;
}

static int ui_Get_InfoEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_MENU_INFO , index ) );

    return 0;
}

static int ui_Get_InfoValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    const hwInfo_t* hwinfo = platform_getHwInfo();
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
       return -1;
    }
    switch( index )
    {
        case 0 : // Git Version
        {
            snprintf( buf , max_len , "%s" , GIT_VERSION );
            break ;
        }
        case 1 : // Battery voltage
        {
            // Compute integer part and mantissa of voltage value, adding 50mV
            // to mantissa for rounding to nearest integer
            uint16_t volt  = ( last_state.v_bat + 50 ) / 1000 ;
            uint16_t mvolt = ( ( last_state.v_bat - volt * 1000 ) + 50 ) / 100 ;
            snprintf( buf , max_len , "%d.%dV" , volt, mvolt );
            break ;
        }
        case 2 : // Battery charge
        {
            snprintf( buf , max_len , "%d%%" , last_state.charge );
            break ;
        }
        case 3 : // RSSI
        {
            snprintf( buf , max_len , "%.1fdBm" , last_state.rssi );
            break ;
        }
        case 4 : // Heap usage
        {
            snprintf( buf , max_len , "%dB" , getHeapSize() - getCurrentFreeHeap() );
            break ;
        }
        case 5 : // Band
        {
            snprintf( buf , max_len , "%s %s" , hwinfo->vhf_band ?
                      currentLanguage->VHF : "" , hwinfo->uhf_band ? currentLanguage->UHF : "" );
            break ;
        }
        case 6 : // VHF
        {
            snprintf( buf , max_len , "%d - %d" , hwinfo->vhf_minFreq, hwinfo->vhf_maxFreq );
            break ;
        }
        case 7 : // UHF
        {
            snprintf( buf , max_len , "%d - %d" , hwinfo->uhf_minFreq, hwinfo->uhf_maxFreq );
            break ;
        }
        case 8 : // LCD Type
        {
            snprintf( buf , max_len , "%d" , hwinfo->hw_version );
            break ;
        }
#ifdef PLATFORM_TTWRPLUS
        case 9 : // Radio model
        {
            strncpy( buf , sa8x8_getModel() , max_len );
            break ;
        }
        case 10 : // Radio firmware version
        {
            // Get FW version string, skip the first nine chars ("sa8x8-fw/" )
            uint8_t major , minor , patch , release ;
            const char *fwVer = sa8x8_getFwVersion();

            sscanf( fwVer , "sa8x8-fw/v%hhu.%hhu.%hhu.r%hhu" , &major , &minor , &patch , &release );
            snprintf( buf , max_len ,"v%hhu.%hhu.%hhu.r%hhu" , major , minor , patch , release );
            break ;
        }
#endif //
    }
    return 0;
}

static int ui_Get_BankName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    (void)guiState ;
    int result = 0 ;
    // First bank "All channels" is not read from flash
    if( index == 0 )
    {
        strncpy( buf , currentLanguage->allChannels , max_len );
    }
    else
    {
        bankHdr_t bank ;
        result = cps_readBankHeader( &bank , index - 1 );
        if(result != -1)
        {
            snprintf( buf , max_len , "%s" , bank.name );
        }
    }
    return result;
}

static int ui_Get_ChannelName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    (void)guiState ;
    channel_t channel ;
    int       result = cps_readChannel( &channel , index );

    if( result != -1 )
    {
        snprintf( buf , max_len , "%s" , channel.name );
    }

    return result ;
}

static int ui_Get_ContactName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    (void)guiState ;
    contact_t contact ;
    int       result = cps_readContact( &contact , index );

    if( result != -1 )
    {
        snprintf( buf , max_len , "%s" , contact.name );
    }

    return result ;
}

static int ui_Get_StubbedName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    (void)guiState ;
    (void)buf ;
    (void)max_len ;
    (void)index ;

    return 0 ;

}

static bool DidSelectedMenuItemChange( char* menuName , char* menuValue )
{
    // menu name can't be empty.
    if( ( menuName == NULL ) || ( *menuName == '\0' ) )
    {
        return false ;
    }
    // If value is supplied it can't be empty but it does not have to be supplied.
    if( ( menuValue != NULL ) && ( *menuValue == '\0' ) )
    {
        return false ;
    }
    if( strcmp( menuName , priorSelectedMenuName ) != 0 )
    {
        strcpy( priorSelectedMenuName , menuName );
        if( menuValue != NULL )
        {
            strcpy( priorSelectedMenuValue , menuValue );
        }
        else
        {
            *priorSelectedMenuValue = '\0' ; // reset it since we've changed menu item.
        }
        return true ;
    }

    if( ( menuValue != NULL ) && ( strcmp( menuValue , priorSelectedMenuValue ) != 0 ) )
    {
        // avoid chatter when value changes rapidly!
        uint32_t now = getTick();

        uint32_t interval = now - lastValueUpdate ;
        lastValueUpdate = now;
        if( interval < 1000 )
        {
            return false ;
        }
        strcpy( priorSelectedMenuValue , menuValue );
        return true ;
    }

    return false ;
}
/*
Normally we determine if we should say the word menu if a menu entry has no
associated value that can be changed.
There are some menus however with no associated value which are not submenus,
e.g. the entries under Channels, contacts, Info,
which are navigable but not modifyable.
*/
static bool ScreenContainsReadOnlyEntries( int menuScreen )
{
    switch( menuScreen )
    {
        case PAGE_MENU_CHANNEL :
        case PAGE_MENU_CONTACTS :
        case PAGE_MENU_INFO :
        {
            return true ;
        }
    }
    return false ;
}

static void announceMenuItemIfNeeded( GuiState_st* guiState , char* name , char* value , bool editMode )
{
    if( state.settings.vpLevel < VPP_LOW )
    {
        return;
    }
    if( ( name == NULL ) || ( *name == '\0' ) )
    {
        return;
    }
    if( DidSelectedMenuItemChange( name , value ) == false )
    {
        return;
    }
    // Stop any prompt in progress and/or clear the buffer.
    vp_flush();

    vp_announceText( name , VPQ_DEFAULT );
// We determine if we should say the word Menu as follows:
// The name has no  associated value ,
// i.e. does not represent a modifyable name/value pair.
// We're not in edit mode.
// The screen is navigable but entries  are readonly.
    if( !value && !editMode && !ScreenContainsReadOnlyEntries( guiState->page.num ) )
    {
        vp_queueStringTableEntry( &currentLanguage->menu );
    }
    if( editMode )
    {
        vp_queuePrompt( PROMPT_EDIT );
    }
    if( ( value != NULL ) && ( *value != '\0' ) )
    {
        vp_announceText( value , VPQ_DEFAULT );
    }
    vp_play();
}

void _ui_reset_menu_anouncement_tracking( void )
{
    *priorSelectedMenuName  = '\0' ;
    *priorSelectedMenuValue = '\0' ;
}
