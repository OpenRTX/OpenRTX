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

#include "ui_menu.h"

       State_st    last_state ;
       bool        macro_latched ;
       bool        macro_menu      = false ;
       bool        redraw_needed   = true ;

static bool        standby         = false ;
       long long   last_event_tick = 0 ;

// UI event queue
       uint8_t     evQueue_rdPos ;
       uint8_t     evQueue_wrPos ;
       Event_st    evQueue[ MAX_NUM_EVENTS ] ;

extern long long getTick();

static void    EnterStandby( void );
static bool    ExitStandby( long long now );
static bool    UpdatePage( GuiState_st* guiState );
static bool    CheckStandby( long long time_since_last_event );
static freq_t  FreqAddDigit( freq_t freq , uint8_t pos , uint8_t number );
static bool    FreqCheckLimits( freq_t freq );
static bool    FreqCheckLimits( freq_t freq );
static void    FSM_MenuMacro( GuiState_st* guiState , kbd_msg_t msg , bool* sync_rtx );
static int     FSM_LoadChannel( int16_t channel_index , bool* sync_rtx );
static void    FSM_ConfirmVFOInput( GuiState_st* guiState , bool* sync_rtx );
static void    FSM_InsertVFONumber( GuiState_st* guiState , kbd_msg_t msg , bool* sync_rtx );
static bool    ui_States_HandleKeyEvent( GuiState_st* guiState );
static void    ui_States_MenuUpNoWrapAround( GuiState_st* guiState );
static void    ui_States_SelectPageNum( GuiState_st* guiState , uint8_t pageNum );
static bool    ui_States_IsEntryPage( GuiState_st* guiState );
//static uint8_t ui_States_GetPageNumOfEntries( GuiState_st* guiState );

static bool ui_updateFSM_PAGE_MAIN_VFO( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MAIN_VFO_INPUT( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MAIN_MEM( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MODE_VFO( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MODE_MEM( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_TOP( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_BANK( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_CHANNEL( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_CONTACTS( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_GPS( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_SETTINGS( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_BACKUP_RESTORE( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_BACKUP( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_RESTORE( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_INFO( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_MENU_ABOUT( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE_SET( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_DISPLAY( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_GPS( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_RADIO( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_M17( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_VOICE( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_SETTINGS_RESET_TO_DEFAULTS( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_LOW_BAT( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_ABOUT( GuiState_st* guiState );
static bool ui_updateFSM_PAGE_STUBBED( GuiState_st* guiState );

typedef bool (*ui_updateFSM_PAGE_fn)( GuiState_st* guiState );

static const ui_updateFSM_PAGE_fn ui_updateFSM_PageTable[ PAGE_NUM_OF ] =
{
    ui_updateFSM_PAGE_MAIN_VFO                   ,
    ui_updateFSM_PAGE_MAIN_VFO_INPUT             ,
    ui_updateFSM_PAGE_MAIN_MEM                   ,
    ui_updateFSM_PAGE_MODE_VFO                   ,
    ui_updateFSM_PAGE_MODE_MEM                   ,
    ui_updateFSM_PAGE_MENU_TOP                   ,
    ui_updateFSM_PAGE_MENU_BANK                  ,
    ui_updateFSM_PAGE_MENU_CHANNEL               ,
    ui_updateFSM_PAGE_MENU_CONTACTS              ,
    ui_updateFSM_PAGE_MENU_GPS                   ,
    ui_updateFSM_PAGE_MENU_SETTINGS              ,
    ui_updateFSM_PAGE_MENU_BACKUP_RESTORE        ,
    ui_updateFSM_PAGE_MENU_BACKUP                ,
    ui_updateFSM_PAGE_MENU_RESTORE               ,
    ui_updateFSM_PAGE_MENU_INFO                  ,
    ui_updateFSM_PAGE_MENU_ABOUT                 ,
    ui_updateFSM_PAGE_SETTINGS_TIMEDATE          ,
    ui_updateFSM_PAGE_SETTINGS_TIMEDATE_SET      ,
    ui_updateFSM_PAGE_SETTINGS_DISPLAY           ,
    ui_updateFSM_PAGE_SETTINGS_GPS               ,
    ui_updateFSM_PAGE_SETTINGS_RADIO             ,
    ui_updateFSM_PAGE_SETTINGS_M17               ,
    ui_updateFSM_PAGE_SETTINGS_VOICE             ,
    ui_updateFSM_PAGE_SETTINGS_RESET_TO_DEFAULTS ,
    ui_updateFSM_PAGE_LOW_BAT                    ,
    ui_updateFSM_PAGE_ABOUT                      ,
    ui_updateFSM_PAGE_STUBBED
};

#ifdef GPS_PRESENT
static float    priorGPSSpeed         =   0 ;
static float    priorGPSAltitude      =   0 ;
static float    priorGPSDirection     = 500 ; // impossible value init.
static uint8_t  priorGPSFixQuality    =   0 ;
static uint8_t  priorGPSFixType       =   0 ;
static uint8_t  priorSatellitesInView =   0 ;
static uint32_t vpGPSLastUpdate       =   0 ;

static VPGPSInfoFlags_t GetGPSDirectionOrSpeedChanged( void )
{
    uint32_t         now ;
    VPGPSInfoFlags_t whatChanged ;
    float            speedDiff ;
    float            altitudeDiff ;
    float            degreeDiff ;

    if( !state.settings.gps_enabled )
    {
        return VPGPS_NONE;
    }

    now = getTick();

    if( ( now - vpGPSLastUpdate ) < 8000 )
    {
        return VPGPS_NONE;
    }

    whatChanged = VPGPS_NONE ;

    if( state.gps_data.fix_quality != priorGPSFixQuality )
    {
        whatChanged        |= VPGPS_FIX_QUALITY ;
        priorGPSFixQuality  = state.gps_data.fix_quality ;
    }

    if( state.gps_data.fix_type != priorGPSFixType )
    {
        whatChanged     |= VPGPS_FIX_TYPE ;
        priorGPSFixType  = state.gps_data.fix_type ;
    }

    speedDiff=fabs( state.gps_data.speed - priorGPSSpeed );
    if( speedDiff >= 1 )
    {
        whatChanged   |= VPGPS_SPEED ;
        priorGPSSpeed  = state.gps_data.speed ;
    }

    altitudeDiff = fabs( state.gps_data.altitude - priorGPSAltitude );

    if( altitudeDiff >= 5 )
    {
        whatChanged      |= VPGPS_ALTITUDE ;
        priorGPSAltitude  = state.gps_data.altitude ;
    }

    degreeDiff = fabs( state.gps_data.tmg_true - priorGPSDirection );

    if( degreeDiff  >= 1 )
    {
        whatChanged       |= VPGPS_DIRECTION ;
        priorGPSDirection  = state.gps_data.tmg_true ;
    }

    if( state.gps_data.satellites_in_view != priorSatellitesInView )
    {
        whatChanged           |= VPGPS_SAT_COUNT ;
        priorSatellitesInView  = state.gps_data.satellites_in_view ;
    }

    if( whatChanged )
    {
        vpGPSLastUpdate = now ;
    }

    return whatChanged ;
}
#endif // GPS_PRESENT

void ui_updateFSM( bool* sync_rtx , Event_st* event )
{
    GuiState_st* guiState     = &GuiState ;
    bool         processEvent = false ;

    if( event->type )
    {
        processEvent = true ;
    }

    if( processEvent )
    {
        // Check if battery has enough charge to operate.
        // Check is skipped if there is an ongoing transmission, since the voltage
        // drop caused by the RF PA power absorption causes spurious triggers of
        // the low battery alert.
        bool txOngoing = platform_getPttStatus();
#if !defined(PLATFORM_TTWRPLUS)
        if( !state.emergency && !txOngoing && ( state.charge <= 0 ) )
        {
            ui_States_SetPageNum( guiState , PAGE_LOW_BAT );
            if( ( event->type == EVENT_TYPE_KBD ) && event->payload )
            {
                ui_States_SetPageNum( guiState , PAGE_MAIN_VFO );
                state.emergency = true ;
            }
            processEvent = false ;
        }
#endif // PLATFORM_TTWRPLUS

        long long timeTick = timeTick = getTick();
        switch( event->type )
        {
            // Process pressed keys
            case EVENT_TYPE_KBD :
            {
                guiState->msg.value  = event->payload ;
                guiState->f1Handled  = false ;
                guiState->queueFlags = vp_getVoiceLevelQueueFlags();
                // If we get out of standby, we ignore the kdb event
                // unless is the MONI key for the MACRO functions
                if( ExitStandby( timeTick ) && !( guiState->msg.keys & KEY_MONI ) )
                {
                    processEvent = false ;
                }

                if( processEvent )
                {
                    // If MONI is pressed, activate MACRO functions
                    bool moniPressed ;
                    moniPressed = guiState->msg.keys & KEY_MONI ;
                    if( moniPressed || macro_latched )
                    {
                        macro_menu = true ;
                        // long press moni on its own latches function.
                        if( moniPressed && guiState->msg.long_press && !macro_latched )
                        {
                            macro_latched = true ;
                            vp_beep( BEEP_FUNCTION_LATCH_ON , LONG_BEEP );
                        }
                        else if( moniPressed && macro_latched )
                        {
                            macro_latched = false ;
                            vp_beep( BEEP_FUNCTION_LATCH_OFF , LONG_BEEP );
                        }
                        FSM_MenuMacro( guiState , guiState->msg , sync_rtx );
                        processEvent = false ;
                    }
                    else
                    {
                        macro_menu = false ;
                    }
                }

                if( processEvent )
                {
#if defined(PLATFORM_TTWRPLUS)
                    // T-TWR Plus has no KEY_MONI, using KEY_VOLDOWN long press instead
                    if( ( guiState->msg.keys & KEY_VOLDOWN ) && guiState->msg.long_press )
                    {
                        macro_menu    = true ;
                        macro_latched = true ;
                    }
#endif // PLA%FORM_TTWRPLUS
                    if( state.tone_enabled && !( guiState->msg.keys & KEY_HASH ) )
                    {
                        state.tone_enabled = false ;
                        *sync_rtx          = true ;
                    }

                    int priorUIScreen = guiState->page.num ;

                    *sync_rtx = UpdatePage( &GuiState );

                    // Enable Tx only if in PAGE_MAIN_VFO or PAGE_MAIN_MEM states
                    bool inMemOrVfo ;
                    inMemOrVfo = ( guiState->page.num == PAGE_MAIN_VFO ) ||
                                 ( guiState->page.num == PAGE_MAIN_MEM )    ;
                    if( (   macro_menu == true                                    ) ||
                        ( ( inMemOrVfo == false ) && ( state.txDisable == false ) )    )
                    {
                        state.txDisable = true;
                        *sync_rtx       = true;
                    }
                    if( !guiState->f1Handled                   &&
                         ( guiState->msg.keys & KEY_F1       ) &&
                         ( state.settings.vpLevel > VPP_BEEP )    )
                    {
                        vp_replayLastPrompt();
                    }
                    else if( ( priorUIScreen != guiState->page.num ) &&
                             ( state.settings.vpLevel > VPP_LOW    )    )
                    {
                        // When we switch to VFO or Channel screen, we need to announce it.
                        // Likewise for information screens.
                        // All other cases are handled as needed.
                        vp_announceScreen( guiState->page.num );
                    }
                    // generic beep for any keydown if beep is enabled.
                    // At vp levels higher than beep, keys will generate voice so no need
                    // to beep or you'll get an unwanted click.
                    if( ( guiState->msg.keys & 0xFFFF ) && ( state.settings.vpLevel == VPP_BEEP ) )
                    {
                        vp_beep( BEEP_KEY_GENERIC , SHORT_BEEP );
                    }
                    // If we exit and re-enter the same menu, we want to ensure it speaks.
                    if( guiState->msg.keys & KEY_ESC )
                    {
                        _ui_reset_menu_anouncement_tracking();
                    }
                }
                redraw_needed = true ;
                break ;
            }// case EVENT_TYPE_KBD :
            case EVENT_TYPE_STATUS :
            {
                redraw_needed = true ;
#ifdef GPS_PRESENT
                if( ( guiState->page.num == PAGE_MENU_GPS ) &&
                    !vp_isPlaying()                         &&
                    ( state.settings.vpLevel > VPP_LOW    ) &&
                    !txOngoing                              &&
                    !rtx_rxSquelchOpen()                       )
                {
                    // automatically read speed and direction changes only!
                    VPGPSInfoFlags_t whatChanged = GetGPSDirectionOrSpeedChanged();
                    if( whatChanged != VPGPS_NONE )
                    {
                        vp_announceGPSInfo( whatChanged );
                    }
                }
#endif //            GPS_PRESENT

                if( txOngoing || rtx_rxSquelchOpen() )
                {
                    if( txOngoing )
                    {
                        macro_latched = 0 ;
                    }
                    ExitStandby( timeTick );
                    processEvent = false ;
                }

                if( processEvent )
                {
                    if( CheckStandby( timeTick - last_event_tick ) )
                    {
                        EnterStandby();
                    }
                }
                break ;
            }// case EVENT_TYPE_STATUS :
        }// switch( event.type )
    }

    // There is some event to process, we need an UI redraw.
    // UI redraw request is cancelled if we're in standby mode.
    if( standby )
    {
        redraw_needed = false ;
    }

}

static void EnterStandby( void )
{
    if( !standby )
    {
        standby       = true ;
        redraw_needed = false ;
        display_setBacklightLevel( 0 );
    }
}

static bool ExitStandby( long long now )
{
    bool result     = false ;

    last_event_tick = now ;

    if( standby )
    {
        standby       = false ;
        redraw_needed = true ;
        display_setBacklightLevel( state.settings.brightness );
        result        = true ;
    }

    return result ;

}

static bool ui_updateFSM_PAGE_MAIN_VFO( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    // Enable Tx in PAGE_MAIN_VFO mode
    if( state.txDisable )
    {
        state.txDisable = false ;
        sync_rtx        = true ;
    }
    // M17 Destination callsign input
    if( !guiState->uiState.input_locked )
    {
        if( guiState->uiState.edit_mode )
        {
            if( state.channel.mode == OPMODE_M17 )
            {
                if( guiState->msg.keys & KEY_ENTER )
                {
                    ui_States_TextInputConfirm( guiState , guiState->uiState.new_callsign );
                    // Save selected dst ID and disable input mode
                    strncpy( state.settings.m17_dest , guiState->uiState.new_callsign , 10 );
                    guiState->uiState.edit_mode = false ;
                    sync_rtx           = true ;
                    vp_announceM17Info( NULL , guiState->uiState.edit_mode , guiState->queueFlags );
                }
                else if( guiState->msg.keys & KEY_HASH )
                {
                    // Save selected dst ID and disable input mode
                    strncpy( state.settings.m17_dest , "" , 1 );
                    guiState->uiState.edit_mode = false ;
                    sync_rtx           = true ;
                    vp_announceM17Info( NULL , guiState->uiState.edit_mode , guiState->queueFlags );
                }
                else if( guiState->msg.keys & KEY_ESC )
                {
                    // Discard selected dst ID and disable input mode
                    guiState->uiState.edit_mode = false ;
                }
                else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
                {
                    ui_States_TextInputDelete( guiState , guiState->uiState.new_callsign );
                }
                else if( input_isNumberPressed( guiState->msg ) )
                {
                    ui_States_TextInputKeypad( guiState , guiState->uiState.new_callsign , 9 , guiState->msg , true );
                }
            }
        }
        else
        {
            if( guiState->msg.keys & KEY_ENTER )
            {
                // Open Menu
                ui_States_SetPageNum( guiState , PAGE_MENU_TOP );
                // The selected item will be announced when the item is first selected.
            }
            else if( guiState->msg.keys & KEY_ESC )
            {
                // Save VFO channel
                state.vfo_channel = state.channel ;
                int result        = FSM_LoadChannel( state.channel_index , &sync_rtx );
                // Read successful and channel is valid
                if(result != -1)
                {
                    // Switch to MEM screen
                    ui_States_SetPageNum( guiState , PAGE_MAIN_MEM );
                    // anounce the active channel name.
                    vp_announceChannelName( &state.channel , state.channel_index , guiState->queueFlags );
                }
            }
            else if( guiState->msg.keys & KEY_HASH )
            {
                // Only enter edit mode when using M17
                if( state.channel.mode == OPMODE_M17 )
                {
                    // Enable dst ID input
                    guiState->uiState.edit_mode = true ;
                    // Reset text input variables
                    ui_States_TextInputReset( guiState , guiState->uiState.new_callsign );
                    vp_announceM17Info( NULL , guiState->uiState.edit_mode , guiState->queueFlags );
                }
                else
                {
                    if(!state.tone_enabled)
                    {
                        state.tone_enabled = true ;
                        sync_rtx           = true ;
                    }
                }
            }
            else if( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) )
            {
                // Increment TX and RX frequency of 12.5KHz
                if( FreqCheckLimits( state.channel.rx_frequency + freq_steps[ state.step_index ] ) &&
                    FreqCheckLimits( state.channel.tx_frequency + freq_steps[ state.step_index ] )    )
                {
                    state.channel.rx_frequency += freq_steps[ state.step_index ];
                    state.channel.tx_frequency += freq_steps[ state.step_index ];
                    sync_rtx                    = true;
                    vp_announceFrequencies( state.channel.rx_frequency , state.channel.tx_frequency , guiState->queueFlags );
                }
            }
            else if( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) )
            {
                // Decrement TX and RX frequency of 12.5KHz
                if( FreqCheckLimits( state.channel.rx_frequency - freq_steps[ state.step_index ] ) &&
                    FreqCheckLimits( state.channel.tx_frequency - freq_steps[ state.step_index ] )    )
                {
                    state.channel.rx_frequency -= freq_steps[ state.step_index ];
                    state.channel.tx_frequency -= freq_steps[ state.step_index ];
                    sync_rtx                    = true ;
                    vp_announceFrequencies( state.channel.rx_frequency , state.channel.tx_frequency , guiState->queueFlags );
                }
            }
            else if( guiState->msg.keys & KEY_F1 )
            {
                if( state.settings.vpLevel > VPP_BEEP )
                {// quick press repeat vp, long press summary.
                    if( guiState->msg.long_press )
                    {
                        vp_announceChannelSummary( &state.channel , 0 , state.bank , VPSI_ALL_INFO );
                    }
                    else
                    {
                        vp_replayLastPrompt();
                    }
                    guiState->f1Handled = true;
                }
            }
            else if( input_isNumberPressed( guiState->msg ) )
            {
                // Open Frequency input screen
                ui_States_SetPageNum( guiState , PAGE_MAIN_VFO_INPUT );
                // Reset input position and selection
                guiState->uiState.input_position = 1 ;
                guiState->uiState.input_set      = SET_RX ;
                // do not play  because we will also announce the number just entered.
                vp_announceInputReceiveOrTransmit( false , VPQ_INIT );
                vp_queueInteger(input_getPressedNumber( guiState->msg ) );
                vp_play();

                guiState->uiState.new_rx_frequency = 0 ;
                guiState->uiState.new_tx_frequency = 0 ;
                // Save pressed number to calculare frequency and show in GUI
                guiState->uiState.input_number     = input_getPressedNumber( guiState->msg );
                // Calculate portion of the new frequency
                guiState->uiState.new_rx_frequency = FreqAddDigit( guiState->uiState.new_rx_frequency , guiState->uiState.input_position , guiState->uiState.input_number );
            }
        }
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MAIN_VFO_INPUT( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ENTER )
    {
        FSM_ConfirmVFOInput( guiState , &sync_rtx );
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        // Cancel frequency input, return to VFO mode
        ui_States_SetPageNum( guiState , PAGE_MAIN_VFO );
    }
    else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN ) )
    {
        //@@@KL why is RX \ TX being toggled?
        switch( guiState->uiState.input_set )
        {
            case SET_RX :
            {
                guiState->uiState.input_set = SET_TX ;
                vp_announceInputReceiveOrTransmit( true , guiState->queueFlags );
                break ;
            }
            case SET_TX :
            {
                guiState->uiState.input_set = SET_RX ;
                vp_announceInputReceiveOrTransmit( false , guiState->queueFlags );
                break ;
            }
        }
        // Reset input position
        guiState->uiState.input_position = 0 ;
    }
    else if( input_isNumberPressed( guiState->msg ) )
    {
        FSM_InsertVFONumber( guiState , guiState->msg , &sync_rtx );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MAIN_MEM( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    // Enable Tx in PAGE_MAIN_MEM mode
    if( state.txDisable )
    {
        state.txDisable = false ;
        sync_rtx        = true ;
    }
    if( !guiState->uiState.input_locked )
    {
        // M17 Destination callsign input
        if( guiState->uiState.edit_mode )
        {
            if( guiState->msg.keys & KEY_ENTER )
            {
                ui_States_TextInputConfirm( guiState , guiState->uiState.new_callsign );
                // Save selected dst ID and disable input mode
                strncpy( state.settings.m17_dest , guiState->uiState.new_callsign , 10 );
                guiState->uiState.edit_mode = false ;
                sync_rtx                    = true ;
            }
            else if( guiState->msg.keys & KEY_HASH )
            {
                // Save selected dst ID and disable input mode
                strncpy( state.settings.m17_dest , "" , 1 );
                guiState->uiState.edit_mode = false;
                sync_rtx                    = true;
            }
            else if( guiState->msg.keys & KEY_ESC )
            {
                // Discard selected dst ID and disable input mode
                guiState->uiState.edit_mode = false ;
            }
            else if( guiState->msg.keys & KEY_F1 )
            {
                if( state.settings.vpLevel > VPP_BEEP )
                {
                    // Quick press repeat vp, long press summary.
                    if( guiState->msg.long_press )
                    {
                        vp_announceChannelSummary( &state.channel , state.channel_index , state.bank , VPSI_ALL_INFO );
                    }
                    else
                    {
                        vp_replayLastPrompt();
                    }

                    guiState->f1Handled = true ;
                }
            }
            else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
            {
                ui_States_TextInputDelete( guiState , guiState->uiState.new_callsign );
            }
            else if( input_isNumberPressed( guiState->msg ) )
            {
                ui_States_TextInputKeypad( guiState , guiState->uiState.new_callsign , 9 , guiState->msg , true );
            }
        }
        else
        {
            if( guiState->msg.keys & KEY_ENTER )
            {
                // Open Menu
                ui_States_SetPageNum( guiState , PAGE_MENU_TOP );
            }
            else if( guiState->msg.keys & KEY_ESC )
            {
                // Restore VFO channel
                state.channel   = state.vfo_channel ;
                // Update RTX configuration
                sync_rtx        = true ;
                // Switch to VFO screen
                ui_States_SetPageNum( guiState , PAGE_MAIN_VFO );
            }
            else if( guiState->msg.keys & KEY_HASH )
            {
                // Only enter edit mode when using M17
                if( state.channel.mode == OPMODE_M17 )
                {
                    // Enable dst ID input
                    guiState->uiState.edit_mode = true ;
                    // Reset text input variables
                    ui_States_TextInputReset( guiState , guiState->uiState.new_callsign );
                }
                else
                {
                    if( !state.tone_enabled )
                    {
                        state.tone_enabled = true ;
                        sync_rtx           = true ;
                    }
                }
            }
            else if( guiState->msg.keys & KEY_F1 )
            {
                if( state.settings.vpLevel > VPP_BEEP )
                {
                    // quick press repeat vp, long press summary.
                    if( guiState->msg.long_press )
                    {
                        vp_announceChannelSummary( &state.channel , state.channel_index+1 , state.bank , VPSI_ALL_INFO );
                    }
                    else
                    {
                        vp_replayLastPrompt();
                    }

                    guiState->f1Handled = true;
                }
            }
            else if( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) )
            {
                FSM_LoadChannel( state.channel_index + 1 , &sync_rtx );
                vp_announceChannelName( &state.channel , state.channel_index + 1 , guiState->queueFlags );
            }
            else if( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) )
            {
                FSM_LoadChannel( state.channel_index - 1 , &sync_rtx );
                vp_announceChannelName( &state.channel , state.channel_index + 1 , guiState->queueFlags );
            }
        }
    }

    return sync_rtx ;

}

char* uiGetPageTextString( uiPageNum_en pageNum , uint8_t textStringIndex )
{
    uiPageNum_en pgNum = pageNum ;
    char*        ptr ;
    uint16_t     index ;
    uint8_t      numOf ;

    if( pgNum >= PAGE_NUM_OF )
    {
        pgNum = PAGE_STUBBED ;
    }

    ptr = (char*)uiPageTable[ pgNum ] ;

    for( numOf = 0 , index = 0 ; ptr[ index ] != GUI_CMD_PAGE_END ; index++ )
    {
        if( ptr[ index ] == GUI_CMD_TEXT )
        {
            if( numOf == textStringIndex )
            {
                ptr = &ptr[ index + 1 ] ;
                break ;
            }
            numOf++ ;
        }
    }

    return ptr ;
}

static bool UpdatePage( GuiState_st* guiState )
{
    bool sync_rtx ;

    sync_rtx = ui_updateFSM_PageTable[ guiState->page.num ]( guiState );

    return sync_rtx ;
}

static freq_t FreqAddDigit( freq_t freq , uint8_t pos , uint8_t number )
{
    freq_t coefficient = 100 ;

    for( uint8_t index = 0 ; index < FREQ_DIGITS - pos ; index++ )
    {
        coefficient *= 10 ;
    }

    return freq += number * coefficient ;

}

#ifdef RTC_PRESENT
static void _ui_timedate_add_digit( datetime_t* timedate ,
                                    uint8_t     pos      ,
                                    uint8_t     number     )
{
    vp_flush();
    vp_queueInteger( number );
    if( ( pos == 2 ) || ( pos == 4 ) )
    {
        vp_queuePrompt( PROMPT_SLASH );
    }
    // just indicates separation of date and time.
    if( pos == 6 ) // start of time.
    {
        vp_queueString( "hh:mm" , VP_ANNOUNCE_COMMON_SYMBOLS | VP_ANNOUNCE_LESS_COMMON_SYMBOLS );
    }
    if( pos == 8 )
    {
        vp_queuePrompt( PROMPT_COLON );
    }
    vp_play();

    switch( pos )
    {
        // Set date
        case 1:
        {
            timedate->date += number * 10 ;
            break ;
        }
        case 2:
        {
            timedate->date += number ;
            break ;
        }
        // Set month
        case 3:
        {
            timedate->month += number * 10 ;
            break ;
        }
        case 4:
        {
            timedate->month += number ;
            break ;
        }
        // Set year
        case 5:
        {
            timedate->year += number * 10 ;
            break ;
        }
        case 6:
        {
            timedate->year += number ;
            break ;
        }
        // Set hour
        case 7:
        {
            timedate->hour += number * 10 ;
            break ;
        }
        case 8:
        {
            timedate->hour += number ;
            break ;
        }
        // Set minute
        case 9:
        {
            timedate->minute += number * 10 ;
            break ;
        }
        case 10:
        {
            timedate->minute += number ;
            break ;
        }
    }
}
#endif

static bool FreqCheckLimits( freq_t freq )
{
          bool      valid  = false ;
    const hwInfo_t* hwinfo = platform_getHwInfo();

    if( hwinfo->vhf_band )
    {
        // hwInfo_t frequencies are in MHz
        if( ( freq >= ( hwinfo->vhf_minFreq * 1000000 ) ) &&
            ( freq <= ( hwinfo->vhf_maxFreq * 1000000 ) )    )
        {
            valid = true ;
        }
    }
    if( hwinfo->uhf_band )
    {
        // hwInfo_t frequencies are in MHz
        if( ( freq >= ( hwinfo->uhf_minFreq * 1000000 ) ) &&
            ( freq <= ( hwinfo->uhf_maxFreq * 1000000 ) )    )
        {
            valid = true ;
        }
    }
    return valid ;
}

static bool _ui_channel_valid( channel_t* channel )
{
    bool valid = false ;

    if( ( FreqCheckLimits( channel->rx_frequency ) ) &&
        ( FreqCheckLimits( channel->tx_frequency ) )    )
    {
       valid = true ;
    }

    return valid ;
}

bool _ui_drawDarkOverlay( void )
{
    Color_st color_agg ;
    ui_ColorLoad( &color_agg , COLOR_AGG );
    Pos_st origin     = { 0 , 0 };

    gfx_drawRect( origin , SCREEN_WIDTH , SCREEN_HEIGHT , color_agg , true );

    return true;

}

static int FSM_LoadChannel( int16_t channel_index , bool* sync_rtx )
{
    channel_t channel ;
    int32_t   selected_channel = channel_index ;
    int       result ;

    // If a bank is active, get index from current bank
    if( state.bank_enabled )
    {
        bankHdr_t bank = { 0 };
        cps_readBankHeader( &bank, state.bank );
        if( (channel_index < 0 ) || ( channel_index >= bank.ch_count ) )
        {
            return -1 ;
        }
        channel_index = cps_readBankData( state.bank , channel_index );
    }

    result = cps_readChannel( &channel , channel_index );

    // Read successful and channel is valid
    if( ( result != -1 ) && _ui_channel_valid( &channel ) )
    {
        // Set new channel index
        state.channel_index = selected_channel ;
        // Copy channel read to state
        state.channel       = channel ;
        *sync_rtx           = true;
    }

    return result ;
}

static int _ui_fsm_loadContact( int16_t contact_index , bool* sync_rtx )
{
    contact_t contact ;
    int       result = 0 ;

    result = cps_readContact( &contact , contact_index );

    // Read successful and contact is valid
    if( result != -1 )
    {
        // Set new contact index
        state.contact_index = contact_index ;
        // Copy contact read to state
        state.contact       = contact ;
        *sync_rtx           = true;
    }

    return result ;
}

static bool ui_updateFSM_PAGE_MODE_VFO( GuiState_st* guiState )
{
    return ui_updateFSM_PAGE_MENU_TOP( guiState );
}

static bool ui_updateFSM_PAGE_MODE_MEM( GuiState_st* guiState )
{
    return ui_updateFSM_PAGE_MENU_TOP( guiState );
}

static bool ui_updateFSM_PAGE_MENU_TOP( GuiState_st* guiState )
{
    return ui_States_HandleKeyEvent( guiState );
}

static bool ui_updateFSM_PAGE_MENU_BANK( GuiState_st* guiState )
{
    return ui_updateFSM_PAGE_MENU_CONTACTS( guiState );
}

static bool ui_updateFSM_PAGE_MENU_CHANNEL( GuiState_st* guiState )
{
    return ui_updateFSM_PAGE_MENU_CONTACTS( guiState );
}

static bool ui_updateFSM_PAGE_MENU_CONTACTS( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        // disable menu wrap around
        ui_States_MenuUpNoWrapAround( guiState );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        if( guiState->page.num == PAGE_MENU_BANK )
        {
            bankHdr_t bank;
            // manu_selected is 0-based
            // bank 0 means "All Channel" mode
            // banks (1, n) are mapped to banks (0, n-1)
            if( cps_readBankHeader( &bank , guiState->uiState.menu_selected ) != -1 )
            {
                guiState->uiState.menu_selected += 1 ;
            }
        }
        else if( guiState->page.num == PAGE_MENU_CHANNEL )
        {
            channel_t channel ;
            if( cps_readChannel( &channel , guiState->uiState.menu_selected + 1 ) != -1 )
            {
                guiState->uiState.menu_selected += 1 ;
            }
        }
        else if( guiState->page.num == PAGE_MENU_CONTACTS )
        {
            contact_t contact ;
            if( cps_readContact( &contact , guiState->uiState.menu_selected + 1 ) != -1 )
            {
                guiState->uiState.menu_selected += 1 ;
            }
        }
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        switch( guiState->page.num )
        {
            case PAGE_MENU_BANK :
            {
                bankHdr_t newbank ;
                int       result  = 0 ;
                // If "All channels" is selected, load default bank
                if( guiState->uiState.menu_selected == 0 )
                {
                    state.bank_enabled = false ;
                }
                else
                {
                    state.bank_enabled = true;
                    result = cps_readBankHeader( &newbank , guiState->uiState.menu_selected - 1 );
                }
                if( result != -1 )
                {
                    state.bank = guiState->uiState.menu_selected - 1 ;
                    // If we were in VFO mode, save VFO channel
                    if( guiState->page.num == PAGE_MAIN_VFO )
                    {
                        state.vfo_channel = state.channel ;
                    }
                    // Load bank first channel
                    FSM_LoadChannel( 0 , &sync_rtx );
                    // Switch to MEM screen
                    ui_States_SetPageNum( guiState , PAGE_MAIN_MEM );
                }
                break ;
            }
            case PAGE_MENU_CHANNEL :
            {
                // If we were in VFO mode, save VFO channel
                if( guiState->page.num == PAGE_MAIN_VFO )
                {
                    state.vfo_channel = state.channel;
                }
                FSM_LoadChannel( guiState->uiState.menu_selected , &sync_rtx );
                // Switch to MEM screen
                ui_States_SetPageNum( guiState , PAGE_MAIN_MEM );
                break ;
            }
            case PAGE_MENU_CONTACTS :
            {
                _ui_fsm_loadContact( guiState->uiState.menu_selected , &sync_rtx );
                // Switch to MEM screen
                ui_States_SetPageNum( guiState , PAGE_MAIN_MEM );
                break ;
            }
        }
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        ui_States_MenuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_GPS( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( ( guiState->msg.keys & KEY_F1 ) && ( state.settings.vpLevel > VPP_BEEP ) )
    {
        // quick press repeat vp, long press summary.
        if( guiState->msg.long_press )
        {
            vp_announceGPSInfo( VPGPS_ALL );
        }
        else
        {
            vp_replayLastPrompt();
        }
        guiState->f1Handled = true;
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        ui_States_MenuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_SETTINGS( GuiState_st* guiState )
{
    (void)guiState ;

    bool sync_rtx = false ;

    ui_States_HandleKeyEvent( guiState );

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_BACKUP_RESTORE( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    ui_States_HandleKeyEvent( guiState );

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_BACKUP( GuiState_st* guiState )
{
    return - ui_updateFSM_PAGE_MENU_RESTORE( guiState );
}

static bool ui_updateFSM_PAGE_MENU_RESTORE( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ESC )
    {
        ui_States_MenuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_INFO( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        ui_States_MenuUp( guiState );
    }
    else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
    {
        ui_States_MenuDown( guiState );
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        ui_States_MenuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_MENU_ABOUT( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ESC )
    {
        ui_States_MenuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ENTER )
    {
        // Switch to set Time&Date mode
        ui_States_SetPageNum( guiState , PAGE_SETTINGS_TIMEDATE_SET );
        // Reset input position and selection
        guiState->uiState.input_position = 0 ;
        memset( &guiState->uiState.new_timedate , 0 , sizeof( datetime_t ) );
        vp_announceBuffer( &currentLanguage->timeAndDate , true , false , "dd/mm/yy" );
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        ui_States_MenuBack( guiState );
    }

    return sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_TIMEDATE_SET( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    if( guiState->msg.keys & KEY_ENTER )
    {
        // Save time only if all digits have been inserted
        if( guiState->uiState.input_position >= TIMEDATE_DIGITS )
        {
            // Return to Time&Date menu, saving values
            // NOTE: The user inserted a local time, we must save an UTC time
            datetime_t utc_time = localTimeToUtc( guiState->uiState.new_timedate , state.settings.utc_timezone );
            platform_setTime( utc_time );
            state.time          = utc_time ;
            vp_announceSettingsTimeDate();
            ui_States_SetPageNum( guiState , PAGE_SETTINGS_TIMEDATE_SET );
        }
    }
    else if( guiState->msg.keys & KEY_ESC )
    {
        ui_States_MenuBack( guiState );
    }
    else if( input_isNumberPressed( guiState->msg ) )
    {
        // if present - discard excess digits
        if( guiState->uiState.input_position <= TIMEDATE_DIGITS )
        {
            guiState->uiState.input_position += 1;
            guiState->uiState.input_number    = input_getPressedNumber( guiState->msg );
            _ui_timedate_add_digit( &guiState->uiState.new_timedate , guiState->uiState.input_position , guiState->uiState.input_number );
        }
    }

    return sync_rtx ;

}

// D_BRIGHTNESS , D_CONTRAST , D_TIMER
static bool ui_updateFSM_PAGE_SETTINGS_DISPLAY( GuiState_st* guiState )
{
    ui_ValueInputFSM( guiState );

    if( !guiState->handled )
    {
        if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            ui_States_MenuUp( guiState );
        }
        else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
        {
            ui_States_MenuDown( guiState );
        }
        else if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            ui_States_MenuBack( guiState );
        }
    }

    return guiState->sync_rtx ;

}

// G_ENABLED , G_SET_TIME , G_TIMEZONE
static bool ui_updateFSM_PAGE_SETTINGS_GPS( GuiState_st* guiState )
{
    ui_ValueInputFSM( guiState );

    if( !guiState->handled )
    {
        if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            ui_States_MenuUp( guiState );
        }
        else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
        {
            ui_States_MenuDown( guiState );
        }
        else if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            ui_States_MenuBack( guiState );
        }
    }

    return guiState->sync_rtx ;

}

// R_OFFSET , R_DIRECTION , R_STEP
static bool ui_updateFSM_PAGE_SETTINGS_RADIO( GuiState_st* guiState )
{
    ui_ValueInputFSM( guiState );

    if( !guiState->handled )
    {
        if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            ui_States_MenuUp( guiState );
        }
        else if( ( guiState->msg.keys & KEY_DOWN ) || ( guiState->msg.keys & KNOB_RIGHT ) )
        {
            ui_States_MenuDown( guiState );
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            ui_States_MenuBack( guiState );
        }
    }

    return guiState->sync_rtx ;

}

// M17_CALLSIGN , M17_CAN , M17_CAN_RX
static bool ui_updateFSM_PAGE_SETTINGS_M17( GuiState_st* guiState )
{
    ui_ValueInputFSM( guiState );

    if( !guiState->handled )
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            // Enable edit mode
            guiState->uiState.edit_mode = true;
        }
        else if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            ui_States_MenuUp( guiState );
        }
        else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
        {
            ui_States_MenuDown( guiState );
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            guiState->sync_rtx = true ;
            ui_States_MenuBack( guiState );
        }
    }

    return guiState->sync_rtx ;

}

// VP_LEVEL , VP_PHONETIC
static bool ui_updateFSM_PAGE_SETTINGS_VOICE( GuiState_st* guiState )
{
    ui_ValueInputFSM( guiState );

    if( !guiState->handled )
    {
        if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            ui_States_MenuUp( guiState );
        }
        else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
        {
            ui_States_MenuDown( guiState );
        }
        else if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            ui_States_MenuBack( guiState );
        }
    }

    return guiState->sync_rtx ;

}

static bool ui_updateFSM_PAGE_SETTINGS_RESET_TO_DEFAULTS( GuiState_st* guiState )
{
    if( !guiState->uiState.edit_mode )
    {
        //require a confirmation ENTER, then another
        //edit_mode is slightly misused to allow for this
        if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = true ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            ui_States_MenuBack( guiState );
        }
    }
    else
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = false ;
            state_resetSettingsAndVfo();
            ui_States_MenuBack( guiState );
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            guiState->uiState.edit_mode = false ;
            ui_States_MenuBack( guiState );
        }
    }

    return guiState->sync_rtx ;

}

static bool ui_updateFSM_PAGE_LOW_BAT( GuiState_st* guiState )
{
    (void)guiState ;
    return false ;
}

static bool ui_updateFSM_PAGE_ABOUT( GuiState_st* guiState )
{
    (void)guiState ;
    return false ;
}

static bool ui_updateFSM_PAGE_STUBBED( GuiState_st* guiState )
{
    (void)guiState ;
    return false ;
}

static void FSM_ConfirmVFOInput( GuiState_st* guiState , bool* sync_rtx )
{
    vp_flush();

    switch( guiState->uiState.input_set )
    {
        case SET_RX :
        {
            // Switch to TX input
            guiState->uiState.input_set      = SET_TX;
            // Reset input position
            guiState->uiState.input_position = 0;
            // announce the rx frequency just confirmed with Enter.
            vp_queueFrequency( guiState->uiState.new_rx_frequency );
            // defer playing till the end.
            // indicate that the user has moved to the tx freq field.
            vp_announceInputReceiveOrTransmit( true , VPQ_DEFAULT );
            break ;
        }
        case SET_TX :
        {
            // Save new frequency setting
            // If TX frequency was not set, TX = RX
            if( guiState->uiState.new_tx_frequency == 0 )
            {
                guiState->uiState.new_tx_frequency = guiState->uiState.new_rx_frequency ;
            }
            // Apply new frequencies if they are valid
            if( FreqCheckLimits( guiState->uiState.new_rx_frequency ) &&
                FreqCheckLimits(guiState->uiState.new_tx_frequency  )    )
            {
                state.channel.rx_frequency = guiState->uiState.new_rx_frequency ;
                state.channel.tx_frequency = guiState->uiState.new_tx_frequency ;
                *sync_rtx                  = true ;
                // force init to clear any prompts in progress.
                // defer play because play is called at the end of the function
                //due to above freq queuing.
                vp_announceFrequencies( state.channel.rx_frequency ,
                                        state.channel.tx_frequency , VPQ_INIT );
            }
            else
            {
                vp_announceError( VPQ_INIT );
            }

            ui_States_SetPageNum( guiState , PAGE_MAIN_VFO );
            break ;
        }
    }

    vp_play();
}

static void FSM_InsertVFONumber( GuiState_st* guiState , kbd_msg_t msg , bool* sync_rtx )
{
    // Advance input position
    guiState->uiState.input_position += 1 ;
    // clear any prompts in progress.
    vp_flush() ;
    // Save pressed number to calculate frequency and show in GUI
    guiState->uiState.input_number = input_getPressedNumber( msg );
    // queue the digit just pressed.
    vp_queueInteger( guiState->uiState.input_number );
    // queue  point if user has entered three digits.
    if( guiState->uiState.input_position == 3 )
    {
        vp_queuePrompt( PROMPT_POINT );
    }

    switch( guiState->uiState.input_set )
    {
        case SET_RX :
        {
            if( guiState->uiState.input_position == 1 )
            {
                guiState->uiState.new_rx_frequency = 0 ;
            }
            // Calculate portion of the new RX frequency
            guiState->uiState.new_rx_frequency = FreqAddDigit( guiState->uiState.new_rx_frequency ,
                                                                     guiState->uiState.input_position   ,
                                                                     guiState->uiState.input_number       );
            if( guiState->uiState.input_position >= FREQ_DIGITS )
            {
                // queue the rx freq just completed.
                vp_queueFrequency( guiState->uiState.new_rx_frequency );
                /// now queue tx as user has changed fields.
                vp_queuePrompt( PROMPT_TRANSMIT );
                // Switch to TX input
                guiState->uiState.input_set        = SET_TX ;
                // Reset input position
                guiState->uiState.input_position   = 0 ;
                // Reset TX frequency
                guiState->uiState.new_tx_frequency = 0 ;
            }
            break ;
        }
        case SET_TX :
        {
            if( guiState->uiState.input_position == 1 )
            {
                guiState->uiState.new_tx_frequency = 0 ;
            }
            // Calculate portion of the new TX frequency
            guiState->uiState.new_tx_frequency = FreqAddDigit( guiState->uiState.new_tx_frequency ,
                                                                     guiState->uiState.input_position   ,
                                                                     guiState->uiState.input_number       );
            if( guiState->uiState.input_position >= FREQ_DIGITS )
            {
                // Save both inserted frequencies
                if( FreqCheckLimits( guiState->uiState.new_rx_frequency ) &&
                    FreqCheckLimits( guiState->uiState.new_tx_frequency )    )
                {
                    state.channel.rx_frequency = guiState->uiState.new_rx_frequency ;
                    state.channel.tx_frequency = guiState->uiState.new_tx_frequency ;
                    *sync_rtx                  = true;
                    // play is called at end.
                    vp_announceFrequencies( state.channel.rx_frequency ,
                                            state.channel.tx_frequency , VPQ_INIT );
                }

                ui_States_SetPageNum( guiState , PAGE_MAIN_VFO );
            }
            break ;
        }
    }

    vp_play();
}

#ifdef SCREEN_BRIGHTNESS
static void _ui_changeBrightness( int variation )
{
    state.settings.brightness += variation ;

    // Max value for brightness is 100, min value is set to 5 to avoid complete
    //  display shutdown.
    if( state.settings.brightness > 100 )
    {
        state.settings.brightness = 100 ;
    }
    else if( state.settings.brightness < 5 )
    {
        state.settings.brightness = 5 ;
    }

    display_setBacklightLevel( state.settings.brightness );
}
#endif // SCREEN_BRIGHTNESS

#ifdef SCREEN_CONTRAST
static void _ui_changeContrast( int variation )
{
    if( variation >= 0 )
    {
        state.settings.contrast =
        (255 - state.settings.contrast < variation) ? 255 : state.settings.contrast + variation ;
    }
    else
    {
        state.settings.contrast =
        (state.settings.contrast < -variation) ? 0 : state.settings.contrast + variation ;
    }

    display_setContrast( state.settings.contrast );
}
#endif // SCREEN_CONTRAST

void _ui_changeTimer( int variation )
{
    if( !( ( ( state.settings.display_timer == TIMER_OFF ) && ( variation < 0 ) ) ||
           ( ( state.settings.display_timer == TIMER_1H  ) && ( variation > 0 ) )    ) )
    {
        state.settings.display_timer += variation ;
    }
}

void _ui_changeM17Can( int variation )
{
    uint8_t can = state.settings.m17_can ;

    state.settings.m17_can = ( can + variation ) % 16 ;

}

void _ui_changeVoiceLevel( int variation )
{
    if( ( ( state.settings.vpLevel == VPP_NONE ) && ( variation < 0 ) ) ||
        ( ( state.settings.vpLevel == VPP_HIGH ) && ( variation > 0 ) )    )
    {
        return ;
    }

    state.settings.vpLevel += variation ;

    // Force these flags to ensure the changes are spoken for levels 1 through 3.
    VPQueueFlags_en flags = VPQ_INIT                   |
                            VPQ_ADD_SEPARATING_SILENCE |
                            VPQ_PLAY_IMMEDIATELY         ;

    if( !vp_isPlaying() )
    {
        flags |= VPQ_INCLUDE_DESCRIPTIONS ;
    }

    vp_announceSettingsVoiceLevel( flags );
}

void _ui_changePhoneticSpell( bool newVal )
{
    state.settings.vpPhoneticSpell = newVal ? 1 : 0 ;

    vp_announceSettingsOnOffToggle( &currentLanguage->phonetic      ,
                                     vp_getVoiceLevelQueueFlags()   ,
                                     state.settings.vpPhoneticSpell   );
}

static bool CheckStandby( long long time_since_last_event )
{
    bool result = false ;

    if( !standby )
    {
        switch( state.settings.display_timer )
        {
            case TIMER_OFF :
            {
                break ;
            }
            case TIMER_5S :
            case TIMER_10S :
            case TIMER_15S :
            case TIMER_20S :
            case TIMER_25S :
            case TIMER_30S :
            {
                result = time_since_last_event >= ( 5000 * state.settings.display_timer );
                break ;
            }
            case TIMER_1M :
            case TIMER_2M :
            case TIMER_3M :
            case TIMER_4M :
            case TIMER_5M :
            {
                result = time_since_last_event >=
                         ( 60000 * ( state.settings.display_timer - ( TIMER_1M - 1 ) ) );
                break ;
            }
            case TIMER_15M :
            case TIMER_30M :
            case TIMER_45M :
            {
                result = time_since_last_event >=
                         ( 60000 * 15 * ( state.settings.display_timer - ( TIMER_15M - 1 ) ) );
                break ;
            }
            case TIMER_1H :
            {
                result = time_since_last_event >= 60 * 60 * 1000 ;
                break ;
            }
        }
    }

    return result ;
}

static void FSM_MenuMacro( GuiState_st* guiState , kbd_msg_t msg , bool* sync_rtx )
{
    // If there is no keyboard left and right select the menu entry to edit
#ifdef UI_NO_KEYBOARD

    switch( msg.keys )
    {
        case KNOB_LEFT :
        {
            guiState->uiState.macro_menu_selected-- ;
            guiState->uiState.macro_menu_selected += 9 ;
            guiState->uiState.macro_menu_selected %= 9 ;
            break ;
        }
        case KNOB_RIGHT :
        {
            guiState->uiState.macro_menu_selected++ ;
            guiState->uiState.macro_menu_selected %= 9 ;
            break ;
        }
        case KEY_ENTER :
        {
            if( !msg.long_press )
            {
                guiState->uiState.input_number = guiState->uiState.macro_menu_selected + 1 ;
            }
            else
            {
                guiState->uiState.input_number = 0 ;
            }
            break ;
        }
        default :
        {
            guiState->uiState.input_number = 0 ;
            break ;
        }
    }
#else // UI_NO_KEYBOARD
    guiState->uiState.input_number      = input_getPressedNumber( msg );
#endif // UI_NO_KEYBOARD
    // CTCSS Encode/Decode Selection
    bool tone_tx_enable        = state.channel.fm.txToneEn ;
    bool tone_rx_enable        = state.channel.fm.rxToneEn ;
    uint8_t tone_flags         = ( tone_tx_enable << 1 ) | tone_rx_enable ;
    VPQueueFlags_en queueFlags = vp_getVoiceLevelQueueFlags();

    switch( guiState->uiState.input_number )
    {
        case KEY_NUM_1 :
        {
            if( state.channel.mode == OPMODE_FM )
            {
                if( state.channel.fm.txTone == 0 )
                {
                    state.channel.fm.txTone = MAX_TONE_INDEX - 1 ;
                }
                else
                {
                    state.channel.fm.txTone--;
                }

                state.channel.fm.txTone %= MAX_TONE_INDEX ;
                state.channel.fm.rxTone  = state.channel.fm.txTone ;
                *sync_rtx                = true ;
                vp_announceCTCSS( state.channel.fm.rxToneEn ,
                                  state.channel.fm.rxTone   ,
                                  state.channel.fm.txToneEn ,
                                  state.channel.fm.txTone   ,
                                  queueFlags                  );
            }
            break;
        }
        case KEY_NUM_2 :
        {
            if( state.channel.mode == OPMODE_FM )
            {
                state.channel.fm.txTone++ ;
                state.channel.fm.txTone %= MAX_TONE_INDEX ;
                state.channel.fm.rxTone  = state.channel.fm.txTone ;
                *sync_rtx                = true ;
                vp_announceCTCSS( state.channel.fm.rxToneEn ,
                                  state.channel.fm.rxTone   ,
                                  state.channel.fm.txToneEn ,
                                  state.channel.fm.txTone   ,
                                  queueFlags                  );
            }
            break ;
        }
        case KEY_NUM_3 :
        {
            if( state.channel.mode == OPMODE_FM )
            {
                tone_flags++;
                tone_flags                %= 4 ;
                tone_tx_enable             = tone_flags >> 1 ;
                tone_rx_enable             = tone_flags & 1 ;
                state.channel.fm.txToneEn  = tone_tx_enable ;
                state.channel.fm.rxToneEn  = tone_rx_enable ;
                *sync_rtx                  = true ;
                vp_announceCTCSS( state.channel.fm.rxToneEn ,
                                  state.channel.fm.rxTone   ,
                                  state.channel.fm.txToneEn ,
                                  state.channel.fm.txTone   ,
                                  queueFlags |VPQ_INCLUDE_DESCRIPTIONS );
            }
            break ;
        }
        case KEY_NUM_4 :
        {
            if( state.channel.mode == OPMODE_FM )
            {
                state.channel.bandwidth++ ;
                state.channel.bandwidth %= 3 ;
                *sync_rtx                = true ;
                vp_announceBandwidth( state.channel.bandwidth , queueFlags );
            }
            break ;
        }
        case KEY_NUM_5 :
        {
            // Cycle through radio modes
            if( state.channel.mode == OPMODE_FM )
            {
                state.channel.mode = OPMODE_M17 ;
            }
            else if( state.channel.mode == OPMODE_M17 )
            {
                state.channel.mode = OPMODE_FM;
            }
            else
            {
                //catch any invalid states so they don't get locked out
                state.channel.mode = OPMODE_FM ;
            }
            *sync_rtx = true ;
            vp_announceRadioMode( state.channel.mode, queueFlags );
            break ;
        }
        case KEY_NUM_6 :
        {
            float power ;
            if( state.channel.power == 100 )
            {
                state.channel.power = 135 ;
            }
            else
            {
                state.channel.power = 100 ;
            }
            *sync_rtx = true ;
            power     = dBmToWatt( state.channel.power );
            vp_anouncePower( power , queueFlags );
            break ;
        }
#ifdef SCREEN_BRIGHTNESS
        case KEY_NUM_7 :
        {
            _ui_changeBrightness( -5 );
            vp_announceSettingsInt( &currentLanguage->brightness , queueFlags ,
                                     state.settings.brightness                  );
            break ;
        }
        case KEY_NUM_8 :
        {
            _ui_changeBrightness( +5 );
            vp_announceSettingsInt( &currentLanguage->brightness , queueFlags ,
                                     state.settings.brightness                  );
            break ;
        }
#endif // SCREEN_BRIGHTNESS
        case KEY_NUM_9 :
        {
            if( !guiState->uiState.input_locked )
            {
                guiState->uiState.input_locked = true ;
            }
            else
            {
                guiState->uiState.input_locked = false ;
            }
            break ;
        }
        default :
        {
            break ;
        }
    }

#ifdef PLATFORM_TTWRPLUS
    if( msg.keys & KEY_VOLDOWN )
#else // PLATFORM_TTWRPLUS
    if( msg.keys & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
#endif // PLATFORM_TTWRPLUS
    {
#ifdef HAS_ABSOLUTE_KNOB // If the radio has an absolute position knob
        state.settings.sqlLevel = platform_getChSelector() - 1 ;
#endif // HAS_ABSOLUTE_KNOB
        if(state.settings.sqlLevel > 0)
        {
            state.settings.sqlLevel -= 1 ;
            *sync_rtx                = true ;
            vp_announceSquelch( state.settings.sqlLevel , queueFlags );
        }
    }

#ifdef PLATFORM_TTWRPLUS
    else if( msg.keys & KEY_VOLUP )
#else // PLATFORM_TTWRPLUS
    else if( msg.keys & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
#endif // PLATFORM_TTWRPLUS
    {
#ifdef HAS_ABSOLUTE_KNOB
        state.settings.sqlLevel = platform_getChSelector() - 1 ;
#endif // HAS_ABSOLUTE_KNOB
        if(state.settings.sqlLevel < 15)
        {
            state.settings.sqlLevel += 1 ;
            *sync_rtx                = true;
            vp_announceSquelch( state.settings.sqlLevel , queueFlags );
        }
    }
}

void ui_States_TextInputReset( GuiState_st* guiState , char* buf )
{
    guiState->uiState.input_number   = 0 ;
    guiState->uiState.input_position = 0 ;
    guiState->uiState.input_set      = 0 ;
    guiState->uiState.last_keypress  = 0 ;
    memset( buf , 0 , 9 );
    buf[ 0 ]                = '_';
}

void ui_States_TextInputKeypad( GuiState_st* guiState , char* buf , uint8_t max_len , kbd_msg_t msg , bool callsign )
{
    if( guiState->uiState.input_position < max_len )
    {
        long long now         = getTick();
        // Get currently pressed number key
        uint8_t   num_key     = input_getPressedNumber( msg );
        // Get number of symbols related to currently pressed key
        uint8_t   num_symbols = 0 ;

        if( callsign )
        {
            num_symbols = strlen( symbols_ITU_T_E161_callsign[ num_key ] );
        }
        else
        {
            num_symbols = strlen( symbols_ITU_T_E161[ num_key ] );
        }

        // Skip keypad logic for first keypress
        if( guiState->uiState.last_keypress != 0 )
        {
            // Same key pressed and timeout not expired: cycle over chars of current key
            if( ( guiState->uiState.input_number == num_key                          ) &&
                ( ( now - guiState->uiState.last_keypress ) < input_longPressTimeout )    )
            {
                guiState->uiState.input_set = ( guiState->uiState.input_set + 1 ) % num_symbols ;
            }
            // Different key pressed: save current char and change key
            else
            {
                guiState->uiState.input_position += 1 ;
                guiState->uiState.input_set       = 0 ;
            }
        }
        // Show current character on buffer
        if( callsign )
        {
            buf[ guiState->uiState.input_position ] = symbols_ITU_T_E161_callsign[ num_key ][ guiState->uiState.input_set ];
        }
        else
        {
            buf[ guiState->uiState.input_position ] = symbols_ITU_T_E161[ num_key ][ guiState->uiState.input_set ];
        }
        // Announce the character
        vp_announceInputChar( buf[ guiState->uiState.input_position ] );
        // Update reference values
        guiState->uiState.input_number  = num_key ;
        guiState->uiState.last_keypress = now ;
    }
}

void ui_States_TextInputConfirm( GuiState_st* guiState , char* buf )
{
    buf[ guiState->uiState.input_position + 1 ] = '\0' ;
}

void ui_States_TextInputDelete( GuiState_st* guiState , char* buf )
{
    // announce the char about to be backspaced.
    // Note this assumes editing callsign.
    // If we edit a different buffer which allows the underline char, we may
    // not want to exclude it, but when editing callsign, we do not want to say
    // underline since it means the field is empty.
    if( buf[ guiState->uiState.input_position ] &&
        buf[ guiState->uiState.input_position ] != '_' )
    {
        vp_announceInputChar( buf[ guiState->uiState.input_position ] );
    }

    buf[ guiState->uiState.input_position ] = '\0' ;
    // Move back input cursor
    if( guiState->uiState.input_position > 0 )
    {
        guiState->uiState.input_position--;
    // If we deleted the initial character, reset starting condition
    }
    else
    {
        guiState->uiState.last_keypress = 0 ;
    }
    guiState->uiState.input_set = 0 ;
}

void _ui_numberInputKeypad( GuiState_st* guiState , uint32_t* num , kbd_msg_t msg )
{
    long long now = getTick();

#ifdef UI_NO_KEYBOARD
    // If knob is turned, increment or Decrement
    if( guiState->msg.keys & KNOB_LEFT )
    {
        *num = *num + 1 ;
        if( *num % 10 == 0 )
        {
            *num = *num - 10 ;
        }
    }

    if( guiState->msg.keys & KNOB_RIGHT )
    {
        if( *num == 0 )
        {
            *num = 9 ;
        }
        else
        {
            *num = *num - 1 ;
            if( ( *num % 10 ) == 9 )
            {
                *num = *num + 10;
            }
        }
    }

    // If enter is pressed, advance to the next digit
    if( guiState->msg.keys & KEY_ENTER )
    {
        *num *= 10 ;
    }
    // Announce the character
    vp_announceInputChar( '0' + ( *num % 10 ) );

    // Update reference values
    guiState->uiState.input_number = *num % 10 ;
#else // UI_NO_KEYBOARD
    // Maximum frequency len is uint32_t max value number of decimal digits
    if( guiState->uiState.input_position >= 10 )
    {
        return;
    }
    // Get currently pressed number key
    uint8_t num_key = input_getPressedNumber( msg );
    *num *= 10 ;
    *num += num_key ;

    // Announce the character
    vp_announceInputChar( '0' + num_key );

    // Update reference values
    guiState->uiState.input_number  = num_key;
#endif // UI_NO_KEYBOARD

    guiState->uiState.last_keypress = now;
}

void _ui_numberInputDel( GuiState_st* guiState , uint32_t* num )
{
    // announce the digit about to be backspaced.
    vp_announceInputChar( '0' + ( *num % 10 ) );

    // Move back input cursor
    if( guiState->uiState.input_position > 0 )
    {
        guiState->uiState.input_position-- ;
    }
    else
    {
        guiState->uiState.last_keypress = 0 ;
    }
    guiState->uiState.input_set = 0 ;
}

/***************************************************************************************************
;   Interfield Movement Handling
***************************************************************************************************/

static bool ui_States_HandleKeyEvent( GuiState_st* guiState )
{
    (void)guiState ;
    bool sync_rtx = false ;

    if( ui_States_IsEntryPage( guiState ) )
    {
        if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
        {
            ui_States_MenuUp( guiState );
        }
        else if( guiState->msg.keys & ( KEY_DOWN | KNOB_RIGHT ) )
        {
            ui_States_MenuDown( guiState );
        }
        else if( guiState->msg.keys & KEY_ENTER )
        {
            ui_States_SelectPage( guiState );
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            ui_States_MenuBack( guiState );
        }
    }

    return sync_rtx ;

}

void ui_States_MenuUp( GuiState_st* guiState )
{
    uint8_t numOfEntries = ui_States_GetPageNumOfEntries( guiState );

    if( guiState->uiState.menu_selected > 0 )
    {
        guiState->uiState.menu_selected -= 1 ;
    }
    else
    {
        guiState->uiState.menu_selected = numOfEntries - 1 ;
    }
    vp_playMenuBeepIfNeeded( guiState->uiState.menu_selected == 0 );
}

static void ui_States_MenuUpNoWrapAround( GuiState_st* guiState )
{
    if( guiState->uiState.menu_selected > 0 )
    {
        guiState->uiState.menu_selected -= 1 ;
    }
    else
    {
        guiState->uiState.menu_selected = 0 ;
    }
    vp_playMenuBeepIfNeeded( guiState->uiState.menu_selected == 0 );
}


void ui_States_MenuDown( GuiState_st* guiState )
{
    uint8_t numOfEntries = ui_States_GetPageNumOfEntries( guiState );

    if( guiState->uiState.menu_selected < ( numOfEntries - 1 ) )
    {
        guiState->uiState.menu_selected += 1 ;
    }
    else
    {
        guiState->uiState.menu_selected = 0 ;
    }
    vp_playMenuBeepIfNeeded( guiState->uiState.menu_selected == 0 );
}

void ui_States_MenuBack( GuiState_st* guiState )
{
    guiState->uiState.edit_mode = false ;

    if( guiState->page.level > 1 )
    {
        guiState->page.level-- ;
        // Return to previous menu
        ui_States_SetPageNum( guiState , guiState->page.levelList[ guiState->page.level - 1 ] );
        // reset menu selection - sets the menu cursor to the top of the screen
        guiState->uiState.menu_selected = 0 ;
    }
    else
    {
        ui_States_SelectPageNum( guiState , PAGE_INITIAL );
    }

    vp_playMenuBeepIfNeeded( true );

}

void ui_States_SelectPage( GuiState_st* guiState )
{
//@@@KL change .links to .entries
    uint8_t pageNum = guiState->layout.links[ guiState->uiState.menu_selected ].num ;

    ui_States_SelectPageNum( guiState , pageNum );
}

void ui_States_SelectPageNum( GuiState_st* guiState , uint8_t pageNum )
{
    ui_States_SetPageNum( guiState , pageNum );
    // reset menu selection - sets the menu cursor to the top of the screen
    guiState->uiState.menu_selected = 0 ;
}

void ui_States_SetPageNum( GuiState_st* guiState , uint8_t pageNum )
{
    uint8_t pgNum = pageNum ;

    if( pgNum >= PAGE_NUM_OF )
    {
        pgNum = PAGE_STUBBED ;
    }

    guiState->page.num = pgNum ;
    state.ui_page      = pgNum ;
}

static bool ui_States_IsEntryPage( GuiState_st* guiState )
{
    uint8_t* ptr ;
    uint16_t index ;
    uint8_t  isEntryPage = false ;

    ptr = (uint8_t*)uiPageTable[ guiState->page.num ] ;

    for( index = 0 ; ptr[ index ] != GUI_CMD_PAGE_END ; index++ )
    {
        if( ( ptr[ index ] == GUI_CMD_LINK      ) ||
            ( ptr[ index ] == GUI_CMD_VALUE_INP )    )
        {
            isEntryPage = true ;
            break ;
        }
    }

    return isEntryPage ;
}

uint8_t ui_States_GetPageNumOfEntries( GuiState_st* guiState )
//static uint8_t ui_States_GetPageNumOfEntries( GuiState_st* guiState )
{
    uint8_t* ptr ;
    uint16_t index ;
    uint8_t  numOf ;

    ptr = (uint8_t*)uiPageTable[ guiState->page.num ] ;

    for( numOf = 0 , index = 0 ; ptr[ index ] != GUI_CMD_PAGE_END ; index++ )
    {
        if( ( ptr[ index ] == GUI_CMD_LINK      ) ||
            ( ptr[ index ] == GUI_CMD_VALUE_INP )    )
        {
            numOf++ ;
        }
    }

    return numOf ;
}
