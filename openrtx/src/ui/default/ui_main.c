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

static void ui_drawMainVFO( ui_state_st* uiState , event_t* event );
static void ui_drawMainVFOInput( ui_state_st* uiState , event_t* event );
static void ui_drawMainMEM( ui_state_st* uiState , event_t* event );
static void ui_drawModeVFO( ui_state_st* uiState , event_t* event );
static void ui_drawModeMEM( ui_state_st* uiState , event_t* event );
extern void _ui_drawMenuTop( ui_state_st* uiState , event_t* event );
extern void _ui_drawMenuBank( ui_state_st* uiState , event_t* event );
extern void _ui_drawMenuChannel( ui_state_st* uiState , event_t* event );
extern void _ui_drawMenuContacts( ui_state_st* uiState , event_t* event );
extern void _ui_drawMenuGPS( ui_state_st* uiState , event_t* event );
extern void _ui_drawMenuSettings( ui_state_st* uiState , event_t* event );
extern void _ui_drawMenuBackupRestore( ui_state_st* uiState , event_t* event );
extern void _ui_drawMenuBackup( ui_state_st* uiState , event_t* event );
extern void _ui_drawMenuRestore( ui_state_st* uiState , event_t* event );
extern void _ui_drawMenuInfo( ui_state_st* uiState , event_t* event );
extern void _ui_drawMenuAbout( ui_state_st* uiState , event_t* event );
extern void _ui_drawSettingsTimeDate( ui_state_st* uiState , event_t* event );
extern void _ui_drawSettingsTimeDateSet( ui_state_st* uiState , event_t* event );
extern void _ui_drawSettingsDisplay( ui_state_st* uiState , event_t* event );
extern void _ui_drawSettingsGPS( ui_state_st* uiState , event_t* event );
extern void _ui_drawSettingsRadio( ui_state_st* uiState , event_t* event );
extern void _ui_drawSettingsM17( ui_state_st* uiState , event_t* event );
extern void _ui_drawSettingsVoicePrompts( ui_state_st* uiState , event_t* event );
extern void _ui_drawSettingsReset2Defaults( ui_state_st* uiState , event_t* event );
static void ui_drawLowBatteryScreen( ui_state_st* uiState , event_t* event );
static void ui_drawAuthors( ui_state_st* uiState , event_t* event );
static void ui_drawBlank( ui_state_st* uiState , event_t* event );

//@@@KL static void ui_drawMainBackground( void );
static void ui_drawMainTop( ui_state_st * ui_state , event_t* event );
static void ui_drawBankChannel( void );
static void ui_drawModeInfo( ui_state_st* ui_state );
static void ui_drawFrequency( void );
static void ui_drawVFOMiddleInput( ui_state_st* ui_state );

void _ui_drawMainBottom( event_t* event );

typedef void (*ui_draw_fn)( ui_state_st* uiState , event_t* event );

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
    ui_drawAuthors                 , // PAGE_AUTHORS
    ui_drawBlank                     // PAGE_BLANK
};

void ui_draw( state_t* state , ui_state_st* ui_state , event_t* event )
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

    uiPageDescTable[ pgNum ]( ui_state , event ) ;

}

static void ui_drawMainVFO( ui_state_st* uiState , event_t* event )
{
    (void)event ;

    gfx_clearScreen();
    ui_drawMainTop( uiState , event );
    ui_drawModeInfo( uiState );

    // Show VFO frequency if the OpMode is not M17 or there is no valid LSF data
    rtxStatus_t status = rtx_getCurrentStatus();
    if( ( status.opMode != OPMODE_M17 ) || ( status.lsfOk == false ) )
    {
        ui_drawFrequency();
    }

    _ui_drawMainBottom( event );

}

static void ui_drawMainVFOInput( ui_state_st* uiState , event_t* event )
{
    (void)event ;

    gfx_clearScreen();
    ui_drawMainTop( uiState , event );
    ui_drawVFOMiddleInput( uiState );
    _ui_drawMainBottom( event );

}

void ui_drawMainMEM( ui_state_st* uiState , event_t* event )
{
    (void)event ;

    gfx_clearScreen();
    ui_drawMainTop( uiState , event );
    ui_drawModeInfo( uiState );

    // Show channel data if the OpMode is not M17 or there is no valid LSF data
    rtxStatus_t status = rtx_getCurrentStatus();
    if((status.opMode != OPMODE_M17) || (status.lsfOk == false))
    {
        ui_drawBankChannel();
        ui_drawFrequency();
    }

    _ui_drawMainBottom( event );

}

static void ui_drawModeVFO( ui_state_st* uiState , event_t* event )
{
    (void)event ;
    (void)uiState ;
}

static void ui_drawModeMEM( ui_state_st* uiState , event_t* event )
{
    (void)event ;
    (void)uiState ;
}

static void ui_drawLowBatteryScreen( ui_state_st* uiState , event_t* event )
{
    (void)event ;
    (void)uiState ;

    gfx_clearScreen();
    uint16_t bat_width = SCREEN_WIDTH / 2 ;
    uint16_t bat_height = SCREEN_HEIGHT / 3 ;
    point_t bat_pos = { SCREEN_WIDTH / 4 , SCREEN_HEIGHT / 8 };
    gfx_drawBattery( bat_pos , bat_width , bat_height , 10 );
    point_t text_pos_1 = { 0 , SCREEN_HEIGHT * 2 / 3 };
    point_t text_pos_2 = { 0 , SCREEN_HEIGHT * 2 / 3 + 16 };

    gfx_print( text_pos_1                       ,
               FONT_SIZE_6PT                    ,
               TEXT_ALIGN_CENTER                ,
               color_white                      ,
               currentLanguage->forEmergencyUse   );

    gfx_print( text_pos_2                      ,
               FONT_SIZE_6PT                   ,
               TEXT_ALIGN_CENTER               ,
               color_white                     ,
               currentLanguage->pressAnyButton   );
}

static void ui_drawAuthors( ui_state_st* uiState , event_t* event )
{
    (void)event ;
    (void)uiState ;
}

static void ui_drawBlank( ui_state_st* uiState , event_t* event )
{
    (void)event ;
    (void)uiState ;
}
//@@@KL - not being called - remove?
/*
static void ui_drawMainBackground( void )
{
    // Print top bar line of hline_h pixel height
    gfx_drawHLine(layout.top_h, layout.hline_h, color_grey);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(SCREEN_HEIGHT - layout.bottom_h - 1, layout.hline_h, color_grey);
}
*/
static void ui_drawMainTop( ui_state_st * ui_state , event_t* event )
{
#ifdef RTC_PRESENT
    if( event->payload & EVENT_STATUS_TIME_DISPLAY_TICK )
    {
        // Print clock on top bar
        datetime_t local_time = utcToLocalTime( last_state.time ,
                                                last_state.settings.utc_timezone);
        gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, "%02d:%02d:%02d", local_time.hour,
                  local_time.minute, local_time.second);
    }
#endif // RTC_PRESENT
    if( event->payload & EVENT_STATUS_BATTERY )
    {
        // If the radio has no built-in battery, print input voltage
#ifdef BAT_NONE
        gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white,"%.1fV", last_state.v_bat);
#else // BAT_NONE
        // Otherwise print battery icon on top bar, use 4 px padding
        uint16_t bat_width = SCREEN_WIDTH / 9;
        uint16_t bat_height = layout.top_h - (layout.status_v_pad * 2);
        point_t bat_pos = {SCREEN_WIDTH - bat_width - layout.horizontal_pad,
                           layout.status_v_pad};
        gfx_drawBattery(bat_pos, bat_width, bat_height, last_state.charge);
#endif // BAT_NONE
    }
    if (ui_state->input_locked == true)
      gfx_drawSymbol(layout.top_pos, layout.top_symbol_size, TEXT_ALIGN_LEFT,
                     color_white, SYMBOL_LOCK);
}

static void ui_drawBankChannel( void )
{
    // Print Bank number, channel number and Channel name
    uint16_t b = (last_state.bank_enabled) ? last_state.bank : 0;
    gfx_print(layout.line1_pos, layout.line1_font, TEXT_ALIGN_CENTER,
              color_white, "%01d-%03d: %.12s",
              b, last_state.channel_index + 1, last_state.channel.name);
}

static void ui_drawModeInfo( ui_state_st* uiState )
{
    char bw_str[ 8 ]     = { 0 };
    char encdec_str[ 9 ] = { 0 };

    switch( last_state.channel.mode )
    {
        case OPMODE_FM:
        {
            // Get Bandwidth string
            if( last_state.channel.bandwidth == BW_12_5 )
            {
                snprintf( bw_str , 8 , "NFM" );
            }
            else
            {
                if(last_state.channel.bandwidth == BW_20)
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
                gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, "%s %4.1f %s", bw_str,
                          ctcss_tone[last_state.channel.fm.txTone]/10.0f, encdec_str);
            }
            else
            {
                gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, "%s", bw_str );
            }
            break;
        }
        case OPMODE_DMR:
        {
            // Print talkgroup
            gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                    color_white, "DMR TG%s", "");
            break;
        }
        case OPMODE_M17:
        {
            // Print M17 Destination ID on line 3 of 3
            rtxStatus_t rtxStatus = rtx_getCurrentStatus();

            if(rtxStatus.lsfOk)
            {
                // Destination address
                gfx_drawSymbol(layout.line2_pos, layout.line2_symbol_size, TEXT_ALIGN_LEFT,
                               color_white, SYMBOL_CALL_RECEIVED);

                gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, "%s", rtxStatus.M17_dst);

                // Source address
                gfx_drawSymbol(layout.line1_pos, layout.line1_symbol_size, TEXT_ALIGN_LEFT,
                               color_white, SYMBOL_CALL_MADE);

                gfx_print(layout.line1_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, "%s", rtxStatus.M17_src);

                // RF link (if present)
                if( rtxStatus.M17_link[0] != '\0' )
                {
                    gfx_drawSymbol(layout.line4_pos, layout.line3_symbol_size, TEXT_ALIGN_LEFT,
                                   color_white, SYMBOL_ACCESS_POINT);

                    gfx_print(layout.line4_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                              color_white, "%s", rtxStatus.M17_link);
                }

                // Reflector (if present)
                if( rtxStatus.M17_refl[0] != '\0' )
                {
                    gfx_drawSymbol(layout.line3_pos, layout.line4_symbol_size, TEXT_ALIGN_LEFT,
                                   color_white, SYMBOL_NETWORK);

                    gfx_print(layout.line3_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                              color_white, "%s", rtxStatus.M17_refl);
                }
            }
            else
            {
                const char* dst = NULL ;
                if( uiState->edit_mode )
                {
                    dst = uiState->new_callsign ;
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

                gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, "M17 #%s", dst);
            }
            break;
        }
    }
}

static void ui_drawFrequency( void )
{
    unsigned long frequency = platform_getPttStatus() ? last_state.channel.tx_frequency
                                                      : last_state.channel.rx_frequency;
    // Print big numbers frequency
    gfx_print(layout.line3_large_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
              color_white, "%.7g", (float) frequency / 1000000.0f);
}

static void ui_drawVFOMiddleInput( ui_state_st* uiState )
{
    // Add inserted number to string, skipping "Rx: "/"Tx: " and "."
    uint8_t insert_pos = uiState->input_position + 3;
    if(uiState->input_position > 3)
    {
        insert_pos += 1;
    }
    char input_char = uiState->input_number + '0';

    if( uiState->input_set == SET_RX )
    {
        if(uiState->input_position == 0)
        {
            gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ">Rx:%03lu.%04lu",
                      (unsigned long)uiState->new_rx_frequency/1000000,
                      (unsigned long)(uiState->new_rx_frequency%1000000)/100);
        }
        else
        {
            // Replace Rx frequency with underscorses
            if(uiState->input_position == 1)
            {
                strcpy(uiState->new_rx_freq_buf, ">Rx:___.____");
            }
            uiState->new_rx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER, color_white, uiState->new_rx_freq_buf);
        }
        gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
                  color_white, " Tx:%03lu.%04lu",
                  (unsigned long)last_state.channel.tx_frequency/1000000,
                  (unsigned long)(last_state.channel.tx_frequency%1000000)/100);
    }
    else
    {
        if(uiState->input_set == SET_TX)
        {
            gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, " Rx:%03lu.%04lu",
                      (unsigned long)uiState->new_rx_frequency/1000000,
                      (unsigned long)(uiState->new_rx_frequency%1000000)/100);
            // Replace Rx frequency with underscorses
            if(uiState->input_position == 0)
            {
                gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
                          color_white, ">Tx:%03lu.%04lu",
                          (unsigned long)uiState->new_rx_frequency/1000000,
                          (unsigned long)(uiState->new_rx_frequency%1000000)/100);
            }
            else
            {
                if(uiState->input_position == 1)
                {
                    strcpy(uiState->new_tx_freq_buf, ">Tx:___.____");
                }
                uiState->new_tx_freq_buf[insert_pos] = input_char;
                gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER, color_white, uiState->new_tx_freq_buf);
            }
        }
    }
}

void _ui_drawMainBottom( event_t* event )
{
    // Squelch bar
    float rssi            = last_state.rssi ;
    float squelch         = last_state.settings.sqlLevel / 16.0f ;
    uint16_t meter_width  = SCREEN_WIDTH - 2 * layout.horizontal_pad ;
    uint16_t meter_height = layout.bottom_h ;
    point_t meter_pos     = { layout.horizontal_pad , SCREEN_HEIGHT - meter_height - layout.bottom_pad };
    uint8_t mic_level     = platform_getMicLevel();

    if( event->payload & EVENT_STATUS_RSSI )
    {
        switch( last_state.channel.mode )
        {
            case OPMODE_FM:
            {
                gfx_drawSmeter(meter_pos,
                               meter_width,
                               meter_height,
                               rssi,
                               squelch,
                               yellow_fab413);
                break;
            }
            case OPMODE_DMR:
            {
                gfx_drawSmeterLevel(meter_pos,
                                    meter_width,
                                    meter_height,
                                    rssi,
                                    mic_level);
                break;
            }
            case OPMODE_M17:
            {
                gfx_drawSmeterLevel(meter_pos,
                                    meter_width,
                                    meter_height,
                                    rssi,
                                    mic_level);
                break;
            }
        }
    }
}

