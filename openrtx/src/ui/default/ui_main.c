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

#include <interfaces/platform.h>
#include <interfaces/cps_io.h>
#include <stdio.h>
#include <stdint.h>
#include <ui/ui_default.h>
#include <string.h>
#include <ui/ui_strings.h>

extern bool ui_DisplayPage( GuiState_st* guiState , uiPageNum_en pageNum );
static void ui_drawMainVFO( GuiState_st* guiState , Event_st* event );
static void ui_drawMainVFOInput( GuiState_st* guiState , Event_st* event );
static void ui_drawMainMEM( GuiState_st* guiState , Event_st* event );
static void ui_drawModeVFO( GuiState_st* guiState , Event_st* event );
static void ui_drawModeMEM( GuiState_st* guiState , Event_st* event );
extern void _ui_drawMenuTop( GuiState_st* guiState , Event_st* event );
extern void _ui_drawMenuBank( GuiState_st* guiState , Event_st* event );
extern void _ui_drawMenuChannel( GuiState_st* guiState , Event_st* event );
extern void _ui_drawMenuContacts( GuiState_st* guiState , Event_st* event );
extern void _ui_drawMenuGPS( GuiState_st* guiState , Event_st* event );
extern void _ui_drawMenuSettings( GuiState_st* guiState , Event_st* event );
extern void _ui_drawMenuBackupRestore( GuiState_st* guiState , Event_st* event );
extern void _ui_drawMenuBackup( GuiState_st* guiState , Event_st* event );
extern void _ui_drawMenuRestore( GuiState_st* guiState , Event_st* event );
extern void _ui_drawMenuInfo( GuiState_st* guiState , Event_st* event );
extern void _ui_drawMenuAbout( GuiState_st* guiState , Event_st* event );
extern void _ui_drawSettingsTimeDate( GuiState_st* guiState , Event_st* event );
extern void _ui_drawSettingsTimeDateSet( GuiState_st* guiState , Event_st* event );
extern void _ui_drawSettingsDisplay( GuiState_st* guiState , Event_st* event );
extern void _ui_drawSettingsGPS( GuiState_st* guiState , Event_st* event );
extern void _ui_drawSettingsRadio( GuiState_st* guiState , Event_st* event );
extern void _ui_drawSettingsM17( GuiState_st* guiState , Event_st* event );
extern void _ui_drawSettingsVoicePrompts( GuiState_st* guiState , Event_st* event );
extern void _ui_drawSettingsReset2Defaults( GuiState_st* guiState , Event_st* event );
static void ui_drawLowBatteryScreen( GuiState_st* guiState , Event_st* event );
static void ui_drawAuthors( GuiState_st* guiState , Event_st* event );
static void ui_drawBlank( GuiState_st* guiState , Event_st* event );

//@@@KL static void ui_drawMainBackground( void );
static void ui_drawMainTop( GuiState_st* guiState , Event_st* event );
static void ui_drawBankChannel( GuiState_st* guiState );
static void ui_drawModeInfo( GuiState_st* guiState );
static void ui_drawFrequency( GuiState_st* guiState );
static void ui_drawVFOMiddleInput( GuiState_st* guiState );

void _ui_drawMainBottom( GuiState_st* guiState , Event_st* event );

typedef void (*ui_draw_fn)( GuiState_st* guiState , Event_st* event );

static const ui_draw_fn uiPageDescTable[ PAGE_NUM_OF ] =
{
    ui_drawMainVFO                 , // PAGE_MAIN_VFO
    ui_drawMainVFOInput            , // PAGE_MAIN_VFO_INPUT
    ui_drawMainMEM                 , // PAGE_MAIN_MEM
    ui_drawModeVFO                 , // PAGE_MODE_VFO
    ui_drawModeMEM                 , // PAGE_MODE_MEM
    _ui_drawMenuTop                , // PAGE_MENU_TOP
    _ui_drawMenuBank               , // PAGE_MENU_BANK
    _ui_drawMenuChannel            , // PAGE_MENU_CHANNEL
    _ui_drawMenuContacts           , // PAGE_MENU_CONTACTS
    _ui_drawMenuGPS                , // PAGE_MENU_GPS
    _ui_drawMenuSettings           , // PAGE_MENU_SETTINGS
    _ui_drawMenuBackupRestore      , // PAGE_MENU_BACKUP_RESTORE
    _ui_drawMenuBackup             , // PAGE_MENU_BACKUP
    _ui_drawMenuRestore            , // PAGE_MENU_RESTORE
    _ui_drawMenuInfo               , // PAGE_MENU_INFO
    _ui_drawMenuAbout              , // PAGE_MENU_ABOUT
    _ui_drawSettingsTimeDate       , // PAGE_SETTINGS_TIMEDATE
    _ui_drawSettingsTimeDateSet    , // PAGE_SETTINGS_TIMEDATE_SET
    _ui_drawSettingsDisplay        , // PAGE_SETTINGS_DISPLAY
    _ui_drawSettingsGPS            , // PAGE_SETTINGS_GPS
    _ui_drawSettingsRadio          , // PAGE_SETTINGS_RADIO
    _ui_drawSettingsM17            , // PAGE_SETTINGS_M17
    _ui_drawSettingsVoicePrompts   , // PAGE_SETTINGS_VOICE
    _ui_drawSettingsReset2Defaults , // PAGE_SETTINGS_RESET_TO_DEFAULTS
    ui_drawLowBatteryScreen        , // PAGE_LOW_BAT
    ui_drawAuthors                 , // PAGE_ABOUT
    ui_drawBlank                     // PAGE_BLANK
};

void ui_draw( GuiState_st* guiState , State_st* state , Event_st* event )
{
    (void)event ;

    uiPageNum_en pgNum = state->ui_screen ;

    if( pgNum >= PAGE_NUM_OF )
    {
        pgNum = PAGE_BLANK ;
    }

    if( state->ui_prevScreen != pgNum )
    {
        event->payload       = EVENT_STATUS_ALL ;
        state->ui_prevScreen = pgNum ;
    }

    if( pgNum >= PAGE_NUM_OF )
    {
        pgNum = PAGE_BLANK ;
    }

    // attempt to display the page as a scripted page
    if( !ui_DisplayPage( guiState , pgNum ) )
    {
        // if not successful - display using the legacy fn.
        uiPageDescTable[ pgNum ]( guiState , event ) ;
    }

}

static void ui_drawMainVFO( GuiState_st* guiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();
    ui_drawMainTop( guiState , event );
    ui_drawModeInfo( guiState );

    // Show VFO frequency if the OpMode is not M17 or there is no valid LSF data
    rtxStatus_t status = rtx_getCurrentStatus();
    if( ( status.opMode != OPMODE_M17 ) || ( status.lsfOk == false ) )
    {
        ui_drawFrequency( guiState );
    }

    _ui_drawMainBottom( guiState , event );

}

static void ui_drawMainVFOInput( GuiState_st* guiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();
    ui_drawMainTop( guiState , event );
    ui_drawVFOMiddleInput( guiState );
    _ui_drawMainBottom( guiState , event );

}

void ui_drawMainMEM( GuiState_st* guiState , Event_st* event )
{
    (void)event ;

    gfx_clearScreen();
    ui_drawMainTop( guiState , event );
    ui_drawModeInfo( guiState );

    // Show channel data if the OpMode is not M17 or there is no valid LSF data
    rtxStatus_t status = rtx_getCurrentStatus();
    if( ( status.opMode != OPMODE_M17 ) || ( status.lsfOk == false ) )
    {
        ui_drawBankChannel( guiState );
        ui_drawFrequency( guiState );
    }

    _ui_drawMainBottom( guiState , event );

}

static void ui_drawModeVFO( GuiState_st* guiState , Event_st* event )
{
    (void)guiState ;
    (void)event ;
}

static void ui_drawModeMEM( GuiState_st* guiState , Event_st* event )
{
    (void)guiState ;
    (void)event ;
}

static void ui_drawLowBatteryScreen( GuiState_st* guiState , Event_st* event )
{
    (void)guiState ;
    (void)event ;
    color_t color_fg ;
    uiColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    uint16_t bat_width       = SCREEN_WIDTH / 2 ;
    uint16_t bat_height      = SCREEN_HEIGHT / 3 ;
             point_t bat_pos = { SCREEN_WIDTH / 4 , SCREEN_HEIGHT / 8 };

             gfx_drawBattery( bat_pos , bat_width , bat_height , 10 );

    point_t  text_pos_1      = { 0 , SCREEN_HEIGHT * 2 / 3 };
    point_t  text_pos_2      = { 0 , SCREEN_HEIGHT * 2 / 3 + 16 };

    gfx_print( text_pos_1                       ,
               FONT_SIZE_6PT                    ,
               TEXT_ALIGN_CENTER                ,
               color_fg                      ,
               currentLanguage->forEmergencyUse   );

    gfx_print( text_pos_2                      ,
               FONT_SIZE_6PT                   ,
               TEXT_ALIGN_CENTER               ,
               color_fg                     ,
               currentLanguage->pressAnyButton   );
}

static void ui_drawAuthors( GuiState_st* guiState , Event_st* event )
{
    (void)guiState ;
    (void)event ;
}

static void ui_drawBlank( GuiState_st* guiState , Event_st* event )
{
    (void)guiState ;
    (void)event ;
}
//@@@KL - not being called - remove?
/*
static void ui_drawMainBackground( void )
{
    // Print top bar line of hline_h pixel height
    gfx_drawHLine(guiState->layout.line_top.height, guiState->layout.hline_h, color_gg);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(SCREEN_HEIGHT - guiState->layout.line_bottom.height - 1, guiState->layout.hline_h, color_gg);
}
*/
static void ui_drawMainTop( GuiState_st* guiState , Event_st* event )
{
    color_t color_fg ;
    uiColorLoad( &color_fg , COLOR_FG );

#ifdef RTC_PRESENT
    if( event->payload & EVENT_STATUS_TIME_DISPLAY_TICK )
    {
        // Print clock on top bar
        datetime_t local_time = utcToLocalTime( last_state.time ,
                                                last_state.settings.utc_timezone );
        gfx_print( guiState->layout.line_top.pos , guiState->layout.line_top.font , TEXT_ALIGN_CENTER ,
                   color_fg , "%02d:%02d:%02d" , local_time.hour ,
                   local_time.minute , local_time.second );
    }
#endif // RTC_PRESENT
    if( event->payload & EVENT_STATUS_BATTERY )
    {
        // If the radio has no built-in battery, print input voltage
#ifdef BAT_NONE
        gfx_print( guiState->layout.line_top.pos , guiState->layout.line_top.font , TEXT_ALIGN_RIGHT ,
                   color_fg , "%.1fV" , last_state.v_bat );
#else // BAT_NONE
        // Otherwise print battery icon on top bar, use 4 px padding
        uint16_t bat_width  = SCREEN_WIDTH / 9 ;
        uint16_t bat_height = guiState->layout.line_top.height - ( guiState->layout.status_v_pad * 2 );
        point_t  bat_pos    = { SCREEN_WIDTH - bat_width - guiState->layout.horizontal_pad ,
                                guiState->layout.status_v_pad };
        gfx_drawBattery( bat_pos , bat_width , bat_height , last_state.charge );
#endif // BAT_NONE
    }
    if( guiState->uiState.input_locked == true )
    {
      gfx_drawSymbol( guiState->layout.line_top.pos , guiState->layout.line_top.symbolSize , TEXT_ALIGN_LEFT ,
                      color_fg , SYMBOL_LOCK );
    }
}

static void ui_drawBankChannel( GuiState_st* guiState )
{
    color_t color_fg ;
    uiColorLoad( &color_fg , COLOR_FG );

    // Print Bank number, channel number and Channel name
    uint16_t bank_enabled = ( last_state.bank_enabled ) ? last_state.bank : 0 ;
    gfx_print( guiState->layout.line_1.pos , guiState->layout.line_1.font , TEXT_ALIGN_CENTER ,
               color_fg , "%01d-%03d: %.12s" ,
               bank_enabled , last_state.channel_index + 1 , last_state.channel.name );
}

static void ui_drawModeInfo( GuiState_st* guiState )
{
    char bw_str[ 8 ]     = { 0 };
    char encdec_str[ 9 ] = { 0 };
    color_t color_fg ;
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
                gfx_print( guiState->layout.line_2.pos , guiState->layout.line_2.font , TEXT_ALIGN_CENTER ,
                           color_fg , "%s %4.1f %s" , bw_str ,
                           ctcss_tone[ last_state.channel.fm.txTone ] / 10.0f , encdec_str );
            }
            else
            {
                gfx_print( guiState->layout.line_2.pos , guiState->layout.line_2.font , TEXT_ALIGN_CENTER ,
                           color_fg , "%s" , bw_str );
            }
            break ;
        }
        case OPMODE_DMR :
        {
            // Print Contact
            gfx_print( guiState->layout.line_2.pos , guiState->layout.line_2.font , TEXT_ALIGN_CENTER ,
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
                gfx_drawSymbol( guiState->layout.line_2.pos , guiState->layout.line_2.symbolSize , TEXT_ALIGN_LEFT ,
                                color_fg , SYMBOL_CALL_RECEIVED );

                gfx_print( guiState->layout.line_2.pos , guiState->layout.line_2.font , TEXT_ALIGN_CENTER ,
                           color_fg , "%s" , rtxStatus.M17_dst );

                // Source address
                gfx_drawSymbol( guiState->layout.line_1.pos , guiState->layout.line_1.symbolSize , TEXT_ALIGN_LEFT ,
                                color_fg , SYMBOL_CALL_MADE );

                gfx_print( guiState->layout.line_1.pos , guiState->layout.line_2.font , TEXT_ALIGN_CENTER ,
                           color_fg , "%s" , rtxStatus.M17_src );

                // RF link (if present)
                if( rtxStatus.M17_link[0] != '\0' )
                {
                    gfx_drawSymbol( guiState->layout.line_4.pos , guiState->layout.line_3.symbolSize , TEXT_ALIGN_LEFT ,
                                    color_fg , SYMBOL_ACCESS_POINT );

                    gfx_print( guiState->layout.line_4.pos , guiState->layout.line_2.font , TEXT_ALIGN_CENTER ,
                               color_fg , "%s" , rtxStatus.M17_link );
                }

                // Reflector (if present)
                if( rtxStatus.M17_refl[0] != '\0' )
                {
                    gfx_drawSymbol( guiState->layout.line_3.pos , guiState->layout.line_4.symbolSize , TEXT_ALIGN_LEFT ,
                                    color_fg , SYMBOL_NETWORK );

                    gfx_print( guiState->layout.line_3.pos , guiState->layout.line_2.font , TEXT_ALIGN_CENTER ,
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

                gfx_print( guiState->layout.line_2.pos , guiState->layout.line_2.font , TEXT_ALIGN_CENTER ,
                           color_fg , "M17 #%s" , dst );
            }
            break ;
        }
    }
}

static void ui_drawFrequency( GuiState_st* guiState )
{
    unsigned long frequency = platform_getPttStatus() ? last_state.channel.tx_frequency
                                                      : last_state.channel.rx_frequency;
    color_t color_fg ;
    uiColorLoad( &color_fg , COLOR_FG );

    // Print big numbers frequency
    gfx_print( guiState->layout.line_3_large.pos , guiState->layout.line_3_large.font , TEXT_ALIGN_CENTER ,
               color_fg , "%.7g" , (float)frequency / 1000000.0f );
}

static void ui_drawVFOMiddleInput( GuiState_st* guiState )
{
    // Add inserted number to string, skipping "Rx: "/"Tx: " and "."
    uint8_t insert_pos = guiState->uiState.input_position + 3;
    color_t color_fg ;
    uiColorLoad( &color_fg , COLOR_FG );

    if( guiState->uiState.input_position > 3 )
    {
        insert_pos += 1 ;
    }
    char input_char = guiState->uiState.input_number + '0' ;

    if( guiState->uiState.input_set == SET_RX )
    {
        if( guiState->uiState.input_position == 0 )
        {
            gfx_print( guiState->layout.line_2.pos , guiState->layout.input_font , TEXT_ALIGN_CENTER ,
                       color_fg , ">Rx:%03lu.%04lu" ,
                       (unsigned long)guiState->uiState.new_rx_frequency / 1000000 ,
                       (unsigned long)( guiState->uiState.new_rx_frequency % 1000000 ) / 100 );
        }
        else
        {
            // Replace Rx frequency with underscorses
            if( guiState->uiState.input_position == 1 )
            {
                strcpy( guiState->uiState.new_rx_freq_buf , ">Rx:___.____" );
            }
            guiState->uiState.new_rx_freq_buf[ insert_pos ] = input_char ;
            gfx_print( guiState->layout.line_2.pos , guiState->layout.input_font , TEXT_ALIGN_CENTER ,
                       color_fg , guiState->uiState.new_rx_freq_buf );
        }
        gfx_print( guiState->layout.line_3_large.pos , guiState->layout.input_font , TEXT_ALIGN_CENTER ,
                   color_fg , " Tx:%03lu.%04lu" ,
                   (unsigned long)last_state.channel.tx_frequency / 1000000 ,
                   (unsigned long)( last_state.channel.tx_frequency % 1000000 ) / 100 );
    }
    else
    {
        if( guiState->uiState.input_set == SET_TX )
        {
            gfx_print( guiState->layout.line_2.pos , guiState->layout.input_font , TEXT_ALIGN_CENTER ,
                       color_fg , " Rx:%03lu.%04lu" ,
                       (unsigned long)guiState->uiState.new_rx_frequency / 1000000 ,
                       (unsigned long)( guiState->uiState.new_rx_frequency % 1000000 ) / 100 );
            // Replace Rx frequency with underscorses
            if( guiState->uiState.input_position == 0 )
            {
                gfx_print( guiState->layout.line_3_large.pos , guiState->layout.input_font , TEXT_ALIGN_CENTER ,
                           color_fg , ">Tx:%03lu.%04lu" ,
                           (unsigned long)guiState->uiState.new_rx_frequency / 1000000 ,
                           (unsigned long)( guiState->uiState.new_rx_frequency % 1000000 ) / 100 );
            }
            else
            {
                if( guiState->uiState.input_position == 1 )
                {
                    strcpy( guiState->uiState.new_tx_freq_buf , ">Tx:___.____" );
                }
                guiState->uiState.new_tx_freq_buf[ insert_pos ] = input_char ;
                gfx_print( guiState->layout.line_3_large.pos , guiState->layout.input_font , TEXT_ALIGN_CENTER ,
                           color_fg , guiState->uiState.new_tx_freq_buf );
            }
        }
    }
}

void _ui_drawMainBottom( GuiState_st* guiState , Event_st* event )
{
    // Squelch bar
    float rssi            = last_state.rssi ;
    float squelch         = last_state.settings.sqlLevel / 16.0f ;
    uint16_t meter_width  = SCREEN_WIDTH - ( 2 * guiState->layout.horizontal_pad ) ;
    uint16_t meter_height = guiState->layout.line_bottom.height ;
    point_t meter_pos     = { guiState->layout.horizontal_pad , ( SCREEN_HEIGHT - meter_height ) - guiState->layout.bottom_pad };
    uint8_t mic_level     = platform_getMicLevel();
    color_t color_op3 ;
    uiColorLoad( &color_op3 , COLOR_OP3 );

    if( event->payload & EVENT_STATUS_RSSI )
    {
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
}
