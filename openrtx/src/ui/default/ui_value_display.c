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
#include <ui.h>
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

//@@@KL Note - in order to update the display the state
// needs to be propagated through to the state
// hence last_ state has been replaced with state in this file
// however where values change constantly - eg time -
//  they will have to be handled statically
// a lot of these values are not state variables so will have to be rationalised

#include "ui.h"
#include "ui_value_arrays.h"
#include "ui_scripts.h"
#include "ui_commands.h"
#include "ui_value_display.h"
#include "ui_list_display.h"
#include "ui_states.h"
#include "ui_value_input.h"

static void GuiValDsp_Disp_Val( GuiState_st* guiState , char* valueBuffer );
static void GuiValDsp_Disp_Bat( GuiState_st* guiState , uint16_t percentage );
static void GuiValDsp_Disp_StoreVarPos( GuiState_st* guiState );
static void GuiValDsp_Disp_ClearVarPos( GuiState_st* guiState );

static void GuiValDsp_BatteryLevel( GuiState_st* guiState );
static void GuiValDsp_LockState( GuiState_st* guiState );
static void GuiValDsp_ModeInfo( GuiState_st* guiState );
static void GuiValDsp_BankChannel( GuiState_st* guiState );
static void GuiValDsp_Frequency( GuiState_st* guiState );
static void GuiValDsp_RSSIMeter( GuiState_st* guiState );
#ifdef GPS_PRESENT
static void GuiValDsp_GPS( GuiState_st* guiState );
#endif // GPS_PRESENT
// Settings
// Display
#ifdef SCREEN_BRIGHTNESS
static void GuiValDsp_ScreenBrightness( GuiState_st* guiState );
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
static void GuiValDsp_ScreenContrast( GuiState_st* guiState );
#endif // SCREEN_CONTRAST
static void GuiValDsp_Timer( GuiState_st* guiState );
// Time and Date
static void GuiValDsp_Date( GuiState_st* guiState );
static void GuiValDsp_Time( GuiState_st* guiState );
// GPS
#ifdef GPS_PRESENT
static void GuiValDsp_GPSEnables( GuiState_st* guiState );
static void GuiValDsp_GPSTime( GuiState_st* guiState );
static void GuiValDsp_GPSTimeZone( GuiState_st* guiState );
#endif // GPS_PRESENT
// Radio
static void GuiValDsp_RadioOffset( GuiState_st* guiState );
static void GuiValDsp_RadioDirection( GuiState_st* guiState );
static void GuiValDsp_RadioStep( GuiState_st* guiState );
// M17
static void GuiValDsp_M17Callsign( GuiState_st* guiState );
static void GuiValDsp_M17Can( GuiState_st* guiState );
static void GuiValDsp_M17CanRxCheck( GuiState_st* guiState );
// Accessibility - Voice
static void GuiValDsp_Voice( GuiState_st* guiState );
static void GuiValDsp_Phonetic( GuiState_st* guiState );
// Info
static void GuiValDsp_BatteryVoltage( GuiState_st* guiState );
static void GuiValDsp_BatteryCharge( GuiState_st* guiState );
static void GuiValDsp_Rssi( GuiState_st* guiState );
static void GuiValDsp_UsedHeap( GuiState_st* guiState );
static void GuiValDsp_Band( GuiState_st* guiState );
static void GuiValDsp_Vhf( GuiState_st* guiState );
static void GuiValDsp_Uhf( GuiState_st* guiState );
static void GuiValDsp_HwVersion( GuiState_st* guiState );
#ifdef PLATFORM_TTWRPLUS
static void GuiValDsp_Radio( GuiState_st* guiState );
static void GuiValDsp_RadioFw( GuiState_st* guiState );
#endif // PLATFORM_TTWRPLUS
static void GuiValDsp_BackupRestore( GuiState_st* guiState );
static void GuiValDsp_LowBattery( GuiState_st* guiState );
#ifdef ENABLE_DEBUG_MSG
  #ifndef DISPLAY_DEBUG_MSG
static void GuiValDsp_DebugCh( GuiState_st* guiState );
static void GuiValDsp_DebugGfx( GuiState_st* guiState );
  #else // DISPLAY_DEBUG_MSG
enum
{
    DEBUG_MSG_ALLOC = 10
};
static char    DebugMsg[ DEBUG_MSG_ALLOC + 1 ] = "\0" ;
static uint8_t DebugVal0 = 0 ;
static uint8_t DebugVal1 = 0 ;
static uint8_t DebugVal2 = 0 ;
static uint8_t DebugVal3 = 0 ;
static uint8_t DebugVal4 = 0 ;
static uint8_t DebugVal5 = 0 ;
static uint8_t DebugCnt  = 0 ;
static void GuiValDsp_DebugMsg( GuiState_st* guiState );
static void GuiValDsp_DebugValues( GuiState_st* guiState );
  #endif // DISPLAY_DEBUG_MSG
#endif // ENABLE_DEBUG_MSG
static void GuiValDsp_Stubbed( GuiState_st* guiState );

static bool GuiValDsp_Disp_HasValueChanged( GuiState_st* guiState , uint32_t value );

typedef void (*ui_GuiValDsp_fn)( GuiState_st* guiState );

static const ui_GuiValDsp_fn ui_GuiValDsp_Table[ GUI_VAL_NUM_OF ] =
{
    GuiValDsp_BatteryLevel     ,   // GUI_VAL_BATTERY_LEVEL    0x00
    GuiValDsp_LockState        ,   // GUI_VAL_LOCK_STATE       0x01
    GuiValDsp_ModeInfo         ,   // GUI_VAL_MODE_INFO        0x02
    GuiValDsp_BankChannel      ,   // GUI_VAL_BANK_CHANNEL     0x03
    GuiValDsp_Frequency        ,   // GUI_VAL_FREQUENCY        0x04
    GuiValDsp_RSSIMeter        ,   // GUI_VAL_RSSI_METER       0x05
#ifdef GPS_PRESENT
    GuiValDsp_GPS              ,   // GUI_VAL_GPS              0x06
#endif // GPS_PRESENT
    // Settings
    // Display
#ifdef SCREEN_BRIGHTNESS
    GuiValDsp_ScreenBrightness ,   // GUI_VAL_BRIGHTNESS       0x07
#endif
#ifdef SCREEN_CONTRAST
    GuiValDsp_ScreenContrast   ,   // GUI_VAL_CONTRAST         0x08
#endif
    GuiValDsp_Timer            ,   // GUI_VAL_TIMER            0x09
    // Time and Date
    GuiValDsp_Date             ,   // GUI_VAL_DATE             0x0A
    GuiValDsp_Time             ,   // GUI_VAL_TIME             0x0B
    // GPS
#ifdef GPS_PRESENT
    GuiValDsp_GPSEnables       ,   // GUI_VAL_GPS_ENABLED      0x0C
    GuiValDsp_GPSTime          ,   // GUI_VAL_GPS_TIME         0x0D
    GuiValDsp_GPSTimeZone      ,   // GUI_VAL_GPS_TIME_ZONE    0x0E
#endif // GPS_PRESENT
    // Radio
    GuiValDsp_RadioOffset      ,   // GUI_VAL_RADIO_OFFSET     0x0F
    GuiValDsp_RadioDirection   ,   // GUI_VAL_RADIO_DIRECTION  0x10
    GuiValDsp_RadioStep        ,   // GUI_VAL_RADIO_STEP       0x11
    // M17
    GuiValDsp_M17Callsign      ,   // GUI_VAL_M17_CALLSIGN     0x12
    GuiValDsp_M17Can           ,   // GUI_VAL_M17_CAN          0x13
    GuiValDsp_M17CanRxCheck    ,   // GUI_VAL_M17_CAN_RX_CHECK 0x14
    // Accessibility - Voice
    GuiValDsp_Voice            ,   // GUI_VAL_LEVEL            0x15
    GuiValDsp_Phonetic         ,   // GUI_VAL_PHONETIC         0x16
    // Info
    GuiValDsp_BatteryVoltage   ,   // GUI_VAL_BATTERY_VOLTAGE  0x17
    GuiValDsp_BatteryCharge    ,   // GUI_VAL_BATTERY_CHARGE   0x18
    GuiValDsp_Rssi             ,   // GUI_VAL_RSSI             0x19
    GuiValDsp_UsedHeap         ,   // GUI_VAL_USED_HEAP        0x1A
    GuiValDsp_Band             ,   // GUI_VAL_BAND             0x1B
    GuiValDsp_Vhf              ,   // GUI_VAL_VHF              0x1C
    GuiValDsp_Uhf              ,   // GUI_VAL_UHF              0x1D
    GuiValDsp_HwVersion        ,   // GUI_VAL_HW_VERSION       0x1E
#ifdef PLATFORM_TTWRPLUS
    GuiValDsp_Radio            ,   // GUI_VAL_RADIO            0x1F
    GuiValDsp_RadioFw          ,   // GUI_VAL_RADIO_FW         0x20
#endif // PLATFORM_TTWRPLUS
    GuiValDsp_BackupRestore    ,   // GUI_VAL_BACKUP_RESTORE   0x21
    GuiValDsp_LowBattery       ,   // GUI_VAL_LOW_BATTERY      0x22
#ifdef ENABLE_DEBUG_MSG
  #ifndef DISPLAY_DEBUG_MSG
    GuiValDsp_DebugCh          ,   // GUI_VAL_DEBUG_CH         0x23
    GuiValDsp_DebugGfx         ,   // GUI_VAL_DEBUG_GFX        0x24
  #else // DISPLAY_DEBUG_MSG
    GuiValDsp_DebugMsg         ,   // GUI_VAL_DEBUG_MSG        0x25
    GuiValDsp_DebugValues      ,   // GUI_VAL_DEBUG_VALUES     0x26
  #endif // DISPLAY_DEBUG_MSG
#endif // ENABLE_DEBUG_MSG
    GuiValDsp_Stubbed              // GUI_VAL_STUBBED          0x27
};

void GuiValDsp_DisplayValue( GuiState_st* guiState )
{
    uint8_t varNum = guiState->layout.vars[ guiState->layout.varIndex ].varNum ;

    guiState->layout.itemPos.y  = 0 ;
    guiState->layout.itemPos.x  = 0 ;
    guiState->layout.itemPos.h  = 0 ;
    guiState->layout.itemPos.w  = 0 ;
    ui_GuiValDsp_Table[ varNum ]( guiState );
    guiState->layout.line.pos.x = guiState->layout.itemPos.x +
                                  guiState->layout.itemPos.w   ;

}

static void GuiValDsp_BatteryLevel( GuiState_st* guiState )
{
    GuiValDsp_Disp_Bat( guiState , state.charge );
}

static void GuiValDsp_LockState( GuiState_st* guiState )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Pos_st    start ;
    uint16_t  width ;
    Color_st  color_bg ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_bg , COLOR_BG );
    ui_ColorLoad( &color_fg , COLOR_FG );

    if( guiState->uiState.input_locked == true )
    {
        start = lineTop->pos ;
        width = styleTop->symbolSize + FONT_SIZE_24PT + 1 ;
        gfx_drawSymbol( &start , width , GFX_ALIGN_LEFT ,
                        &color_fg , SYMBOL_LOCK );
    }

}

static void GuiValDsp_ModeInfo( GuiState_st* guiState )
{
    Pos_st    pos ;
    Line_st*  line1           = &guiState->layout.lines[ GUI_LINE_1 ] ;
    Style_st* style1          = &guiState->layout.styles[ GUI_STYLE_1 ] ;
    Line_st*  line2           = &guiState->layout.lines[ GUI_LINE_2 ] ;
    Style_st* style2          = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Line_st*  line3           = &guiState->layout.lines[ GUI_LINE_3 ] ;
    Style_st* style3          = &guiState->layout.styles[ GUI_STYLE_3 ] ;
    Line_st*  line4           = &guiState->layout.lines[ GUI_LINE_4 ] ;
    Style_st* style4          = &guiState->layout.styles[ GUI_STYLE_4 ] ;
    char      bw_str[ 8 ]     = { 0 };
    char      encdec_str[ 9 ] = { 0 };
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    switch( state.channel.mode )
    {
        case OPMODE_FM :
        {
            // Get Bandwidth string
            if( state.channel.bandwidth == BW_12_5 )
            {
                snprintf( bw_str , 8 , "NFM" );
            }
            else
            {
                if( state.channel.bandwidth == BW_20 )
                {
                    snprintf( bw_str , 8 , "FM20" );
                }
                else
                {
                    if( state.channel.bandwidth == BW_25 )
                    {
                        snprintf( bw_str , 8 , "FM" );
                    }
                }
            }

            // Get encdec string
            bool tone_tx_enable = state.channel.fm.txToneEn ;
            bool tone_rx_enable = state.channel.fm.rxToneEn ;

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
            pos   = line2->pos ;
            pos.y = line2->textY ;
            if( tone_tx_enable || tone_rx_enable )
            {
                gfx_print( &pos , style2->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "%s %4.1f %s" , bw_str ,
                           ctcss_tone[ state.channel.fm.txTone ] / 10.0f , encdec_str );
            }
            else
            {
                gfx_print( &pos , style2->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "%s" , bw_str );
            }
            break ;
        }
        case OPMODE_DMR :
        {
            // Print Contact
            pos   = line2->pos ;
            pos.y = line2->textY ;
            gfx_print( &pos , style2->font.size , GFX_ALIGN_CENTER ,
                       &color_fg , "%s" , state.contact.name );
            break ;
        }
        case OPMODE_M17 :
        {
            // Print M17 Destination ID on line 3 of 3
            rtxStatus_t rtxStatus = rtx_getCurrentStatus();

            if( rtxStatus.lsfOk )
            {
                // Destination address
                pos   = line2->pos ;
                pos.y = line2->textY ;
                gfx_drawSymbol( &pos , style2->symbolSize , GFX_ALIGN_LEFT ,
                                &color_fg , SYMBOL_CALL_RECEIVED );

                gfx_print( &pos , style2->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "%s" , rtxStatus.M17_dst );

                // Source address
                pos   = line1->pos ;
                pos.y = line1->textY ;
                gfx_drawSymbol( &pos , style1->symbolSize , GFX_ALIGN_LEFT ,
                                &color_fg , SYMBOL_CALL_MADE );

                gfx_print( &pos , style2->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "%s" , rtxStatus.M17_src );

                // RF link (if present)
                if( rtxStatus.M17_link[0] != '\0' )
                {
                    pos   = line4->pos ;
                    pos.y = line4->textY ;
                    gfx_drawSymbol( &pos , style3->symbolSize , GFX_ALIGN_LEFT ,
                                    &color_fg , SYMBOL_ACCESS_POINT );
                    gfx_print( &pos , style2->font.size , GFX_ALIGN_CENTER ,
                               &color_fg , "%s" , rtxStatus.M17_link );
                }

                // Reflector (if present)
                if( rtxStatus.M17_refl[0] != '\0' )
                {
                    pos   = line3->pos ;
                    pos.y = line3->textY ;
                    gfx_drawSymbol( &pos , style4->symbolSize , GFX_ALIGN_LEFT ,
                                    &color_fg , SYMBOL_NETWORK );
                    gfx_print( &line3->pos , style2->font.size , GFX_ALIGN_CENTER ,
                               &color_fg , "%s" , rtxStatus.M17_refl );
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

                pos   = line2->pos ;
                pos.y = line2->textY ;
                gfx_print( &pos , style2->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "M17 #%s" , dst );
            }
            break ;
        }
    }

}

static void GuiValDsp_BankChannel( GuiState_st* guiState )
{
    Pos_st    pos ;
    Line_st*  line1       = &guiState->layout.lines[ GUI_LINE_1 ] ;
    Style_st* style1      = &guiState->layout.styles[ GUI_STYLE_1 ] ;
    uint16_t bank_enabled = ( state.bank_enabled ) ? state.bank : 0 ;
    Color_st color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    // Print Bank number, channel number and Channel name
    pos   = line1->pos ;
    pos.y = line1->textY ;
    gfx_print( &pos ,
               style1->font.size , GFX_ALIGN_CENTER ,
               &color_fg , "%01d-%03d: %.12s" ,
               bank_enabled , state.channel_index + 1 , state.channel.name );
}

static void GuiValDsp_Frequency( GuiState_st* guiState )
{
    Pos_st    pos ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;

    // Show VFO frequency if the OpMode is not M17 or there is no valid LSF data
    rtxStatus_t status = rtx_getCurrentStatus();

    if( ( status.opMode != OPMODE_M17 ) || ( status.lsfOk == false ) )
    {
        unsigned long frequency = platform_getPttStatus() ? state.channel.tx_frequency
                                                          : state.channel.rx_frequency;
        Color_st color_fg ;
        ui_ColorLoad( &color_fg , COLOR_FG );

        // Print big numbers frequency
        pos   = line3Large->pos ;
        pos.y = line3Large->textY ;
        gfx_print( &pos , style3Large->font.size , GFX_ALIGN_CENTER ,
                   &color_fg , "%.7g" , (float)frequency / 1000000.0f );
    }

}

static void GuiValDsp_RSSIMeter( GuiState_st* guiState )
{
    Line_st*  lineBottom   = &guiState->layout.lines[ GUI_LINE_BOTTOM ] ;
//    Style_st* styleBottom  = &guiState->layout.styles[ GUI_STYLE_BOTTOM ] ;
    float     rssi         = state.rssi ;
    float     squelch      = state.settings.sqlLevel / 16.0f ; // squelch bar
    Pos_st    meter_pos ;
    uint8_t   mic_level    = platform_getMicLevel();
    Color_st  color_op3 ;

    meter_pos.w = SCREEN_WIDTH - ( 2 * guiState->layout.horizontal_pad ) ;
    meter_pos.h = lineBottom->height ;
    meter_pos.x = guiState->layout.horizontal_pad ;
    meter_pos.y = ( SCREEN_HEIGHT - meter_pos.h ) - guiState->layout.bottom_pad ;

    ui_ColorLoad( &color_op3 , COLOR_OP3 );

    switch( state.channel.mode )
    {
        case OPMODE_FM :
        {
            gfx_drawSmeter( &meter_pos , rssi , squelch , &color_op3 );
            break ;
        }
        case OPMODE_DMR :
        {
            gfx_drawSmeter( &meter_pos , rssi , squelch , &color_op3 );
            break ;
        }
        case OPMODE_M17 :
        {
            gfx_drawSmeterLevel( &meter_pos , rssi , mic_level );
            break ;
        }
    }

}

#ifdef GPS_PRESENT
static void GuiValDsp_GPS( GuiState_st* guiState )
{
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line1       = &guiState->layout.lines[ GUI_LINE_1 ] ;
    Line_st*  line2       = &guiState->layout.lines[ GUI_LINE_2 ] ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    Line_st*  lineBottom  = &guiState->layout.lines[ GUI_LINE_BOTTOM ] ;
    Style_st* styleBottom = &guiState->layout.styles[ GUI_STYLE_BOTTOM ] ;
    char*     fix_buf ;
    char*     type_buf ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    Pos_st fix_pos = { line2->pos.x , ( SCREEN_HEIGHT * 2 ) / 5 , 0 , 0 };
    // Print GPS status, if no fix, hide details
    if( !state.settings.gps_enabled )
    {
        gfx_print( &fix_pos , style3Large->font.size , GFX_ALIGN_CENTER ,
                   &color_fg , currentLanguage->gpsOff );
    }
    else
    {
        if( state.gps_data.fix_quality == 0 )
        {
            gfx_print( &fix_pos , style3Large->font.size , GFX_ALIGN_CENTER ,
                       &color_fg , currentLanguage->noFix );
        }
        else
        {
            if( state.gps_data.fix_quality == 6 )
            {
                gfx_print( &fix_pos , style3Large->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , currentLanguage->fixLost );
            }
            else
            {
                switch( state.gps_data.fix_quality )
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

                switch( state.gps_data.fix_type )
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
                gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
                           &color_fg , fix_buf );
                gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "N     " );
                gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_RIGHT ,
                           &color_fg , "%8.6f" , state.gps_data.latitude );
                gfx_print( &line2->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
                           &color_fg , type_buf );
                // Convert from signed longitude, to unsigned + direction
                float longitude = state.gps_data.longitude ;
                char* direction = (longitude < 0) ? "W     " : "E     " ;
                longitude = (longitude < 0) ? -longitude : longitude ;
                gfx_print( &line2->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , direction );
                gfx_print( &line2->pos , styleTop->font.size , GFX_ALIGN_RIGHT ,
                           &color_fg , "%8.6f" , longitude );
                gfx_print( &lineBottom->pos , styleBottom->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "S %4.1fkm/h  A %4.1fm" ,
                           state.gps_data.speed , state.gps_data.altitude );
            }
        }
    }
    // Draw compass
    Pos_st compass_pos = { guiState->layout.horizontal_pad * 2 , SCREEN_HEIGHT / 2 , 0 , 0 };
    gfx_drawGPScompass( &compass_pos , SCREEN_WIDTH / 9 + 2 ,
                        state.gps_data.tmg_true ,
                        ( state.gps_data.fix_quality != 0 ) &&
                        ( state.gps_data.fix_quality != 6 )    );
    // Draw satellites bar graph
    Pos_st bar_pos = { line3Large->pos.x + SCREEN_WIDTH * 1 / 3 ,
                       SCREEN_HEIGHT / 2 ,
                       ( ( SCREEN_WIDTH * 2 ) / 3 ) - guiState->layout.horizontal_pad ,
                       SCREEN_HEIGHT / 3 };
    gfx_drawGPSgraph( &bar_pos ,
                      state.gps_data.satellites ,
                      state.gps_data.active_sats );

}
#endif // GPS_PRESENT

#ifdef SCREEN_BRIGHTNESS
static void GuiValDsp_ScreenBrightness( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    if( !guiState->update )
    {
        guiState->edit.settings.brightness = state.settings.brightness ;
    }

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , guiState->edit.settings.brightness );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
static void GuiValDsp_ScreenContrast( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    if( !guiState->update )
    {
        guiState->edit.settings.contrast = state.settings.contrast ;
    }

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , guiState->edit.settings.contrast );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}
#endif
static void GuiValDsp_Timer( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    if( !guiState->update )
    {
        guiState->edit.settings.display_timer = state.settings.display_timer ;
    }

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              display_timer_values[ guiState->edit.settings.display_timer ] );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_Date( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    //@@@KL ensure that the display is updated when the date is set
    if( !guiState->layout.varInputDisplay )
    {
        guiState->edit.localTime = utcToLocalTime( state.time ,
                                                   state.settings.utc_timezone );
        snprintf( valueBuffer , MAX_ENTRY_LEN , "%02d/%02d/%02d" ,
                  guiState->edit.localTime.date  ,
                  guiState->edit.localTime.month ,
                  guiState->edit.localTime.year    );
    }
    else
    {
        if( !guiState->update )
        {
            guiState->edit.localTime = utcToLocalTime( state.time ,
                                                       state.settings.utc_timezone );
        }
        switch( guiState->layout.varInputSelect )
        {
            case VAR_INPUT_SELECT_0 :
            {
                snprintf( valueBuffer , MAX_ENTRY_LEN , "[%02d]/%02d/%02d" ,
                          guiState->edit.localTime.date  ,
                          guiState->edit.localTime.month ,
                          guiState->edit.localTime.year    );
                break ;
            }
            case VAR_INPUT_SELECT_1 :
            {
                snprintf( valueBuffer , MAX_ENTRY_LEN , "%02d/[%02d]/%02d" ,
                          guiState->edit.localTime.date  ,
                          guiState->edit.localTime.month ,
                          guiState->edit.localTime.year    );
                break ;
            }
            case VAR_INPUT_SELECT_2 :
            {
                snprintf( valueBuffer , MAX_ENTRY_LEN , "%02d/%02d/[%02d]" ,
                          guiState->edit.localTime.date  ,
                          guiState->edit.localTime.month ,
                          guiState->edit.localTime.year    );
                break ;
            }
        }
    }

    GuiValDsp_Disp_Val( guiState , valueBuffer );
}

static void GuiValDsp_Time( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    //@@@KL ensure that the display is updated when the time is set
    if( !guiState->layout.varInputDisplay )
    {
        guiState->edit.localTime = utcToLocalTime( state.time ,
                                                   state.settings.utc_timezone );
        snprintf( valueBuffer , MAX_ENTRY_LEN , "%02d:%02d:%02d" ,
                  guiState->edit.localTime.hour   ,
                  guiState->edit.localTime.minute ,
                  guiState->edit.localTime.second   );
    }
    else
    {
        if( !guiState->update )
        {
            guiState->edit.localTime = utcToLocalTime( state.time ,
                                                       state.settings.utc_timezone );
        }
        switch( guiState->layout.varInputSelect )
        {
            case VAR_INPUT_SELECT_0 :
            {
                snprintf( valueBuffer , MAX_ENTRY_LEN , "[%02d]:%02d:%02d" ,
                          guiState->edit.localTime.hour   ,
                          guiState->edit.localTime.minute ,
                          guiState->edit.localTime.second   );
                break ;
            }
            case VAR_INPUT_SELECT_1 :
            {
                snprintf( valueBuffer , MAX_ENTRY_LEN , "%02d:[%02d]:%02d" ,
                          guiState->edit.localTime.hour   ,
                          guiState->edit.localTime.minute ,
                          guiState->edit.localTime.second   );
                break ;
            }
            case VAR_INPUT_SELECT_2 :
            {
                snprintf( valueBuffer , MAX_ENTRY_LEN , "%02d:%02d:[%02d]" ,
                          guiState->edit.localTime.hour   ,
                          guiState->edit.localTime.minute ,
                          guiState->edit.localTime.second   );
                break ;
            }
        }
    }

    GuiValDsp_Disp_Val( guiState , valueBuffer );
}

static void GuiValDsp_GPSEnables( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    if( !guiState->update )
    {
        guiState->edit.settings.gps_enabled = state.settings.gps_enabled ;
    }

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              (guiState->edit.settings.gps_enabled) ? currentLanguage->on : currentLanguage->off );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

#ifdef GPS_PRESENT
static void GuiValDsp_GPSTime( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    if( !guiState->update )
    {
        guiState->edit.gps_set_time = state.gps_set_time ;
    }

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              (guiState->edit.gps_set_time) ? currentLanguage->on : currentLanguage->off );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_GPSTimeZone( GuiState_st* guiState )
{
    char   valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    int8_t tz_hr ;
    int8_t tz_mn ;
    char   sign  = ' ';

    if( !guiState->update )
    {
        guiState->edit.settings.utc_timezone = state.settings.utc_timezone ;
    }

    tz_hr = ( guiState->edit.settings.utc_timezone / 2 ) ;
    tz_mn = ( guiState->edit.settings.utc_timezone % 2 ) * 5 ;

    if( guiState->edit.settings.utc_timezone > 0 )
    {
        sign = '+' ;
    }
    else if( guiState->edit.settings.utc_timezone < 0 )
    {
        sign   = '-' ;
        tz_hr *= (-1) ;
        tz_mn *= (-1) ;
    }

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%c%d.%d" , sign , tz_hr , tz_mn );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}
#endif // GPS_PRESENT

// Radio
static void GuiValDsp_RadioOffset( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    int32_t offset = 0 ;

    if( !guiState->update )
    {
        guiState->edit.channel.tx_frequency = state.channel.tx_frequency ;
        guiState->edit.channel.rx_frequency = state.channel.rx_frequency ;
    }

    offset = (int32_t)guiState->edit.channel.tx_frequency -
             (int32_t)guiState->edit.channel.rx_frequency   ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%dKHz" , (int)offset );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_RadioDirection( GuiState_st* guiState )
{
    char chDirection ;
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
/*
    if( !guiState->update )
    {
        guiState->edit.direction = true ;
    }
*/
    if( !guiState->edit.direction )
    {
        chDirection = '-' ;
    }
    else
    {
        chDirection = '+' ;
    }

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%c" , chDirection );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_RadioStep( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    if( !guiState->update )
    {
        guiState->edit.step_index = state.step_index ;
    }

    // Print in kHz if it is smaller than 1MHz
    if( freq_steps[ guiState->edit.step_index ] < 1000000 )
    {
        snprintf( valueBuffer , MAX_ENTRY_LEN , "%gkHz" , (float)freq_steps[ guiState->edit.step_index ] / 1000.0f );
    }
    else
    {
        snprintf( valueBuffer , MAX_ENTRY_LEN , "%gMHz" , (float)freq_steps[ guiState->edit.step_index ] / 1000000.0f );
    }
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_M17Callsign( GuiState_st* guiState )
{
    uint8_t indexDst ;
    uint8_t indexSrc ;
    char    valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    if( !guiState->update )
    {
        for( indexDst = 0 ; indexDst < CALLSIGN_MAX_LENGTH ; indexDst++ )
        {
            guiState->edit.settings.callsign[ indexDst ] = state.settings.callsign[ indexDst ] ;
        }
    }

    if( !guiState->layout.varInputDisplay )
    {
        for( indexDst = 0 ; indexDst < CALLSIGN_MAX_LENGTH ; indexDst++ )
        {
            valueBuffer[ indexDst ] = guiState->edit.settings.callsign[ indexDst ] ;
            if( !valueBuffer[ indexDst ] )
            {
                break ;
            }
        }
    }
    else
    {
        for( indexDst = 0 , indexSrc = 0 ; indexSrc < guiState->layout.varInputSelect ; indexDst++ , indexSrc++ )
        {
            valueBuffer[ indexDst ] = guiState->edit.settings.callsign[ indexSrc ] ;
        }

        valueBuffer[ indexDst ] = '[' ;
        indexDst++ ;
        valueBuffer[ indexDst ] = guiState->edit.settings.callsign[ indexSrc ] ;
        indexDst++ ;
        valueBuffer[ indexDst ] = ']' ;
        indexDst++ ;
        indexSrc++ ;

        while( indexDst < CALLSIGN_MAX_LENGTH )
        {
            valueBuffer[ indexDst ] = guiState->edit.settings.callsign[ indexSrc ] ;
            if( !valueBuffer[ indexDst ] )
            {
                break ;
            }
            indexDst++ ;
            indexSrc++ ;
        }

    }

    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_M17Can( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    if( !guiState->update )
    {
        guiState->edit.settings.m17_can = state.settings.m17_can ;
    }

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , guiState->edit.settings.m17_can );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_M17CanRxCheck( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    if( !guiState->update )
    {
        guiState->edit.settings.m17_can_rx = state.settings.m17_can_rx ;
    }

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              (guiState->edit.settings.m17_can_rx) ? currentLanguage->on : currentLanguage->off );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

// Accessibility - Voice
static void GuiValDsp_Voice( GuiState_st* guiState )
{
    char    valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    uint8_t value = state.settings.vpLevel ;

    if( !guiState->update )
    {
        guiState->edit.settings.vpLevel = state.settings.vpLevel ;
    }

    value = guiState->edit.settings.vpLevel ;

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
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_Phonetic( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    if( !guiState->update )
    {
        guiState->edit.settings.vpPhoneticSpell = state.settings.vpPhoneticSpell ;
    }

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s" ,
              guiState->edit.settings.vpPhoneticSpell ? currentLanguage->on : currentLanguage->off );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

// Info
static void GuiValDsp_BatteryVoltage( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    // Compute integer part and mantissa of voltage value, adding 50mV
    // to mantissa for rounding to nearest integer
    uint16_t volt  = ( state.v_bat + 50 ) / 1000 ;
    uint16_t mvolt = ( ( state.v_bat - volt * 1000 ) + 50 ) / 100 ;
    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d.%dV" , volt, mvolt );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_BatteryCharge( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d%%" , state.charge );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_Rssi( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%.1fdBm" , state.rssi );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_UsedHeap( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%dB" , getHeapSize() - getCurrentFreeHeap() );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_Band( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%s %s" , hwinfo->vhf_band ? currentLanguage->VHF : "" , hwinfo->uhf_band ? currentLanguage->UHF : "" );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_Vhf( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d - %d" , hwinfo->vhf_minFreq, hwinfo->vhf_maxFreq );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_Uhf( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();

    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d - %d" , hwinfo->uhf_minFreq, hwinfo->uhf_maxFreq );
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_HwVersion( GuiState_st* guiState )
{
    char      valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
    hwInfo_t* hwinfo = (hwInfo_t*)platform_getHwInfo();
    snprintf( valueBuffer , MAX_ENTRY_LEN , "%d" , hwinfo->hw_version );

    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

#ifdef PLATFORM_TTWRPLUS
static void GuiValDsp_Radio( GuiState_st* guiState )
{
    (void)guiState ;
    //@@@KL Populate
//    GuiValDsp_Disp_Val( guiState , valueBuffer );
}

static void GuiValDsp_RadioFw( GuiState_st* guiState )
{
    (void)guiState ;
    //@@@KL Populate
//    GuiValDsp_Disp_Val( guiState , valueBuffer );
}
#endif // PLATFORM_TTWRPLUS

static void GuiValDsp_BackupRestore( GuiState_st* guiState )
{
    (void)guiState ;
}

static void GuiValDsp_LowBattery( GuiState_st* guiState )
{
    GuiValDsp_Disp_Bat( guiState , 10 );
}

#ifdef ENABLE_DEBUG_MSG
  #ifndef DISPLAY_DEBUG_MSG
static void GuiValDsp_DebugCh( GuiState_st* guiState )
{
           char debugMsg[ 2 ] = { 'A' , '\0' } ;
    static char count = 0 ;

    debugMsg[ 0 ] += count ;
    count++ ;
    if( count == 26 )
    {
        count = 0 ;
    }
    GuiValDsp_Disp_Val( guiState , debugMsg );

    guiState->layout.line.pos.w = 0 ;
    guiState->layout.line.pos.h = 0 ;

}

static void GuiValDsp_DebugGfx( GuiState_st* guiState )
{
    (void)guiState ;
    Pos_st   pos = { 40 , 40 , 20 , 10 } ;
    Color_st color_fg ;

    ui_ColorLoad( &color_fg , COLOR_GREEN );
    gfx_drawRect( &pos , &color_fg , true );

}
  #else // DISPLAY_DEBUG_MSG
void GuiValDsp_SetDebugMessage( char* debugMsg )
{
    uint8_t index ;

    for( index = 0 ;
         debugMsg[ index ]           &&
         ( index < DEBUG_MSG_ALLOC )    ;
         index++ )
    {
        DebugMsg[ index ] = debugMsg[ index ] ;
    }
    DebugMsg[ index ] = '\0' ;
}

static void GuiValDsp_DebugMsg( GuiState_st* guiState )
{
    GuiValDsp_Disp_Val( guiState , DebugMsg );

    guiState->layout.line.pos.w = 0 ;
    guiState->layout.line.pos.h = 0 ;
}

void GuiValDsp_SetDebugValues( uint8_t debugVal0 , uint8_t debugVal1 ,
                            uint8_t debugVal2 , uint8_t debugVal3 ,
                            uint8_t debugVal4 , uint8_t debugVal5   )
{
    DebugVal0 = debugVal0 ;
    DebugVal1 = debugVal1 ;
    DebugVal2 = debugVal2 ;
    DebugVal3 = debugVal3 ;
    DebugVal4 = debugVal4 ;
    DebugVal5 = debugVal5 ;
}

static void GuiValDsp_DebugValues( GuiState_st* guiState )
{
    char valueBuffer[ 65 ] ;

    snprintf( valueBuffer , 64 , "%d   \n%d   \n%d   \n%d   \n%d   \n%d   \n%d   " ,
    DebugVal0 , DebugVal1 , DebugVal2 , DebugVal3 ,
    DebugVal4 , DebugVal5 , DebugCnt );

    GuiValDsp_Disp_Val( guiState , valueBuffer );

    guiState->layout.line.pos.w = 0 ;
    guiState->layout.line.pos.h = 0 ;

    DebugCnt++;

}
  #endif // DISPLAY_DEBUG_MSG
#endif // ENABLE_DEBUG_MSG

// Default
static void GuiValDsp_Stubbed( GuiState_st* guiState )
{
    char valueBuffer[ MAX_ENTRY_LEN + 1 ] ;

    valueBuffer[ 0 ] = '?' ;
    valueBuffer[ 1 ] = '\0' ;
    GuiValDsp_Disp_Val( guiState , valueBuffer );

}

static void GuiValDsp_Disp_Val( GuiState_st* guiState , char* valueBuffer )
{
    Pos_st   pos        = guiState->layout.line.pos ;
    Color_st color_text ;

    ui_ColorLoad( &color_text , guiState->layout.style.colorFG );

    if( guiState->update )
    {
        GuiValDsp_Disp_ClearVarPos( guiState );
    }

    pos.y = guiState->layout.line.textY ;

    guiState->layout.itemPos = gfx_print( &pos                  ,
                               guiState->layout.style.font.size ,
                               guiState->layout.style.align     ,
                               &color_text , valueBuffer          );

    GuiValDsp_Disp_StoreVarPos( guiState );

    ui_RenderDisplay( guiState );
}

static void GuiValDsp_Disp_Bat( GuiState_st* guiState , uint16_t percentage )
{
    Pos_st pos ;

    pos.w = ( SCREEN_WIDTH / 9 ) + ( SCREEN_WIDTH / 24 ) ;
    pos.h = guiState->layout.line.height -
            ( guiState->layout.status_v_pad * 2 ) ;
    pos.y = guiState->layout.line.pos.y -
            guiState->layout.status_v_pad ;

    switch( guiState->layout.style.align )
    {
        case GFX_ALIGN_LEFT :
        {
            pos.x = guiState->layout.horizontal_pad ;
            break ;
        }
        case GFX_ALIGN_CENTER :
        {
            pos.x = ( SCREEN_WIDTH - pos.w ) / 2 ;
            break ;
        }
        case GFX_ALIGN_RIGHT :
        {
            pos.x = ( SCREEN_WIDTH - pos.w ) - guiState->layout.horizontal_pad ;
            break ;
        }
    }

    if( GuiValDsp_Disp_HasValueChanged( guiState , (uint32_t)percentage ) )
    {
        if( guiState->update )
        {
            GuiValDsp_Disp_ClearVarPos( guiState );
        }
        guiState->layout.itemPos  = gfx_drawBattery( &pos , percentage );
        GuiValDsp_Disp_StoreVarPos( guiState );
        ui_RenderDisplay( guiState );
    }
}

static void GuiValDsp_Disp_StoreVarPos( GuiState_st* guiState )
{
    guiState->layout.vars[ guiState->layout.varIndex ].pos =
    guiState->layout.itemPos ;
}

static void GuiValDsp_Disp_ClearVarPos( GuiState_st* guiState )
{
    Pos_st   pos      = guiState->layout.vars[ guiState->layout.varIndex ].pos ;
    Color_st color_bg ;

    ui_ColorLoad( &color_bg , guiState->layout.style.colorBG );
    gfx_drawRect( &pos , &color_bg , true );
}

static bool GuiValDsp_Disp_HasValueChanged( GuiState_st* guiState , uint32_t value )
{
    bool valueHasChanged = false ;

    if( !guiState->update )
    {
        valueHasChanged = true ;
    }
    else
    {
        if( guiState->layout.vars[ guiState->layout.varIndex ].value != value )
        {
            valueHasChanged = true ;
        }
    }

    if( valueHasChanged )
    {
        guiState->layout.vars[ guiState->layout.varIndex ].value = value ;
    }

    return valueHasChanged ;
}
