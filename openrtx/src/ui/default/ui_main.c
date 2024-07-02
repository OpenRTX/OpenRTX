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
#include <ui.h>
#include <ui/ui_default.h>
#include "ui_commands.h"
#include <string.h>
#include <ui/ui_strings.h>

//static void ui_Draw_MainVFO( GuiState_st* guiState , bool update , Event_st* event );
//static void ui_Draw_MainVFOInput( GuiState_st* guiState , bool update , Event_st* event );
//static void ui_Draw_MainMEM( GuiState_st* guiState , bool update , Event_st* event );
//static void ui_Draw_ModeVFO( GuiState_st* guiState , bool update , Event_st* event );
//static void ui_Draw_ModeMEM( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_MenuTop( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_MenuBank( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_MenuChannel( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_MenuContacts( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_MenuGPS( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_MenuSettings( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_MenuBackupRestore( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_MenuBackup( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_MenuRestore( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_MenuInfo( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_MenuAbout( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_SettingsTimeDate( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_SettingsTimeDateSet( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_SettingsDisplay( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_SettingsGPS( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_SettingsRadio( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_SettingsM17( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_SettingsVoicePrompts( GuiState_st* guiState , bool update , Event_st* event );
extern void _ui_Draw_SettingsReset2Defaults( GuiState_st* guiState , bool update , Event_st* event );
//static void ui_Draw_LowBatteryScreen( GuiState_st* guiState , bool update , Event_st* event );
//static void ui_Draw_Authors( GuiState_st* guiState , bool update , Event_st* event );
//static void ui_Draw_Blank( GuiState_st* guiState , bool update , Event_st* event );

//@@@KL static void ui_Draw_MainBackground( void );
//static void ui_Draw_MainTop( GuiState_st* guiState , Event_st* event );
//static void ui_Draw_BankChannel( GuiState_st* guiState );
//static void ui_Draw_ModeInfo( GuiState_st* guiState );
//static void ui_Draw_Frequency( GuiState_st* guiState );
//static void ui_Draw_VFOMiddleInput( GuiState_st* guiState );

void _ui_Draw_MainBottom( GuiState_st* guiState , Event_st* event );
/*
typedef void (*ui_Draw__fn)( GuiState_st* guiState , bool update , Event_st* event );

static const ui_Draw__fn uiPageDescTable[ PAGE_NUM_OF ] =
{
    ui_Draw_MainVFO                 , // PAGE_MAIN_VFO
    ui_Draw_MainVFOInput            , // PAGE_MAIN_VFO_INPUT
    ui_Draw_MainMEM                 , // PAGE_MAIN_MEM
    ui_Draw_ModeVFO                 , // PAGE_MODE_VFO
    ui_Draw_ModeMEM                 , // PAGE_MODE_MEM
    _ui_Draw_MenuTop                , // PAGE_MENU_TOP
    _ui_Draw_MenuBank               , // PAGE_MENU_BANK
    _ui_Draw_MenuChannel            , // PAGE_MENU_CHANNEL
    _ui_Draw_MenuContacts           , // PAGE_MENU_CONTACTS
    _ui_Draw_MenuGPS                , // PAGE_MENU_GPS
    _ui_Draw_MenuSettings           , // PAGE_MENU_SETTINGS
    _ui_Draw_MenuBackupRestore      , // PAGE_MENU_BACKUP_RESTORE
    _ui_Draw_MenuBackup             , // PAGE_MENU_BACKUP
    _ui_Draw_MenuRestore            , // PAGE_MENU_RESTORE
    _ui_Draw_MenuInfo               , // PAGE_MENU_INFO
    _ui_Draw_SettingsTimeDate       , // PAGE_SETTINGS_TIMEDATE
    _ui_Draw_SettingsTimeDateSet    , // PAGE_SETTINGS_TIMEDATE_SET
    _ui_Draw_SettingsDisplay        , // PAGE_SETTINGS_DISPLAY
    _ui_Draw_SettingsGPS            , // PAGE_SETTINGS_GPS
    _ui_Draw_SettingsRadio          , // PAGE_SETTINGS_RADIO
    _ui_Draw_SettingsM17            , // PAGE_SETTINGS_M17
    _ui_Draw_SettingsVoicePrompts   , // PAGE_SETTINGS_VOICE
    _ui_Draw_SettingsReset2Defaults , // PAGE_SETTINGS_RESET_TO_DEFAULTS
    ui_Draw_LowBatteryScreen        , // PAGE_LOW_BAT
    ui_Draw_Authors                 , // PAGE_ABOUT
    ui_Draw_Blank                     // PAGE_STUBBED
};
*/
void ui_Draw_Page( GuiState_st* guiState , Event_st* event )
{
    static uint8_t prevPageNum = ~0 ;
           bool    drawPage    = false ;

    guiState->update          = false ;
    guiState->page.renderPage = false ;
    guiState->event           = *event ;

    if( guiState->page.num != prevPageNum )
    {
        guiState->initialPageDisplay = true ;
        prevPageNum                  = guiState->page.num ;
        drawPage                     = true ;
    }
    else
    {
        guiState->initialPageDisplay = false ;
        switch( guiState->event.type )
        {
            case EVENT_TYPE_KBD :
            {
                drawPage = true ;
                break ;
            }
            case EVENT_TYPE_STATUS :
            {
                if( guiState->event.payload & EVENT_STATUS_DEVICE )
                {
                    guiState->update = true ; //@@@KL comment out to remove the flashing
                    drawPage         = true ;
                }
                break ;
            }
            case EVENT_TYPE_RTX :
            {
                drawPage = true ;
                break ;
            }
            default :
            {
                drawPage = true ;
                break ;
            }
        }
    }

    if( drawPage )
    {
        ui_DisplayPage( guiState );

        // attempt to display the page as a scripted page
        //@@@KL redact
/*        if( !ui_DisplayPage( guiState ) )
        {
            // if not successful - display using the legacy fn.
//            uiPageDescTable[ guiState->page.num ]( guiState , guiState->update , &guiState->event ) ;
        }*/
    }

}
/*
static void ui_Draw_MainVFO( GuiState_st* guiState , bool update , Event_st* event )
{
    ui_Draw_MainTop( guiState , event );
    if( !update )
    {
        ui_Draw_ModeInfo( guiState );

        // Show VFO frequency if the OpMode is not M17 or there is no valid LSF data
        rtxStatus_t status = rtx_getCurrentStatus();
        if( ( status.opMode != OPMODE_M17 ) || ( status.lsfOk == false ) )
        {
            ui_Draw_Frequency( guiState );
        }
    }
    _ui_Draw_MainBottom( guiState , event );

}
*/
/*
static void ui_Draw_MainVFOInput( GuiState_st* guiState , bool update , Event_st* event )
{
    ui_Draw_MainTop( guiState , event );
    if( !update )
    {
        ui_Draw_VFOMiddleInput( guiState );
    }
    _ui_Draw_MainBottom( guiState , event );

}
*/
/*
void ui_Draw_MainMEM( GuiState_st* guiState , bool update , Event_st* event )
{
    ui_Draw_MainTop( guiState , event );
    if( !update )
    {
        ui_Draw_ModeInfo( guiState );
        // Show channel data if the OpMode is not M17 or there is no valid LSF data
        rtxStatus_t status = rtx_getCurrentStatus();
        if( ( status.opMode != OPMODE_M17 ) || ( status.lsfOk == false ) )
        {
            ui_Draw_BankChannel( guiState );
            ui_Draw_Frequency( guiState );
        }
    }
    _ui_Draw_MainBottom( guiState , event );

}
*/
/*
static void ui_Draw_ModeVFO( GuiState_st* guiState , bool update , Event_st* event )
{
    (void)guiState ;
    (void)update ;
    (void)event ;
}
*/
/*
static void ui_Draw_ModeMEM( GuiState_st* guiState , bool update , Event_st* event )
{
    (void)guiState ;
    (void)update ;
    (void)event ;
}
*/
/*
static void ui_Draw_LowBatteryScreen( GuiState_st* guiState , bool update , Event_st* event )
{
    (void)guiState ;
    (void)update ;
    (void)event ;
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    if( !update )
    {
                 Pos_st bat_pos = { SCREEN_WIDTH / 4 , SCREEN_HEIGHT / 8 ,
                                    SCREEN_WIDTH / 2 , SCREEN_HEIGHT / 3 };

                 gfx_drawBattery( &bat_pos , 10 );

        Pos_st  text_pos_1      = { 0 , SCREEN_HEIGHT * 2 / 3 , 0 , 0 };
        Pos_st  text_pos_2      = { 0 , SCREEN_HEIGHT * 2 / 3 + 16 , 0 , 0 };

        gfx_print( &text_pos_1                      ,
                   FONT_SIZE_6PT                    ,
                   GFX_ALIGN_CENTER                     ,
                   &color_fg                        ,
                   currentLanguage->forEmergencyUse   );

        gfx_print( &text_pos_2                     ,
                   FONT_SIZE_6PT                   ,
                   GFX_ALIGN_CENTER                    ,
                   &color_fg                       ,
                   currentLanguage->pressAnyButton   );
        guiState->page.renderPage = true ;
    }
}
*/
/*
static void ui_Draw_Authors( GuiState_st* guiState , bool update , Event_st* event )
{
    (void)guiState ;
    (void)update ;
    (void)event ;
}
*/
/*
static void ui_Draw_Blank( GuiState_st* guiState , bool update , Event_st* event )
{
    (void)guiState ;
    (void)update ;
    (void)event ;
}
*/
//@@@KL - not being called - remove?
/*
static void ui_Draw_MainBackground( void )
{
    // Print top bar line of hline_h pixel height
    gfx_drawHLine(lineTop->height, guiState->layout.hline_h, color_gg);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(SCREEN_HEIGHT - guiState->layout.lineStyle[ GUI_LINE_BOTTOM ].height - 1, guiState->layout.hline_h, color_gg);
}
*/
/*
static void ui_Draw_MainTop( GuiState_st* guiState , Event_st* event )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Pos_st    start ;
    Color_st  color_bg ;
    Color_st  color_fg ;
    ui_ColorLoad( &color_bg , COLOR_BG );
    ui_ColorLoad( &color_fg , COLOR_FG );

    if( event->type == EVENT_TYPE_STATUS )
    {
#ifdef RTC_PRESENT
        if( event->payload & EVENT_STATUS_DISPLAY_TIME_TICK )
        {
            // clear the time display area
            //@@@KL needs to be more objectively determined
            start.h   = lineTop->height ;
            start.w   = 68 ;
            start.y   = ( lineTop->pos.y - start.h ) + 1 ;
            start.x   = 44 ;
            gfx_clearRectangle( &start );

            // Print clock on top bar
            datetime_t local_time = utcToLocalTime( last_state.time ,
                                                    last_state.settings.utc_timezone );
            gfx_print( &lineTop->pos ,
                       styleTop->font.size , GFX_ALIGN_CENTER ,
                       &color_fg , "%02d:%02d:%02d" , local_time.hour ,
                       local_time.minute , local_time.second );
            guiState->page.renderPage = true ;
        }
#endif // RTC_PRESENT
        if( event->payload & EVENT_STATUS_BATTERY )
        {
            // clear the time display area
            start.w = SCREEN_WIDTH / 9 ;
            start.h = lineTop->height - ( guiState->layout.status_v_pad * 2 );
            start.x = SCREEN_WIDTH - start.w - guiState->layout.horizontal_pad ;
            start.y = guiState->layout.status_v_pad ;
            gfx_clearRectangle( &start );
            // If the radio has no built-in battery, print input voltage
#ifdef BAT_NONE
            gfx_print( &lineTop->pos ,
                       styleTop->font.size ,
                       GFX_ALIGN_RIGHT , &color_fg , "%.1fV" , last_state.v_bat );
#else // BAT_NONE
            // Otherwise print battery icon on top bar, use 4 px padding
            gfx_drawBattery( &start , last_state.charge );
#endif // BAT_NONE
            guiState->page.renderPage = true ;
        }
        if( event->payload & EVENT_STATUS_DISPLAY_TIME_TICK )
        {
            if( guiState->uiState.input_locked == true )
            {
                start  = lineTop->pos ;
                start.w = styleTop->symbolSize + FONT_SIZE_24PT + 1 ;
                start.h = start.w ;
                gfx_drawSymbol( &start , start.w , GFX_ALIGN_LEFT , &color_fg , SYMBOL_LOCK );
                guiState->page.renderPage = true ;
            }
        }
    }
}
*/
/*
static void ui_Draw_BankChannel( GuiState_st* guiState )
{
    Line_st*  line1    = &guiState->layout.lines[ GUI_LINE_1 ] ;
    Style_st* style1   = &guiState->layout.styles[ GUI_STYLE_1 ] ;
    Color_st  color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    // Print Bank number, channel number and Channel name
    uint16_t bank_enabled = ( last_state.bank_enabled ) ? last_state.bank : 0 ;
    gfx_print( &line1->pos , style1->font.size , GFX_ALIGN_CENTER ,
               &color_fg , "%01d-%03d: %.12s" ,
               bank_enabled , last_state.channel_index + 1 , last_state.channel.name );
    guiState->page.renderPage = true ;
}
*/
/*
static void ui_Draw_ModeInfo( GuiState_st* guiState )
{
    Line_st*  line1       = &guiState->layout.lines[ GUI_LINE_1 ] ;
    Style_st* style1      = &guiState->layout.styles[ GUI_STYLE_1 ] ;
    Line_st*  line2       = &guiState->layout.lines[ GUI_LINE_2 ] ;
    Style_st* style2      = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Line_st*  line3       = &guiState->layout.lines[ GUI_LINE_3 ] ;
    Style_st* style3      = &guiState->layout.styles[ GUI_STYLE_3 ] ;
    Line_st*  line4       = &guiState->layout.lines[ GUI_LINE_4 ] ;
    Style_st* style4      = &guiState->layout.styles[ GUI_STYLE_4 ] ;
    char      bw_str[ 8 ]     = { 0 };
    char      encdec_str[ 9 ] = { 0 };
    Color_st  color_fg ;

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
                gfx_print( &line2->pos , style2->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "%s %4.1f %s" , bw_str ,
                           ctcss_tone[ last_state.channel.fm.txTone ] / 10.0f , encdec_str );
            }
            else
            {
                gfx_print( &line2->pos , style2->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "%s" , bw_str );
            }
            guiState->page.renderPage = true ;
            break ;
        }
        case OPMODE_DMR :
        {
            // Print Contact
            gfx_print( &line2->pos , style2->font.size , GFX_ALIGN_CENTER ,
                       &color_fg , "%s" , last_state.contact.name );
            guiState->page.renderPage = true ;
            break ;
        }
        case OPMODE_M17 :
        {
            // Print M17 Destination ID on line 3 of 3
            rtxStatus_t rtxStatus = rtx_getCurrentStatus();

            if( rtxStatus.lsfOk )
            {
                // Destination address
                gfx_drawSymbol( &line2->pos , style2->symbolSize , GFX_ALIGN_LEFT ,
                                &color_fg , SYMBOL_CALL_RECEIVED );

                gfx_print( &line2->pos , style2->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "%s" , rtxStatus.M17_dst );

                // Source address
                gfx_drawSymbol( &line1->pos , style1->symbolSize , GFX_ALIGN_LEFT ,
                                &color_fg , SYMBOL_CALL_MADE );

                gfx_print( &line1->pos , style2->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "%s" , rtxStatus.M17_src );

                // RF link (if present)
                if( rtxStatus.M17_link[0] != '\0' )
                {
                    gfx_drawSymbol( &line4->pos , style3->symbolSize , GFX_ALIGN_LEFT ,
                                    &color_fg , SYMBOL_ACCESS_POINT );
                    gfx_print( &line4->pos , style2->font.size , GFX_ALIGN_CENTER ,
                               &color_fg , "%s" , rtxStatus.M17_link );
                }

                // Reflector (if present)
                if( rtxStatus.M17_refl[0] != '\0' )
                {
                    gfx_drawSymbol( &line3->pos , style4->symbolSize , GFX_ALIGN_LEFT ,
                                    &color_fg , SYMBOL_NETWORK );
                    gfx_print( &line3->pos , style2->font.size , GFX_ALIGN_CENTER ,
                               &color_fg , "%s" , rtxStatus.M17_refl );
                }
                guiState->page.renderPage = true ;
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

                gfx_print( &line2->pos , style2->font.size , GFX_ALIGN_CENTER ,
                           &color_fg , "M17 #%s" , dst );
                guiState->page.renderPage = true ;
            }
            break ;
        }
    }
}
*/
/*
static void ui_Draw_Frequency( GuiState_st* guiState )
{
    Line_st*      line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
    Style_st*     style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    unsigned long frequency   = platform_getPttStatus() ? last_state.channel.tx_frequency
                                                      : last_state.channel.rx_frequency;
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    // Print big numbers frequency
    gfx_print( &line3Large->pos , style3Large->font.size , GFX_ALIGN_CENTER ,
               &color_fg , "%.7g" , (float)frequency / 1000000.0f );
    guiState->page.renderPage = true ;
}
*/
/*
static void ui_Draw_VFOMiddleInput( GuiState_st* guiState )
{
    Line_st*  line2       = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2      = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
//    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    // Add inserted number to string, skipping "Rx: "/"Tx: " and "."
    uint8_t   insert_pos  = guiState->uiState.input_position + 3;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    if( guiState->uiState.input_position > 3 )
    {
        insert_pos += 1 ;
    }
    char input_char = guiState->uiState.input_number + '0' ;

    if( guiState->uiState.input_set == SET_RX )
    {
        if( guiState->uiState.input_position == 0 )
        {
            gfx_print( &line2->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
                       &color_fg , ">Rx:%03lu.%04lu" ,
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
            gfx_print( &line2->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
                       &color_fg , guiState->uiState.new_rx_freq_buf );
        }
        gfx_print( &line3Large->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
                   &color_fg , " Tx:%03lu.%04lu" ,
                   (unsigned long)last_state.channel.tx_frequency / 1000000 ,
                   (unsigned long)( last_state.channel.tx_frequency % 1000000 ) / 100 );
        guiState->page.renderPage = true ;
    }
    else
    {
        if( guiState->uiState.input_set == SET_TX )
        {
            gfx_print( &line2->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
                       &color_fg , " Rx:%03lu.%04lu" ,
                       (unsigned long)guiState->uiState.new_rx_frequency / 1000000 ,
                       (unsigned long)( guiState->uiState.new_rx_frequency % 1000000 ) / 100 );
            // Replace Rx frequency with underscorses
            if( guiState->uiState.input_position == 0 )
            {
                gfx_print( &line3Large->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
                           &color_fg , ">Tx:%03lu.%04lu" ,
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
                gfx_print( &line3Large->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
                           &color_fg , guiState->uiState.new_tx_freq_buf );
            }
            guiState->page.renderPage = true ;
        }
    }
}
*/
void _ui_Draw_MainBottom( GuiState_st* guiState , Event_st* event )
{
    Line_st*  lineBottom  = &guiState->layout.lines[ GUI_LINE_BOTTOM ] ;
//    Style_st* styleBottom = &guiState->layout.styles[ GUI_STYLE_BOTTOM ] ;
    // Squelch bar
    float    rssi         = last_state.rssi ;
    float    squelch      = last_state.settings.sqlLevel / 16.0f ;
    Pos_st   meter_pos ;
    uint8_t  mic_level    = platform_getMicLevel();
    Color_st color_op3 ;

    meter_pos.x  = guiState->layout.horizontal_pad ;
    meter_pos.y  = ( SCREEN_HEIGHT - lineBottom->height ) -
                   guiState->layout.bottom_pad ;
    meter_pos.w  = SCREEN_WIDTH - ( 2 * guiState->layout.horizontal_pad ) ;
    meter_pos.h  = lineBottom->height ;

    ui_ColorLoad( &color_op3 , COLOR_OP3 );

    if( event->type == EVENT_TYPE_STATUS )
    {
        if( event->payload & EVENT_STATUS_RSSI )
        {
            gfx_clearRectangle( &meter_pos );
            switch( last_state.channel.mode )
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
    }
}
