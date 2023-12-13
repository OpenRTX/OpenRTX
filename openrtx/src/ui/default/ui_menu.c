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
#include <interfaces/nvmem.h>
#include <interfaces/cps_io.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <memory_profiling.h>
#include <ui/ui_strings.h>
#include <core/voicePromptUtils.h>

#ifdef PLATFORM_TTWRPLUS
#include <SA8x8.h>
#endif

/* UI main screen helper functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainBottom( GuiState_st* guiState , Event_st* event );

static char     priorSelectedMenuName[ MAX_ENTRY_LEN ]  = "\0" ;
static char     priorSelectedMenuValue[ MAX_ENTRY_LEN ] = "\0" ;
static bool     priorEditMode                           = false ;
static uint32_t lastValueUpdate                         = 0 ;

const char *display_timer_values[] =
{
    "Off" ,
    "5 s" ,
    "10 s" ,
    "15 s" ,
    "20 s" ,
    "25 s" ,
    "30 s" ,
    "1 min" ,
    "2 min" ,
    "3 min" ,
    "4 min" ,
    "5 min" ,
    "15 min" ,
    "30 min" ,
    "45 min" ,
    "1 hour"
};

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

static void announceMenuItemIfNeeded( char* name , char* value , bool editMode )
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
    if( !value && !editMode && !ScreenContainsReadOnlyEntries( state.ui_screen ) )
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

void _ui_drawMenuList( GuiState_st* guiState , uint8_t selected , int (*getCurrentEntry)( char* buf , uint8_t max_len , uint8_t index ) )
{
    point_t pos = guiState->layout.line1_pos ;
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen = ( SCREEN_HEIGHT - 1 - pos.y ) / guiState->layout.menu_h + 1 ;
    uint8_t scroll = 0 ;
    char entry_buf[ MAX_ENTRY_LEN ] = "" ;
    color_t text_color = Color_White ;

    for( int item = 0 , result = 0 ; ( result == 0 ) && ( pos.y < SCREEN_HEIGHT ); item++ )
    {
        // If selection is off the screen, scroll screen
        if( selected >= entries_in_screen )
        {
            scroll = selected - entries_in_screen + 1 ;
        }

        // Call function pointer to get current menu entry string
        result = (*getCurrentEntry)( entry_buf , sizeof( entry_buf ) , item + scroll );

        if( result != -1 )
        {
            text_color = Color_White ;

            if( ( item + scroll ) == selected )
            {
                text_color = Color_Black ;
                // Draw rectangle under selected item, compensating for text height
                point_t rect_pos = { 0 , pos.y - guiState->layout.menu_h + 3 };
                gfx_drawRect( rect_pos , SCREEN_WIDTH , guiState->layout.menu_h , Color_White , true );
                announceMenuItemIfNeeded( entry_buf , NULL , false );
            }
            gfx_print( pos , guiState->layout.menu_font , TEXT_ALIGN_LEFT , text_color , entry_buf );
            pos.y += guiState->layout.menu_h ;
        }
    }
}

void _ui_drawMenuListValue( GuiState_st* guiState , UI_State_st* uiState , uint8_t selected ,
                            int (*getCurrentEntry)( char* buf , uint8_t max_len , uint8_t index ) ,
                            int (*getCurrentValue)( char* buf , uint8_t max_len , uint8_t index )   )
{
    point_t pos = guiState->layout.line1_pos;
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen = ( SCREEN_HEIGHT - 1 - pos.y ) / guiState->layout.menu_h + 1 ;
    uint8_t scroll = 0 ;
    char entry_buf[ MAX_ENTRY_LEN ] = "" ;
    char value_buf[ MAX_ENTRY_LEN ] = "" ;
    color_t text_color = Color_White ;

    for( int item = 0 , result = 0 ; ( result == 0 ) && ( pos.y < SCREEN_HEIGHT ) ; item++ )
    {
        // If selection is off the screen, scroll screen
        if( selected >= entries_in_screen )
        {
            scroll = ( selected - entries_in_screen ) + 1 ; //@@@KL check this
        }
        // Call function pointer to get current menu entry string
        result = (*getCurrentEntry)( entry_buf , sizeof( entry_buf ) , item+scroll );
        // Call function pointer to get current entry value string
        result = (*getCurrentValue)( value_buf , sizeof( value_buf ) , item+scroll );
        if( result != -1 )
        {
            text_color = Color_White;
            if( ( item + scroll ) == selected )
            {
                // Draw rectangle under selected item, compensating for text height
                // If we are in edit mode, draw a hollow rectangle
                     text_color = Color_Black ;
                bool full_rect  = true ;

                if( uiState->edit_mode )
                {
                    text_color = Color_White ;
                    full_rect  = false ;
                }
                point_t rect_pos = { 0 , pos.y - guiState->layout.menu_h + 3 };
                gfx_drawRect( rect_pos , SCREEN_WIDTH , guiState->layout.menu_h , Color_White , full_rect );
                bool editModeChanged = priorEditMode != uiState->edit_mode ;
                priorEditMode = uiState->edit_mode ;
                // force the menu item to be spoken  when the edit mode changes.
                // E.g. when pressing Enter on Display Brightness etc.
                if( editModeChanged )
                {
                    priorSelectedMenuName[ 0 ] = '\0' ;
                }
                if( !uiState->edit_mode || editModeChanged )
                {// If in edit mode, only want to speak the char being entered,,
            //not repeat the entire display.
                    announceMenuItemIfNeeded( entry_buf , value_buf , uiState->edit_mode );
                }
            }
            gfx_print( pos , guiState->layout.menu_font , TEXT_ALIGN_LEFT , text_color , entry_buf );
            gfx_print( pos , guiState->layout.menu_font , TEXT_ALIGN_RIGHT , text_color , value_buf );
            pos.y += guiState->layout.menu_h ;
        }
    }
}

int _ui_getMenuTopEntryName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >=  uiGetPageNumOf( PAGE_MENU_TOP ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , Page_MenuItems[ index ] );
    return 0;
}

int _ui_getSettingsEntryName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_MENU_SETTINGS ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , Page_MenuSettings[ index ] );
    return 0;
}

int _ui_getDisplayEntryName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_SETTINGS_DISPLAY ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , Page_SettingsDisplay[index]);
    return 0;
}

int _ui_getDisplayValueName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_SETTINGS_DISPLAY ) )
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
int _ui_getSettingsGPSEntryName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_SETTINGS_GPS ) )
    {
        return -1 ;
    }
    snprintf( buf , max_len , "%s" , Page_SettingsGPS[ index ] );
    return 0 ;
}

int _ui_getSettingsGPSValueName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_SETTINGS_GPS ) )
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

int _ui_getRadioEntryName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_SETTINGS_RADIO ) )
    {
        return -1 ;
    }
    snprintf( buf , max_len , "%s" , Page_SettingsRadio[ index ] );
    return 0 ;
}

int _ui_getRadioValueName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_SETTINGS_RADIO ) )
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

int _ui_getM17EntryName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_SETTINGS_M17 ) )
    {
       return -1 ;
    }
    snprintf( buf , max_len , "%s" , Page_SettingsM17[ index ] );
    return 0;
}

int _ui_getM17ValueName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_SETTINGS_M17 ) )
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

int _ui_getVoiceEntryName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_SETTINGS_VOICE ) )
    {
        return -1 ;
    }
    snprintf( buf , max_len , "%s" , Page_SettingsVoice[ index ] );
    return 0 ;
}

int _ui_getVoiceValueName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_SETTINGS_VOICE ) )
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

int _ui_getBackupRestoreEntryName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_MENU_BACKUP_RESTORE ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , Page_MenuBackupRestore[index]);
    return 0;
}

int _ui_getInfoEntryName( char* buf , uint8_t max_len , uint8_t index )
{
    if( index >= uiGetPageNumOf( PAGE_MENU_INFO ) )
    {
        return -1;
    }
    snprintf( buf , max_len , "%s" , Page_MenuInfo[index]);
    return 0;
}

int _ui_getInfoValueName( char* buf , uint8_t max_len , uint8_t index )
{
    const hwInfo_t* hwinfo = platform_getHwInfo();
    if( index >= uiGetPageNumOf( PAGE_MENU_INFO ) )
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

int _ui_getBankName( char* buf , uint8_t max_len , uint8_t index )
{
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

int _ui_getChannelName( char* buf , uint8_t max_len , uint8_t index )
{
    channel_t channel ;
    int       result = cps_readChannel( &channel , index );

    if( result != -1 )
    {
        snprintf( buf , max_len , "%s" , channel.name );
    }

    return result ;
}

int _ui_getContactName( char* buf , uint8_t max_len , uint8_t index )
{
    contact_t contact ;
    int       result = cps_readContact( &contact , index );

    if( result != -1 )
    {
        snprintf( buf , max_len , "%s" , contact.name );
    }

    return result ;
}

void _ui_drawMenuTop( GuiState_st* guiState , UI_State_st* uiState )
{
    gfx_clearScreen();
    // Print "Menu" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->menu );
    // Print menu entries
    _ui_drawMenuList( guiState , uiState->menu_selected , _ui_getMenuTopEntryName );
}

void _ui_drawMenuBank( GuiState_st* guiState , UI_State_st* uiState )
{
    gfx_clearScreen();
    // Print "Bank" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->banks );
    // Print bank entries
    _ui_drawMenuList( guiState , uiState->menu_selected , _ui_getBankName );
}

void _ui_drawMenuChannel( GuiState_st* guiState , UI_State_st* uiState )
{
    gfx_clearScreen();
    // Print "Channel" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->channels );
    // Print channel entries
    _ui_drawMenuList( guiState , uiState->menu_selected , _ui_getChannelName );
}

void _ui_drawMenuContacts( GuiState_st* guiState , UI_State_st* uiState )
{
    gfx_clearScreen();
    // Print "Contacts" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->contacts );
    // Print contact entries
    _ui_drawMenuList( guiState , uiState->menu_selected , _ui_getContactName );
}

#ifdef GPS_PRESENT
void _ui_drawMenuGPS( GuiState_st* guiState , UI_State_st* uiState )
{
    (void)uiState ;
    char* fix_buf ;
    char* type_buf ;

    gfx_clearScreen();
    // Print "GPS" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->gps );
    point_t fix_pos = { guiState->layout.line2_pos.x , ( SCREEN_HEIGHT * 2 ) / 5 };
    // Print GPS status, if no fix, hide details
    if( !last_state.settings.gps_enabled )
    {
        gfx_print( fix_pos , guiState->layout.line3_large_font , TEXT_ALIGN_CENTER ,
                   Color_White , currentLanguage->gpsOff );
    }
    else if( last_state.gps_data.fix_quality == 0 )
    {
        gfx_print( fix_pos , guiState->layout.line3_large_font , TEXT_ALIGN_CENTER ,
                   Color_White , currentLanguage->noFix );
    }
    else if( last_state.gps_data.fix_quality == 6 )
    {
        gfx_print( fix_pos , guiState->layout.line3_large_font , TEXT_ALIGN_CENTER ,
                   Color_White , currentLanguage->fixLost );
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
        gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                   Color_White , fix_buf );
        gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
                   Color_White , "N     " );
        gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_RIGHT ,
                   Color_White , "%8.6f" , last_state.gps_data.latitude );
        gfx_print( guiState->layout.line2_pos , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                   Color_White , type_buf);
        // Convert from signed longitude, to unsigned + direction
        float longitude = last_state.gps_data.longitude ;
        char* direction = (longitude < 0) ? "W     " : "E     " ;
        longitude = (longitude < 0) ? -longitude : longitude ;
        gfx_print( guiState->layout.line2_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
                   Color_White , direction );
        gfx_print( guiState->layout.line2_pos , guiState->layout.top_font , TEXT_ALIGN_RIGHT ,
                   Color_White , "%8.6f" , longitude );
        gfx_print( guiState->layout.bottom_pos , guiState->layout.bottom_font , TEXT_ALIGN_CENTER ,
                   Color_White , "S %4.1fkm/h  A %4.1fm" ,
                   last_state.gps_data.speed , last_state.gps_data.altitude );
    }
    // Draw compass
    point_t compass_pos = { guiState->layout.horizontal_pad * 2 , SCREEN_HEIGHT / 2 };
    gfx_drawGPScompass( compass_pos , SCREEN_WIDTH / 9 + 2 ,
                        last_state.gps_data.tmg_true ,
                        last_state.gps_data.fix_quality != 0 &&
                        last_state.gps_data.fix_quality != 6 );
    // Draw satellites bar graph
    point_t bar_pos = { guiState->layout.line3_large_pos.x + SCREEN_WIDTH * 1 / 3 , SCREEN_HEIGHT / 2 };
    gfx_drawGPSgraph( bar_pos ,
                      ( ( SCREEN_WIDTH * 2 ) / 3) - guiState->layout.horizontal_pad ,
                      SCREEN_HEIGHT / 3 ,
                      last_state.gps_data.satellites ,
                      last_state.gps_data.active_sats );
}
#endif

void _ui_drawMenuSettings( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();
    // Print "Settings" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->settings );
    // Print menu entries
    _ui_drawMenuList( guiState , uiState->menu_selected , _ui_getSettingsEntryName );
}

void _ui_drawMenuBackupRestore( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();
    // Print "Backup & Restore" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->backupAndRestore );
    // Print menu entries
    _ui_drawMenuList( guiState , uiState->menu_selected , _ui_getBackupRestoreEntryName );
}

void _ui_drawMenuBackup( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)uiState ;
    (void)event ;

    gfx_clearScreen();
    // Print "Flash Backup" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->flashBackup );
    // Print backup message
    point_t line = guiState->layout.line2_pos ;
    gfx_print( line , FONT_SIZE_8PT , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->connectToRTXTool );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( line , FONT_SIZE_8PT , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->toBackupFlashAnd );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( line , FONT_SIZE_8PT , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->pressPTTToStart );

    if( !platform_getPttStatus() )
    {
        return ;
    }

    state.devStatus     = DATATRANSFER ;
    state.backup_eflash = true ;
}

void _ui_drawMenuRestore( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)uiState ;
    (void)event ;

    gfx_clearScreen();
    // Print "Flash Restore" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->flashRestore );
    // Print backup message
    point_t line = guiState->layout.line2_pos ;
    gfx_print( line , FONT_SIZE_8PT , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->connectToRTXTool );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( line , FONT_SIZE_8PT , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->toRestoreFlashAnd );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( line , FONT_SIZE_8PT , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->pressPTTToStart );

    if( !platform_getPttStatus() )
    {
        return ;
    }

    state.devStatus      = DATATRANSFER ;
    state.restore_eflash = true ;
}

void _ui_drawMenuInfo( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();
    // Print "Info" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->info );
    // Print menu entries
    _ui_drawMenuListValue( guiState , uiState , uiState->menu_selected , _ui_getInfoEntryName ,
                           _ui_getInfoValueName );
}

void _ui_drawMenuAbout( GuiState_st* guiState , UI_State_st* uiState )
{
    (void)uiState ;
    point_t logo_pos;

    gfx_clearScreen();

    if( SCREEN_HEIGHT >= 100 )
    {
        logo_pos.x = 0 ;
        logo_pos.y = SCREEN_HEIGHT / 5 ;
        gfx_print( logo_pos , FONT_SIZE_12PT , TEXT_ALIGN_CENTER , Color_Yellow_Fab413 ,
                  "O P N\nR T X" );
    }
    else
    {
        logo_pos.x = guiState->layout.horizontal_pad ;
        logo_pos.y = guiState->layout.line3_large_h ;
        gfx_print( logo_pos , guiState->layout.line3_large_font , TEXT_ALIGN_CENTER ,
                   Color_Yellow_Fab413 , currentLanguage->openRTX );
    }

    uint8_t line_h = guiState->layout.menu_h ;
//@@@KL check this
    point_t pos    = { SCREEN_WIDTH / 7 , SCREEN_HEIGHT -
                       (line_h * ( uiGetPageNumOf( PAGE_AUTHORS ) - 1 ) ) - 5 };
//@@@KL and this - looks like dangerous code
    for( int author = 0 ; author < uiGetPageNumOf( PAGE_AUTHORS ) ; author++ )
    {
        gfx_print( pos , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                   Color_White , "%s" , *( &currentLanguage->Niccolo + author ) );
        pos.y += line_h;
    }

}

void _ui_drawSettingsDisplay( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();
    // Print "Display" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->display );
    // Print display settings entries
    _ui_drawMenuListValue( guiState , uiState , uiState->menu_selected , _ui_getDisplayEntryName ,
                           _ui_getDisplayValueName );
}

#ifdef GPS_PRESENT
void _ui_drawSettingsGPS( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();
    // Print "GPS Settings" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->gpsSettings );
    // Print display settings entries
    _ui_drawMenuListValue( guiState , uiState , uiState->menu_selected ,
                           _ui_getSettingsGPSEntryName ,
                           _ui_getSettingsGPSValueName );
}
#endif

#ifdef RTC_PRESENT
void _ui_drawSettingsTimeDate( GuiState_st* guiState )
{
    gfx_clearScreen();
    datetime_t local_time = utcToLocalTime( last_state.time ,
                                            last_state.settings.utc_timezone );
    // Print "Time&Date" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->timeAndDate );
    // Print current time and date
    gfx_print( guiState->layout.line2_pos , guiState->layout.input_font , TEXT_ALIGN_CENTER ,
               Color_White , "%02d/%02d/%02d" ,
               local_time.date , local_time.month , local_time.year );
    gfx_print( guiState->layout.line3_large_pos , guiState->layout.input_font , TEXT_ALIGN_CENTER ,
               Color_White , "%02d:%02d:%02d" ,
               local_time.hour , local_time.minute , local_time.second );
}

void _ui_drawSettingsTimeDateSet( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->timeAndDate );

    if( uiState->input_position <= 0 )
    {
        strcpy( uiState->new_date_buf , "__/__/__" );
        strcpy( uiState->new_time_buf , "__:__:00" );
    }
    else
    {
        char input_char = uiState->input_number + '0' ;
        // Insert date digit
        if( uiState->input_position <= 6 )
        {
            uint8_t pos = uiState->input_position -1 ;
            // Skip "/"
            if( uiState->input_position > 2 ) pos += 1 ;
            if( uiState->input_position > 4 ) pos += 1 ;
            uiState->new_date_buf[ pos ] = input_char ;
        }
        // Insert time digit
        else
        {
            uint8_t pos = uiState->input_position -7 ;
            // Skip ":"
            if( uiState->input_position > 8 ) pos += 1 ;
            uiState->new_time_buf[ pos ] = input_char ;
        }
    }
    gfx_print( guiState->layout.line2_pos , guiState->layout.input_font , TEXT_ALIGN_CENTER ,
               Color_White , uiState->new_date_buf );
    gfx_print( guiState->layout.line3_large_pos , guiState->layout.input_font , TEXT_ALIGN_CENTER ,
               Color_White , uiState->new_time_buf );
}
#endif

void _ui_drawSettingsM17( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();
    // Print "M17 Settings" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->m17settings );
    gfx_printLine( 1 , 4 , guiState->layout.top_h , SCREEN_HEIGHT - guiState->layout.bottom_h ,
                   guiState->layout.horizontal_pad , guiState->layout.menu_font ,
                   TEXT_ALIGN_LEFT , Color_White , currentLanguage->callsign );
    if( uiState->edit_mode && ( uiState->menu_selected == M17_CALLSIGN ) )
    {
        uint16_t rect_width  = SCREEN_WIDTH - ( guiState->layout.horizontal_pad * 2 ) ;
        uint16_t rect_height = ( SCREEN_HEIGHT - ( guiState->layout.top_h + guiState->layout.bottom_h ) ) / 2 ;
        point_t  rect_origin = { ( SCREEN_WIDTH - rect_width ) / 2 ,
                                 ( SCREEN_HEIGHT - rect_height ) / 2 };
        gfx_drawRect( rect_origin , rect_width , rect_height , Color_White , false );
        // Print M17 callsign being typed
        gfx_printLine( 1 , 1 , guiState->layout.top_h , SCREEN_HEIGHT - guiState->layout.bottom_h ,
                       guiState->layout.horizontal_pad , guiState->layout.input_font ,
                       TEXT_ALIGN_CENTER , Color_White , uiState->new_callsign );
    }
    else
    {
        _ui_drawMenuListValue( guiState , uiState , uiState->menu_selected , _ui_getM17EntryName ,
                               _ui_getM17ValueName );
    }
}

void _ui_drawSettingsVoicePrompts( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();
    // Print "Voice" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->voice );
    // Print voice settings entries
    _ui_drawMenuListValue( guiState , uiState , uiState->menu_selected , _ui_getVoiceEntryName ,
                           _ui_getVoiceValueName );
}

void _ui_drawSettingsReset2Defaults( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)uiState ;
    (void)event ;

    static int       drawcnt  = 0 ;
    static long long lastDraw = 0 ;

    gfx_clearScreen();
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->resetToDefaults );

    // Make text flash yellow once every 1s
    color_t textcolor = drawcnt % 2 == 0 ? Color_White : Color_Yellow_Fab413 ;
    gfx_printLine( 1 , 4 , guiState->layout.top_h , SCREEN_HEIGHT - guiState->layout.bottom_h ,
                   guiState->layout.horizontal_pad , guiState->layout.top_font ,
                   TEXT_ALIGN_CENTER , textcolor , currentLanguage->toReset );
    gfx_printLine( 2 , 4 , guiState->layout.top_h , SCREEN_HEIGHT - guiState->layout.bottom_h ,
                   guiState->layout.horizontal_pad , guiState->layout.top_font ,
                   TEXT_ALIGN_CENTER , textcolor , currentLanguage->pressEnterTwice );

    if( ( getTick() - lastDraw ) > 1000 )
    {
        drawcnt++ ;
        lastDraw = getTick();
    }

    drawcnt++ ;
}

void _ui_drawSettingsRadio( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();

    // Print "Radio Settings" on top bar
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->radioSettings );

    // Handle the special case where a frequency is being input
    if( ( uiState->menu_selected == R_OFFSET ) && ( uiState->edit_mode ) )
    {
        char buf[ 17 ] = { 0 };
        uint16_t rect_width  = SCREEN_WIDTH - ( guiState->layout.horizontal_pad * 2 );
        uint16_t rect_height = ( SCREEN_HEIGHT - ( guiState->layout.top_h + guiState->layout.bottom_h ) ) / 2 ;
        point_t  rect_origin = { ( SCREEN_WIDTH - rect_width ) / 2 ,
                                 (SCREEN_HEIGHT - rect_height) / 2   };

        gfx_drawRect(rect_origin, rect_width, rect_height, Color_White, false);

        // Print frequency with the most sensible unit
        if( uiState->new_offset < 1000 )
        {
            snprintf( buf , 17 , "%dHz" , (int)uiState->new_offset );
        }
        else if( uiState->new_offset < 1000000 )
        {
            snprintf( buf , 17 , "%gkHz" , (float)uiState->new_offset / 1000.0f );
        }
        else
        {
            snprintf( buf , 17 , "%gMHz" , (float)uiState->new_offset / 1000000.0f );
        }
        gfx_printLine( 1 , 1 , guiState->layout.top_h , SCREEN_HEIGHT - guiState->layout.bottom_h ,
                       guiState->layout.horizontal_pad , guiState->layout.input_font ,
                       TEXT_ALIGN_CENTER , Color_White, buf );
    }
    else
    {
        // Print radio settings entries
        _ui_drawMenuListValue( guiState , uiState , uiState->menu_selected , _ui_getRadioEntryName ,
                               _ui_getRadioValueName );
    }
}

void _ui_drawMacroTop( GuiState_st* guiState )
{
    gfx_print( guiState->layout.top_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , currentLanguage->macroMenu );

    if( macro_latched )
    {
        gfx_drawSymbol( guiState->layout.top_pos , guiState->layout.top_symbol_size , TEXT_ALIGN_LEFT ,
                        Color_White , SYMBOL_ALPHA_M_BOX_OUTLINE );
    }
    if( last_state.settings.gps_enabled )
    {
        if( last_state.gps_data.fix_quality > 0 )
        {
            gfx_drawSymbol( guiState->layout.top_pos , guiState->layout.top_symbol_size , TEXT_ALIGN_RIGHT ,
                            Color_White , SYMBOL_CROSSHAIRS_GPS );
        }
        else
        {
            gfx_drawSymbol( guiState->layout.top_pos , guiState->layout.top_symbol_size , TEXT_ALIGN_RIGHT ,
                            Color_White , SYMBOL_CROSSHAIRS );
        }
    }
}

bool _ui_drawMacroMenu( GuiState_st* guiState , UI_State_st* uiState , Event_st* event )
{
    (void)event ;

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
        if( uiState->macro_menu_selected == 0 )
        {
#endif // UI_NO_KEYBOARD
            gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                       Color_Yellow_Fab413 , "1" );
            gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                       Color_White , "   T-" );
            gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                        Color_White , "     %7.1f" ,
                        ctcss_tone[ last_state.channel.fm.txTone ] / 10.0f );
#ifdef UI_NO_KEYBOARD
        }
#endif // UI_NO_KEYBOARD
#ifdef UI_NO_KEYBOARD
        if( uiState->macro_menu_selected == 1 )
        {
#endif // UI_NO_KEYBOARD
            gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
                       Color_Yellow_Fab413 , "2" );
            gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
                       Color_White ,   "       T+" );
#ifdef UI_NO_KEYBOARD
        }
#endif // UI_NO_KEYBOARD
    }
    else
    {
        if( last_state.channel.mode == OPMODE_M17 )
        {
            gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                       Color_Yellow_Fab413 , "1" );
            gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                       Color_White , "          " );
            gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
                       Color_Yellow_Fab413 , "2" );
        }
    }
#ifdef UI_NO_KEYBOARD
    if( uiState->macro_menu_selected == 2 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_RIGHT ,
                   Color_Yellow_Fab413 , "3        " );
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
        gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_RIGHT ,
                   Color_White , encdec_str );
    }
    else
    {
        if ( last_state.channel.mode == OPMODE_M17 )
        {
            char encdec_str[ 9 ] = "        " ;
            gfx_print( guiState->layout.line1_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
                       Color_White , encdec_str );
        }
    }
    // Second row
    // Calculate symmetric second row position, line2_pos is asymmetric like main screen
    point_t pos_2 = { guiState->layout.line1_pos.x ,
                      guiState->layout.line1_pos.y + ( guiState->layout.line3_large_pos.y - guiState->layout.line1_pos.y ) / 2 };
#ifdef UI_NO_KEYBOARD
    if( uiState->macro_menu_selected == 3 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( pos_2 , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                   Color_Yellow_Fab413 , "4" );
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
        gfx_print( pos_2 , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                   Color_White , bw_str );
    }
    else
    {
        if( last_state.channel.mode == OPMODE_M17 )
        {
            gfx_print( pos_2 , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                       Color_White , "       " );

        }
    }
#ifdef UI_NO_KEYBOARD
    if( uiState->macro_menu_selected == 4 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( pos_2 , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
                   Color_Yellow_Fab413 , "5" );
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
    gfx_print( pos_2 , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White , mode_str );
#ifdef UI_NO_KEYBOARD
    if( uiState->macro_menu_selected == 5 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( pos_2 , guiState->layout.top_font , TEXT_ALIGN_RIGHT ,
                   Color_Yellow_Fab413 , "6        " );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
    gfx_print( pos_2 , guiState->layout.top_font , TEXT_ALIGN_RIGHT ,
               Color_White , "%.1gW" , dBmToWatt( last_state.channel.power ) );
    // Third row
#ifdef UI_NO_KEYBOARD
    if( uiState->macro_menu_selected == 6 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( guiState->layout.line3_large_pos , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
                   Color_Yellow_Fab413 , "7" );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
#ifdef SCREEN_BRIGHTNESS
    gfx_print( guiState->layout.line3_large_pos , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
               Color_White , "   B-" );
    gfx_print( guiState->layout.line3_large_pos , guiState->layout.top_font , TEXT_ALIGN_LEFT ,
               Color_White , "       %5d" , state.settings.brightness );
#endif
#ifdef UI_NO_KEYBOARD
    if( uiState->macro_menu_selected == 7 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( guiState->layout.line3_large_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
                   Color_Yellow_Fab413 , "8" );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
#ifdef SCREEN_BRIGHTNESS
    gfx_print( guiState->layout.line3_large_pos , guiState->layout.top_font , TEXT_ALIGN_CENTER ,
               Color_White ,   "       B+" );
#endif
#ifdef UI_NO_KEYBOARD
    if( uiState->macro_menu_selected == 8 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( guiState->layout.line3_large_pos , guiState->layout.top_font , TEXT_ALIGN_RIGHT ,
                   Color_Yellow_Fab413 , "9        " );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
    if( uiState->input_locked == true )
    {
       gfx_print( guiState->layout.line3_large_pos , guiState->layout.top_font , TEXT_ALIGN_RIGHT ,
                  Color_White , "Unlk" );
    }
    else
    {
       gfx_print( guiState->layout.line3_large_pos , guiState->layout.top_font , TEXT_ALIGN_RIGHT ,
                  Color_White , "Lck" );
    }
    // Draw S-meter bar
    _ui_drawMainBottom( guiState , event );
    return true;
}
