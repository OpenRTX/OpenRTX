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

#include "ui.h"
#include "ui_value_arrays.h"
#include "ui_scripts.h"
#include "ui_commands.h"
#include "ui_value_display.h"
#include "ui_states.h"
#include "ui_value_input.h"

static void GuiVal_DispVal( GuiState_st* guiState , char* valueBuffer );

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

typedef void (*ui_GuiVal_fn)( GuiState_st* guiState );

static const ui_GuiVal_fn ui_GuiVal_Table[ GUI_VAL_DSP_NUM_OF ] =
{
    GuiVal_Stubbed          , // GUI_VAL_INP_VFO_MIDDLE_INPUT

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

void GuiVal_DisplayValue( GuiState_st* guiState , uint8_t valueNum )
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
    ui_ColorLoad( &color_bg , COLOR_BG );
    ui_ColorLoad( &color_fg , COLOR_FG );

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
    ui_ColorLoad( &color_bg , COLOR_BG );
    ui_ColorLoad( &color_fg , COLOR_FG );

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
    ui_ColorLoad( &color_bg , COLOR_BG );
    ui_ColorLoad( &color_fg , COLOR_FG );

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
    ui_ColorLoad( &color_fg , COLOR_FG );

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
        ui_ColorLoad( &color_fg , COLOR_FG );

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

    ui_ColorLoad( &color_op3 , COLOR_OP3 );

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
//    GuiVal_DispVal( guiState , valueBuffer );
}

static void GuiVal_Channels( GuiState_st* guiState )
{
    (void)guiState ;

//    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

//            snprintf( valueBuffer , valueBufferSize , "%s" ,
//                      ... );
//    GuiVal_DispVal( guiState , valueBuffer );
}

static void GuiVal_Contacts( GuiState_st* guiState )
{
    (void)guiState ;

//    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

//            snprintf( valueBuffer , valueBufferSize , "%s" ,
//                      ... );
//    GuiVal_DispVal( guiState , valueBuffer );
}

static void GuiVal_Gps( GuiState_st* guiState )
{
    (void)guiState ;

//    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

//            snprintf( valueBuffer , valueBufferSize , "%s" ,
//                      ... );
//    GuiVal_DispVal( guiState , valueBuffer );
}
// Settings
// Display

#ifdef SCREEN_BRIGHTNESS
static void GuiVal_ScreenBrightness( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , last_state.settings.brightness );
    GuiVal_DispVal( guiState , valueBuffer );

}
#endif
#ifdef SCREEN_CONTRAST
static void GuiVal_ScreenContrast( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , last_state.settings.contrast );
    GuiVal_DispVal( guiState , valueBuffer );

}
#endif
static void GuiVal_Timer( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              display_timer_values[ last_state.settings.display_timer ] );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_Date( GuiState_st* guiState )
{
    char       valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    datetime_t local_time = utcToLocalTime( last_state.time , last_state.settings.utc_timezone );

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%02d/%02d/%02d" ,
              local_time.date , local_time.month , local_time.year );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_Time( GuiState_st* guiState )
{
    char       valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    datetime_t local_time = utcToLocalTime( last_state.time , last_state.settings.utc_timezone );

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%02d:%02d:%02d" ,
              local_time.hour , local_time.minute , local_time.second );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_GpsEnables( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              (last_state.settings.gps_enabled) ? currentLanguage->on : currentLanguage->off );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_GpsSetTime( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              (last_state.gps_set_time) ? currentLanguage->on : currentLanguage->off );
    GuiVal_DispVal( guiState , valueBuffer );

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
    GuiVal_DispVal( guiState , valueBuffer );

}

    // Radio
static void GuiVal_RadioOffset( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    int32_t offset = 0 ;

    offset = abs( (int32_t)last_state.channel.tx_frequency -
                  (int32_t)last_state.channel.rx_frequency );
    snprintf( valueBuffer , MAX_ENTRY_LEN , "%gMHz" , (float)offset / 1000000.0f );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_RadioDirection( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    valueBuffer[ 0 ] = ( last_state.channel.tx_frequency >= last_state.channel.rx_frequency ) ? '+' : '-';
    valueBuffer[ 1 ] = '\0';
    GuiVal_DispVal( guiState , valueBuffer );

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
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_M17Callsign( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" , last_state.settings.callsign );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_M17Can( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , last_state.settings.m17_can );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_M17CanRxCheck( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              (last_state.settings.m17_can_rx) ? currentLanguage->on : currentLanguage->off );
    GuiVal_DispVal( guiState , valueBuffer );

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
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_Phonetic( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              last_state.settings.vpPhoneticSpell ? currentLanguage->on : currentLanguage->off );
    GuiVal_DispVal( guiState , valueBuffer );

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
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_BatteryCharge( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d%%" , last_state.charge );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_Rssi( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%.1fdBm" , last_state.rssi );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_UsedHeap( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%dB" , getHeapSize() - getCurrentFreeHeap() );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_Band( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s %s" , hwinfo->vhf_band ? currentLanguage->VHF : "" , hwinfo->uhf_band ? currentLanguage->UHF : "" );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_Vhf( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d - %d" , hwinfo->vhf_minFreq, hwinfo->vhf_maxFreq );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_Uhf( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d - %d" , hwinfo->uhf_minFreq, hwinfo->uhf_maxFreq );
    GuiVal_DispVal( guiState , valueBuffer );

}

static void GuiVal_HwVersion( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();
    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , hwinfo->hw_version );

    GuiVal_DispVal( guiState , valueBuffer );

}

#ifdef PLATFORM_TTWRPLUS
static void GuiVal_Radio( GuiState_st* guiState )
{
    (void)guiState ;
    //@@@KL Populate
//    GuiVal_DispVal( guiState , valueBuffer );
}

static void GuiVal_RadioFw( GuiState_st* guiState )
{
    (void)guiState ;
    //@@@KL Populate
//    GuiVal_DispVal( guiState , valueBuffer );
}
#endif // PLATFORM_TTWRPLUS

// Default
static void GuiVal_Stubbed( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] ;

    valueBuffer[ 0 ] = '?' ;
    valueBuffer[ 1 ] = '\0' ;
    GuiVal_DispVal( guiState , valueBuffer );

}

void GuiVal_DispVal( GuiState_st* guiState , char* valueBuffer )
{
    Color_st color_fg ;
    Color_st color_bg ;
    Color_st color_text ;

    ui_ColorLoad( &color_fg , COLOR_FG );
    ui_ColorLoad( &color_bg , COLOR_BG );
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
