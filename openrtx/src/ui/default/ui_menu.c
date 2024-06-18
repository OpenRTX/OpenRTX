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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <utils.h>
#include <ui/ui_default.h>
#include "ui_states.h"
#include "ui_commands.h"
#include <interfaces/nvmem.h>
#include <interfaces/cps_io.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <memory_profiling.h>
#include <ui/ui_strings.h>
#include <core/voicePromptUtils.h>
#include "ui_value_display.h"
#include "ui_value_arrays.h"

#ifdef PLATFORM_TTWRPLUS
#include <SA8x8.h>
#endif

extern char* uiGetPageTextString( uiPageNum_en pageNum , uint8_t textStringIndex );

/* UI main screen helper functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainBottom( GuiState_st* guiState , Event_st* event );

void ui_drawMenuItem( GuiState_st* guiState , char* entryBuf );

int _ui_getMenuTopEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getBankName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getChannelName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getContactName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getSettingsEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getBackupRestoreEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );

int _ui_getInfoEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getDisplayEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getSettingsGPSEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getM17EntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getVoiceEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getRadioEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );

int _ui_getInfoValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getDisplayValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getSettingsGPSValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getM17ValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getVoiceValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );
int _ui_getRadioValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );

int _ui_getStubbedName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index );

static char     priorSelectedMenuName[ MAX_ENTRY_LEN ]  = "\0" ;
static char     priorSelectedMenuValue[ MAX_ENTRY_LEN ] = "\0" ;
static bool     priorEditMode                           = false ;
static uint32_t lastValueUpdate                         = 0 ;

void _ui_reset_menu_anouncement_tracking( void )
{
    *priorSelectedMenuName  = '\0' ;
    *priorSelectedMenuValue = '\0' ;
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

static const GetMenuList_fn GetEntryName_table[ PAGE_NUM_OF ] =
{
    _ui_getStubbedName            , // PAGE_MAIN_VFO
    _ui_getStubbedName            , // PAGE_MAIN_VFO_INPUT
    _ui_getStubbedName            , // PAGE_MAIN_MEM
    _ui_getStubbedName            , // PAGE_MODE_VFO
    _ui_getStubbedName            , // PAGE_MODE_MEM
    _ui_getMenuTopEntryName       , // PAGE_MENU_TOP
    _ui_getBankName               , // PAGE_MENU_BANK
    _ui_getChannelName            , // PAGE_MENU_CHANNEL
    _ui_getContactName            , // PAGE_MENU_CONTACTS
    _ui_getStubbedName            , // PAGE_MENU_GPS
    _ui_getSettingsEntryName      , // PAGE_MENU_SETTINGS
    _ui_getBackupRestoreEntryName , // PAGE_MENU_BACKUP_RESTORE
    _ui_getStubbedName            , // PAGE_MENU_BACKUP
    _ui_getStubbedName            , // PAGE_MENU_RESTORE
    _ui_getInfoEntryName          , // PAGE_MENU_INFO
    _ui_getStubbedName            , // PAGE_MENU_ABOUT
    _ui_getStubbedName            , // PAGE_SETTINGS_TIMEDATE
    _ui_getStubbedName            , // PAGE_SETTINGS_TIMEDATE_SET
    _ui_getDisplayEntryName       , // PAGE_SETTINGS_DISPLAY
    _ui_getSettingsGPSEntryName   , // PAGE_SETTINGS_GPS
    _ui_getRadioEntryName         , // PAGE_SETTINGS_RADIO
    _ui_getM17EntryName           , // PAGE_SETTINGS_M17
    _ui_getVoiceEntryName         , // PAGE_SETTINGS_VOICE
    _ui_getStubbedName            , // PAGE_SETTINGS_RESET_TO_DEFAULTS
    _ui_getStubbedName            , // PAGE_LOW_BAT
    _ui_getStubbedName            , // PAGE_ABOUT
    _ui_getStubbedName              // PAGE_STUBBED
};

void _ui_drawMenuList( GuiState_st* guiState , uiPageNum_en currentEntry )
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
            ui_drawMenuItem( guiState , entryBuf );
        }

    }
}

void ui_drawMenuItem( GuiState_st* guiState , char* entryBuf )
{
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
        Pos_st rect_pos = { 0 , line->pos.y - guiState->layout.menu_h + 3 };
        gfx_drawRect( rect_pos , SCREEN_WIDTH , guiState->layout.menu_h , color_fg , true );
        announceMenuItemIfNeeded( guiState , entryBuf , NULL , false );
    }
    gfx_print( line->pos , guiState->layout.menu_font.size ,
               ALIGN_LEFT , Color_text , entryBuf );
    line->pos.y += guiState->layout.menu_h ;

}

static const GetMenuList_fn GetEntryValue_table[ PAGE_NUM_OF ] =
{
    _ui_getStubbedName          , // PAGE_MAIN_VFO
    _ui_getStubbedName          , // PAGE_MAIN_VFO_INPUT
    _ui_getStubbedName          , // PAGE_MAIN_MEM
    _ui_getStubbedName          , // PAGE_MODE_VFO
    _ui_getStubbedName          , // PAGE_MODE_MEM
    _ui_getStubbedName          , // PAGE_MENU_TOP
    _ui_getStubbedName          , // PAGE_MENU_BANK
    _ui_getStubbedName          , // PAGE_MENU_CHANNEL
    _ui_getStubbedName          , // PAGE_MENU_CONTACTS
    _ui_getStubbedName          , // PAGE_MENU_GPS
    _ui_getStubbedName          , // PAGE_MENU_SETTINGS
    _ui_getStubbedName          , // PAGE_MENU_BACKUP_RESTORE
    _ui_getStubbedName          , // PAGE_MENU_BACKUP
    _ui_getStubbedName          , // PAGE_MENU_RESTORE
    _ui_getInfoValueName        , // PAGE_MENU_INFO
    _ui_getStubbedName          , // PAGE_MENU_ABOUT
    _ui_getStubbedName          , // PAGE_SETTINGS_TIMEDATE
    _ui_getStubbedName          , // PAGE_SETTINGS_TIMEDATE_SET
    _ui_getDisplayValueName     , // PAGE_SETTINGS_DISPLAY
    _ui_getSettingsGPSValueName , // PAGE_SETTINGS_GPS
    _ui_getRadioValueName       , // PAGE_SETTINGS_RADIO
    _ui_getM17ValueName         , // PAGE_SETTINGS_M17
    _ui_getVoiceValueName       , // PAGE_SETTINGS_VOICE
    _ui_getStubbedName          , // PAGE_SETTINGS_RESET_TO_DEFAULTS
    _ui_getStubbedName          , // PAGE_LOW_BAT
    _ui_getStubbedName          , // PAGE_ABOUT
    _ui_getStubbedName            // PAGE_STUBBED
};

void _ui_drawMenuListValue( GuiState_st* guiState ,
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
                Pos_st rect_pos = { 0 , line->pos.y - guiState->layout.menu_h + 3 };
                gfx_drawRect( rect_pos , SCREEN_WIDTH , guiState->layout.menu_h , color_fg , full_rect );
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
            gfx_print( line->pos , guiState->layout.menu_font.size , ALIGN_LEFT , text_color , entry_buf );
            gfx_print( line->pos , guiState->layout.menu_font.size , ALIGN_RIGHT , text_color , value_buf );
            line1->pos.y += guiState->layout.menu_h ;
        }
    }
}

int _ui_getMenuTopEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) ) // @@@KL rationalise this
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_MENU_TOP , index ) );

    return 0;
}

int _ui_getSettingsEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) ) // @@@KL rationalise this
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_MENU_SETTINGS , index ) );

    return 0;
}

int _ui_getDisplayEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_SETTINGS_DISPLAY , index ) );

    return 0;
}

int _ui_getDisplayValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
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
int _ui_getSettingsGPSEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1 ;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_SETTINGS_GPS , index ) );

    return 0 ;
}

int _ui_getSettingsGPSValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
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

int _ui_getRadioEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1 ;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_SETTINGS_RADIO , index ) );

    return 0 ;
}

int _ui_getRadioValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
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

int _ui_getM17EntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
       return -1 ;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_SETTINGS_M17 , index ) );

    return 0;
}

int _ui_getM17ValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
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

int _ui_getVoiceEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1 ;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_SETTINGS_VOICE , index ) );

    return 0 ;
}

int _ui_getVoiceValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
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

int _ui_getBackupRestoreEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_MENU_BACKUP_RESTORE , index ) );

    return 0;
}

int _ui_getInfoEntryName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= ui_States_GetPageNumOfEntries( guiState ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , uiGetPageTextString( PAGE_MENU_INFO , index ) );

    return 0;
}

int _ui_getInfoValueName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
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

int _ui_getBankName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
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

int _ui_getChannelName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
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

int _ui_getContactName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
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

int _ui_getStubbedName( GuiState_st* guiState , char* buf , uint8_t max_len , uint8_t index )
{
    (void)guiState ;
    (void)buf ;
    (void)max_len ;
    (void)index ;

    return 0 ;

}

void _ui_drawMenuTop( GuiState_st* guiState )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Menu" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->menu );
    // Print menu entries
    _ui_drawMenuList( guiState , PAGE_MENU_TOP );
}

void _ui_drawMenuBank( GuiState_st* guiState )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Bank" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->banks );
    // Print bank entries
    _ui_drawMenuList( guiState , PAGE_MENU_BANK );
}

void _ui_drawMenuChannel( GuiState_st* guiState )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Channel" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->channels );
    // Print channel entries
    _ui_drawMenuList( guiState , PAGE_MENU_CHANNEL );
}

void _ui_drawMenuContacts( GuiState_st* guiState )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Contacts" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->contacts );
    // Print contact entries
    _ui_drawMenuList( guiState , PAGE_MENU_CONTACTS );
}

#ifdef GPS_PRESENT
void _ui_drawMenuGPS( GuiState_st* guiState )
{
    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line1       = &guiState->layout.lines[ GUI_LINE_1 ] ;
//    Style_st* style1      = &guiState->layout.styles[ GUI_STYLE_1 ] ;
    Line_st*  line2       = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2      = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    Line_st*  lineBottom  = &guiState->layout.lines[ GUI_LINE_BOTTOM ] ;
    Style_st* styleBottom = &guiState->layout.styles[ GUI_STYLE_BOTTOM ] ;
    char*     fix_buf ;
    char*     type_buf ;
    Color_st  color_fg ;
    Pos_st    fix_pos     = { line2->pos.x , ( SCREEN_HEIGHT * 2 ) / 5 };

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "GPS" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->gps );
    // Print GPS status, if no fix, hide details
    if( !last_state.settings.gps_enabled )
    {
        gfx_print( fix_pos , style3Large->font.size , ALIGN_CENTER ,
                   color_fg , currentLanguage->gpsOff );
    }
    else if( last_state.gps_data.fix_quality == 0 )
    {
        gfx_print( fix_pos , style3Large->font.size , ALIGN_CENTER ,
                   color_fg , currentLanguage->noFix );
    }
    else if( last_state.gps_data.fix_quality == 6 )
    {
        gfx_print( fix_pos , style3Large->font.size , ALIGN_CENTER ,
                   color_fg , currentLanguage->fixLost );
    }
    else
    {
        switch(last_state.gps_data.fix_quality)
        {
            case 1 :
            {
                fix_buf = "SPS" ;
                break ;
            }
            case 2 :
            {
                fix_buf = "DGPS" ;
                break ;
            }
            case 3:
            {
                fix_buf = "PPS" ;
                break ;
            }
            default:
            {
                fix_buf = (char*)currentLanguage->error ;
                break ;
            }
        }

        switch(last_state.gps_data.fix_type)
        {
            case 1:
            {
                type_buf = "" ;
                break ;
            }
            case 2:
            {
                type_buf = "2D" ;
                break ;
            }
            case 3:
            {
                type_buf = "3D" ;
                break ;
            }
            default:
            {
                type_buf = (char*)currentLanguage->error ;
                break ;
            }
        }
        gfx_print( line1->pos , styleTop->font.size , ALIGN_LEFT ,
                   color_fg , fix_buf );
        gfx_print( line1->pos , styleTop->font.size , ALIGN_CENTER ,
                   color_fg , "N     " );
        gfx_print( line1->pos , styleTop->font.size , ALIGN_RIGHT ,
                   color_fg , "%8.6f" , last_state.gps_data.latitude );
        gfx_print( line2->pos , styleTop->font.size , ALIGN_LEFT ,
                   color_fg , type_buf);
        // Convert from signed longitude, to unsigned + direction
        float longitude = last_state.gps_data.longitude ;
        char* direction = (longitude < 0) ? "W     " : "E     " ;
        longitude = (longitude < 0) ? -longitude : longitude ;
        gfx_print( line2->pos , styleTop->font.size , ALIGN_CENTER ,
                   color_fg , direction );
        gfx_print( line2->pos , styleTop->font.size , ALIGN_RIGHT ,
                   color_fg , "%8.6f" , longitude );
        gfx_print( lineBottom->pos , styleBottom->font.size , ALIGN_CENTER ,
                   color_fg , "S %4.1fkm/h  A %4.1fm" ,
                   last_state.gps_data.speed , last_state.gps_data.altitude );
    }
    // Draw compass
    Pos_st compass_pos = { guiState->layout.horizontal_pad * 2 , SCREEN_HEIGHT / 2 };
    gfx_drawGPScompass( compass_pos , SCREEN_WIDTH / 9 + 2 ,
                        last_state.gps_data.tmg_true ,
                        last_state.gps_data.fix_quality != 0 &&
                        last_state.gps_data.fix_quality != 6 );
    // Draw satellites bar graph
    Pos_st bar_pos = { line3Large->pos.x + SCREEN_WIDTH * 1 / 3 , SCREEN_HEIGHT / 2 };
    gfx_drawGPSgraph( bar_pos ,
                      ( ( SCREEN_WIDTH * 2 ) / 3) - guiState->layout.horizontal_pad ,
                      SCREEN_HEIGHT / 3 ,
                      last_state.gps_data.satellites ,
                      last_state.gps_data.active_sats );
}
#endif

void _ui_drawMenuSettings( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Settings" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->settings );
    // Print menu entries
    _ui_drawMenuList( guiState , PAGE_MENU_SETTINGS );
}

void _ui_drawMenuBackupRestore( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Backup & Restore" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->backupAndRestore );
    // Print menu entries
    _ui_drawMenuList( guiState , PAGE_MENU_BACKUP_RESTORE );
}

void _ui_drawMenuBackup( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line2    = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2   = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Flash Backup" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->flashBackup );
    // Print backup message
    Pos_st line = line2->pos ;
    gfx_print( line , FONT_SIZE_8PT , ALIGN_CENTER ,
               color_fg , currentLanguage->connectToRTXTool );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( line , FONT_SIZE_8PT , ALIGN_CENTER ,
               color_fg , currentLanguage->toBackupFlashAnd );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( line , FONT_SIZE_8PT , ALIGN_CENTER ,
               color_fg , currentLanguage->pressPTTToStart );

    if( !platform_getPttStatus() )
    {
        return ;
    }

    state.devStatus     = DATATRANSFER ;
    state.backup_eflash = true ;
}

void _ui_drawMenuRestore( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line2    = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2   = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Flash Restore" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->flashRestore );
    // Print backup message
    Pos_st line = line2->pos ;
    gfx_print( line , FONT_SIZE_8PT , ALIGN_CENTER ,
               color_fg , currentLanguage->connectToRTXTool );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( line , FONT_SIZE_8PT , ALIGN_CENTER ,
               color_fg , currentLanguage->toRestoreFlashAnd );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( line , FONT_SIZE_8PT , ALIGN_CENTER ,
               color_fg , currentLanguage->pressPTTToStart );

    if( !platform_getPttStatus() )
    {
        return ;
    }

    state.devStatus      = DATATRANSFER ;
    state.restore_eflash = true ;
}

void _ui_drawMenuInfo( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Info" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->info );
    // Print menu entries
    _ui_drawMenuListValue( guiState ,
                           PAGE_MENU_INFO , PAGE_MENU_INFO );
}

void _ui_drawMenuAbout( GuiState_st* guiState )
{
    ui_States_SetPageNum( guiState , PAGE_ABOUT );
    ui_DisplayPage( guiState );
/* @@@KL
    Pos_st logo_pos;
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );
    Color_st color_op3 ;
    ui_ColorLoad( &color_op3 , COLOR_OP3 );

    gfx_clearScreen();

    if( SCREEN_HEIGHT >= 100 )
    {
        logo_pos.x = 0 ;
        logo_pos.y = SCREEN_HEIGHT / 5 ;
        gfx_print( logo_pos , FONT_SIZE_12PT , ALIGN_CENTER , color_op3 ,
                  "O P N\nR T X" );
    }
    else
    {
        logo_pos.x = guiState->layout.horizontal_pad ;
        logo_pos.y = lineStyle3Large->height ;
        gfx_print( logo_pos , style3Large->font.size , ALIGN_CENTER ,
                   color_op3 , currentLanguage->openRTX );
    }

    uint8_t line_h = guiState->layout.menu_h ;
//@@@KL check this
    Pos_st pos    = { SCREEN_WIDTH / 7 , SCREEN_HEIGHT -
                       (line_h * ( ui_States_GetPageNumOfEntries( guiState ) - 1 ) ) - 5 };
//@@@KL and this - looks like dangerous code
    for( int author = 0 ; author < ui_States_GetPageNumOfEntries( guiState ) ; author++ )
    {
        gfx_print( pos , styleTop->font.size , ALIGN_LEFT ,
                   color_fg , "%s" , *( &currentLanguage->Niccolo + author ) );
        pos.y += line_h;
    }
*/
}

void _ui_drawSettingsDisplay( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Display" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->display );
    // Print display settings entries
    _ui_drawMenuListValue( guiState ,
                           PAGE_SETTINGS_DISPLAY , PAGE_SETTINGS_DISPLAY );
}

#ifdef GPS_PRESENT
void _ui_drawSettingsGPS( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "GPS Settings" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->gpsSettings );
    // Print display settings entries
    _ui_drawMenuListValue( guiState ,
                           PAGE_SETTINGS_GPS , PAGE_SETTINGS_GPS );
}
#endif

#ifdef RTC_PRESENT
void _ui_drawSettingsTimeDate( GuiState_st* guiState )
{
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
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->timeAndDate );
    // Print current time and date
    gfx_print( line2->pos , guiState->layout.input_font.size , ALIGN_CENTER ,
               color_fg , "%02d/%02d/%02d" ,
               local_time.date , local_time.month , local_time.year );
    gfx_print( line3Large->pos , guiState->layout.input_font.size , ALIGN_CENTER ,
               color_fg , "%02d:%02d:%02d" ,
               local_time.hour , local_time.minute , local_time.second );
}

void _ui_drawSettingsTimeDateSet( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line2       = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2      = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
//    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->timeAndDate );

    if( guiState->uiState.input_position <= 0 )
    {
        strcpy( guiState->uiState.new_date_buf , "__/__/__" );
        strcpy( guiState->uiState.new_time_buf , "__:__:00" );
    }
    else
    {
        char input_char = guiState->uiState.input_number + '0' ;
        // Insert date digit
        if( guiState->uiState.input_position <= 6 )
        {
            uint8_t pos = guiState->uiState.input_position -1 ;
            // Skip "/"
            if( guiState->uiState.input_position > 2 ) pos += 1 ;
            if( guiState->uiState.input_position > 4 ) pos += 1 ;
            guiState->uiState.new_date_buf[ pos ] = input_char ;
        }
        // Insert time digit
        else
        {
            uint8_t pos = guiState->uiState.input_position -7 ;
            // Skip ":"
            if( guiState->uiState.input_position > 8 ) pos += 1 ;
            guiState->uiState.new_time_buf[ pos ] = input_char ;
        }
    }
    gfx_print( line2->pos , guiState->layout.input_font.size , ALIGN_CENTER ,
               color_fg , guiState->uiState.new_date_buf );
    gfx_print( line3Large->pos , guiState->layout.input_font.size , ALIGN_CENTER ,
               color_fg , guiState->uiState.new_time_buf );
}
#endif

void _ui_drawSettingsM17( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  lineBottom  = &guiState->layout.lines[ GUI_LINE_BOTTOM ] ;
//    Style_st* styleBottom = &guiState->layout.styles[ GUI_STYLE_BOTTOM ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "M17 Settings" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->m17settings );
    gfx_printLine( 1 , 4 , lineTop->height , SCREEN_HEIGHT - lineBottom->height ,
                   guiState->layout.horizontal_pad , guiState->layout.menu_font.size ,
                   ALIGN_LEFT , color_fg , currentLanguage->callsign );
    if( guiState->uiState.edit_mode && ( guiState->uiState.entrySelected == M17_CALLSIGN ) )
    {
        uint16_t rect_width  = SCREEN_WIDTH - ( guiState->layout.horizontal_pad * 2 ) ;
        uint16_t rect_height = ( SCREEN_HEIGHT - ( lineTop->height + lineBottom->height ) ) / 2 ;
        Pos_st  rect_origin = { ( SCREEN_WIDTH - rect_width ) / 2 ,
                                 ( SCREEN_HEIGHT - rect_height ) / 2 };
        gfx_drawRect( rect_origin , rect_width , rect_height , color_fg , false );
        // Print M17 callsign being typed
        gfx_printLine( 1 , 1 , lineTop->height , SCREEN_HEIGHT - lineBottom->height ,
                       guiState->layout.horizontal_pad , guiState->layout.input_font.size ,
                       ALIGN_CENTER , color_fg , guiState->uiState.new_callsign );
    }
    else
    {
        _ui_drawMenuListValue( guiState ,
                               PAGE_SETTINGS_M17 , PAGE_SETTINGS_M17 );
    }
}

void _ui_drawSettingsVoicePrompts( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Voice" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->voice );
    // Print voice settings entries
    _ui_drawMenuListValue( guiState ,
                           PAGE_SETTINGS_VOICE , PAGE_SETTINGS_VOICE );
}

void _ui_drawSettingsReset2Defaults( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*         lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st*        styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*         lineBottom  = &guiState->layout.lines[ GUI_LINE_BOTTOM ] ;
//    Style_st*        styleBottom = &guiState->layout.styles[ GUI_STYLE_BOTTOM ] ;
    static int       drawcnt     = 0 ;
    static long long lastDraw    = 0 ;
    Color_st         text_color ;
    Color_st         color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->resetToDefaults );

    // Make text flash yellow once every 1s
    if( drawcnt % 2 == 0 )
    {
        ui_ColorLoad( &text_color , COLOR_FG );
    }
    else
    {
        ui_ColorLoad( &text_color , COLOR_OP3 );
    }
    gfx_printLine( 1 , 4 ,
                   lineTop->height ,
                   SCREEN_HEIGHT - lineBottom->height ,
                   guiState->layout.horizontal_pad ,
                   styleTop->font.size ,
                   ALIGN_CENTER , text_color , currentLanguage->toReset );
    gfx_printLine( 2 , 4 , lineTop->height ,
                    SCREEN_HEIGHT - lineBottom->height ,
                   guiState->layout.horizontal_pad ,
                   styleTop->font.size ,
                   ALIGN_CENTER , text_color , currentLanguage->pressEnterTwice );

    if( ( getTick() - lastDraw ) > 1000 )
    {
        drawcnt++ ;
        lastDraw = getTick();
    }

    drawcnt++ ;
}

void _ui_drawSettingsRadio( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  lineBottom  = &guiState->layout.lines[ GUI_LINE_BOTTOM ] ;
//    Style_st* styleBottom = &guiState->layout.styles[ GUI_STYLE_BOTTOM ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();

    // Print "Radio Settings" on top bar
    gfx_print( lineTop->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->radioSettings );

    // Handle the special case where a frequency is being input
    if( ( guiState->uiState.entrySelected == R_OFFSET ) && ( guiState->uiState.edit_mode ) )
    {
        char buf[ 17 ] = { 0 };
        uint16_t rect_width  = SCREEN_WIDTH - ( guiState->layout.horizontal_pad * 2 );
        uint16_t rect_height = ( SCREEN_HEIGHT - ( lineTop->height + lineBottom->height ) ) / 2 ;
        Pos_st  rect_origin = { ( SCREEN_WIDTH - rect_width ) / 2 ,
                                 (SCREEN_HEIGHT - rect_height) / 2   };

        gfx_drawRect(rect_origin, rect_width, rect_height, color_fg, false);

        // Print frequency with the most sensible unit
        if( guiState->uiState.new_offset < 1000 )
        {
            snprintf( buf , 17 , "%dHz" , (int)guiState->uiState.new_offset );
        }
        else if( guiState->uiState.new_offset < 1000000 )
        {
            snprintf( buf , 17 , "%gkHz" , (float)guiState->uiState.new_offset / 1000.0f );
        }
        else
        {
            snprintf( buf , 17 , "%gMHz" , (float)guiState->uiState.new_offset / 1000000.0f );
        }
        gfx_printLine( 1 , 1 , lineTop->height , SCREEN_HEIGHT - lineBottom->height ,
                       guiState->layout.horizontal_pad , guiState->layout.input_font.size ,
                       ALIGN_CENTER , color_fg, buf );
    }
    else
    {
        // Print radio settings entries
        _ui_drawMenuListValue( guiState ,
                               PAGE_SETTINGS_RADIO , PAGE_SETTINGS_RADIO );
    }
}

void _ui_drawMacroTop( GuiState_st* guiState )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_print( lineTop->pos ,
                styleTop->font.size , ALIGN_CENTER ,
               color_fg , currentLanguage->macroMenu );

    if( macro_latched )
    {
        gfx_drawSymbol( lineTop->pos ,
                        styleTop->symbolSize , ALIGN_LEFT ,
                        color_fg , SYMBOL_ALPHA_M_BOX_OUTLINE );
    }
    if( last_state.settings.gps_enabled )
    {
        if( last_state.gps_data.fix_quality > 0 )
        {
            gfx_drawSymbol( lineTop->pos , styleTop->symbolSize , ALIGN_RIGHT ,
                            color_fg , SYMBOL_CROSSHAIRS_GPS );
        }
        else
        {
            gfx_drawSymbol( lineTop->pos , styleTop->symbolSize , ALIGN_RIGHT ,
                            color_fg , SYMBOL_CROSSHAIRS );
        }
    }
}

bool _ui_drawMacroMenu( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
//    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line1       = &guiState->layout.lines[ GUI_LINE_1 ] ;
//    Style_st* style1      = &guiState->layout.styles[ GUI_STYLE_1 ] ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
//    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    Color_st  color_fg ;
    Color_st  color_op3 ;

    ui_ColorLoad( &color_fg , COLOR_FG );
    ui_ColorLoad( &color_op3 , COLOR_OP3 );

    // Header
    _ui_drawMacroTop( guiState );
    // First row
    if( last_state.channel.mode == OPMODE_FM )
    {
/*
 * If we have a keyboard installed draw all numbers, otherwise draw only the
 * currently selected number.
 */
#ifdef UI_NO_KEYBOARD
        if( guiState->uiState.macro_entrySelected == 0 )
        {
#endif // UI_NO_KEYBOARD
            gfx_print( line1->pos , styleTop->font.size , ALIGN_LEFT ,
                       color_op3 , "1" );
            gfx_print( line1->pos , styleTop->font.size , ALIGN_LEFT ,
                       color_fg , "   T-" );
            gfx_print( line1->pos , styleTop->font.size , ALIGN_LEFT ,
                        color_fg , "     %7.1f" ,
                        ctcss_tone[ last_state.channel.fm.txTone ] / 10.0f );
#ifdef UI_NO_KEYBOARD
        }
#endif // UI_NO_KEYBOARD
#ifdef UI_NO_KEYBOARD
        if( guiState->uiState.macro_entrySelected == 1 )
        {
#endif // UI_NO_KEYBOARD
            gfx_print( line1->pos , styleTop->font.size , ALIGN_CENTER ,
                       color_op3 , "2" );
            gfx_print( line1->pos , styleTop->font.size , ALIGN_CENTER ,
                       color_fg ,   "       T+" );
#ifdef UI_NO_KEYBOARD
        }
#endif // UI_NO_KEYBOARD
    }
    else
    {
        if( last_state.channel.mode == OPMODE_M17 )
        {
            gfx_print( line1->pos , styleTop->font.size , ALIGN_LEFT ,
                       color_op3 , "1" );
            gfx_print( line1->pos , styleTop->font.size , ALIGN_LEFT ,
                       color_fg , "          " );
            gfx_print( line1->pos , styleTop->font.size , ALIGN_CENTER ,
                       color_op3 , "2" );
        }
    }
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 2 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( line1->pos , styleTop->font.size , ALIGN_RIGHT ,
                   color_op3 , "3        " );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD

    if( last_state.channel.mode == OPMODE_FM )
    {
        char encdec_str[ 9 ] = { 0 };
        bool tone_tx_enable = last_state.channel.fm.txToneEn ;
        bool tone_rx_enable = last_state.channel.fm.rxToneEn ;

        if( tone_tx_enable && tone_rx_enable )
        {
            snprintf( encdec_str , 9 , "     E+D" );
        }
        else
        {
            if( tone_tx_enable && !tone_rx_enable )
            {
                snprintf( encdec_str , 9 , "      E " );
            }
            else
            {
                if( !tone_tx_enable && tone_rx_enable )
                {
                    snprintf( encdec_str , 9 , "      D " );
                }
                else
                {
                    snprintf( encdec_str , 9 , "        " );
                }
            }
        }
        gfx_print( line1->pos , styleTop->font.size , ALIGN_RIGHT ,
                   color_fg , encdec_str );
    }
    else
    {
        if ( last_state.channel.mode == OPMODE_M17 )
        {
            char encdec_str[ 9 ] = "        " ;
            gfx_print( line1->pos , styleTop->font.size , ALIGN_CENTER ,
                       color_fg , encdec_str );
        }
    }
    // Second row
    // Calculate symmetric second row position, line2_pos is asymmetric like main screen
    Pos_st pos_2 = { line1->pos.x ,
                      line1->pos.y + ( line3Large->pos.y - line3Large->pos.y ) / 2 };
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 3 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( pos_2 , styleTop->font.size , ALIGN_LEFT ,
                   color_op3 , "4" );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
    if( last_state.channel.mode == OPMODE_FM )
    {
        char bw_str[ 12 ] = { 0 };
        switch( last_state.channel.bandwidth )
        {
            case BW_12_5 :
            {
                snprintf( bw_str , 12 , "   BW 12.5" );
                break ;
            }
            case BW_20 :
            {
                snprintf( bw_str , 12 , "   BW  20 " );
                break ;
            }
            case BW_25 :
            {
                snprintf( bw_str , 12 , "   BW  25 " );
                break ;
            }
        }
        gfx_print( pos_2 , styleTop->font.size , ALIGN_LEFT ,
                   color_fg , bw_str );
    }
    else
    {
        if( last_state.channel.mode == OPMODE_M17 )
        {
            gfx_print( pos_2 , styleTop->font.size , ALIGN_LEFT ,
                       color_fg , "       " );

        }
    }
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 4 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( pos_2 , styleTop->font.size , ALIGN_CENTER ,
                   color_op3 , "5" );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
    char mode_str[12] = "";
    switch( last_state.channel.mode )
    {
        case OPMODE_FM :
        {
            snprintf( mode_str , 12 ,"         FM" );
            break ;
        }
        case OPMODE_DMR :
        {
            snprintf( mode_str , 12 ,"        DMR" );
            break ;
        }
        case OPMODE_M17 :
        {
            snprintf( mode_str , 12 ,"        M17" );
            break ;
        }
    }
    gfx_print( pos_2 , styleTop->font.size , ALIGN_CENTER ,
               color_fg , mode_str );
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 5 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( pos_2 , styleTop->font.size , ALIGN_RIGHT ,
                   color_op3 , "6        " );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
    gfx_print( pos_2 , styleTop->font.size , ALIGN_RIGHT ,
               color_fg , "%.1gW" , dBmToWatt( last_state.channel.power ) );
    // Third row
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 6 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( line3Large->pos , styleTop->font.size , ALIGN_LEFT ,
                   color_op3 , "7" );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
#ifdef SCREEN_BRIGHTNESS
    gfx_print( line3Large->pos , styleTop->font.size , ALIGN_LEFT ,
               color_fg , "   B-" );
    gfx_print( line3Large->pos , styleTop->font.size , ALIGN_LEFT ,
               color_fg , "       %5d" , state.settings.brightness );
#endif
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 7 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( line3Large->pos , styleTop->font.size , ALIGN_CENTER ,
                   color_op3 , "8" );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
#ifdef SCREEN_BRIGHTNESS
    gfx_print( line3Large->pos , styleTop->font.size , ALIGN_CENTER ,
               color_fg ,   "       B+" );
#endif
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 8 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( line3Large->pos , styleTop->font.size , ALIGN_RIGHT ,
                   color_op3 , "9        " );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
    if( guiState->uiState.input_locked == true )
    {
       gfx_print( line3Large->pos , styleTop->font.size , ALIGN_RIGHT ,
                  color_fg , "Unlk" );
    }
    else
    {
       gfx_print( line3Large->pos , styleTop->font.size , ALIGN_RIGHT ,
                  color_fg , "Lck" );
    }
    // Draw S-meter bar
    _ui_drawMainBottom( guiState , event );
    return true;
}
