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

static void ui_ValueInput_VFOMiddleInput( GuiState_st* guiState );
#ifdef SCREEN_BRIGHTNESS
static void ui_ValueInput_BRIGHTNESS( GuiState_st* guiState );
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
static void ui_ValueInput_CONTRAST( GuiState_st* guiState );
#endif // SCREEN_CONTRAST
static void ui_ValueInput_TIMER( GuiState_st* guiState );
#ifdef GPS_PRESENT
static void ui_ValueInput_ENABLED( GuiState_st* guiState );
static void ui_ValueInput_SET_TIME( GuiState_st* guiState );
static void ui_ValueInput_TIMEZONE( GuiState_st* guiState );
#endif // GPS_PRESENT
static void ui_ValueInput_LEVEL( GuiState_st* guiState );
static void ui_ValueInput_PHONETIC( GuiState_st* guiState );
static void ui_ValueInput_OFFSET( GuiState_st* guiState );
static void ui_ValueInput_DIRECTION( GuiState_st* guiState );
static void ui_ValueInput_STEP( GuiState_st* guiState );
static void ui_ValueInput_CALLSIGN( GuiState_st* guiState );
static void ui_ValueInput_CAN( GuiState_st* guiState );
static void ui_ValueInput_CAN_RX( GuiState_st* guiState );
static void ui_ValueInput_STUBBED( GuiState_st* guiState );

typedef void (*ui_ValueInput_fn)( GuiState_st* guiState );

// GUI Values - Set
static const ui_ValueInput_fn ui_ValueInput_Table[ GUI_VAL_INP_NUM_OF ] =
{
    ui_ValueInput_VFOMiddleInput , // GUI_VAL_INP_VFO_MIDDLE_INPUT

    ui_ValueInput_STUBBED        , // GUI_VAL_INP_CURRENT_TIME
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_BATTERY_LEVEL
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_LOCK_STATE
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_MODE_INFO
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_FREQUENCY
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_RSSI_METER

    ui_ValueInput_STUBBED        , // GUI_VAL_INP_BANKS
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_CHANNELS
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_CONTACTS
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_GPS
#ifdef SCREEN_BRIGHTNESS
    ui_ValueInput_BRIGHTNESS     , // GUI_VAL_INP_BRIGHTNESS          , D_BRIGHTNESS
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
    ui_ValueInput_CONTRAST       , // GUI_VAL_INP_CONTRAST            , D_CONTRAST
#endif // SCREEN_CONTRAST
    ui_ValueInput_TIMER          , // GUI_VAL_INP_TIMER               , D_TIMER
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_DATE
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_TIME
    ui_ValueInput_ENABLED        , // GUI_VAL_INP_GPS_ENABLED         , G_ENABLED
    ui_ValueInput_SET_TIME       , // GUI_VAL_INP_GPS_SET_TIME        , G_SET_TIME
    ui_ValueInput_TIMEZONE       , // GUI_VAL_INP_GPS_TIME_ZONE       , G_TIMEZONE
    ui_ValueInput_OFFSET         , // GUI_VAL_INP_RADIO_OFFSET        , R_OFFSET
    ui_ValueInput_DIRECTION      , // GUI_VAL_INP_RADIO_DIRECTION     , R_DIRECTION
    ui_ValueInput_STEP           , // GUI_VAL_INP_RADIO_STEP          , R_STEP
    ui_ValueInput_CALLSIGN       , // GUI_VAL_INP_M17_CALLSIGN        , M17_CALLSIGN
    ui_ValueInput_CAN            , // GUI_VAL_INP_M17_CAN             , M17_CAN
    ui_ValueInput_CAN_RX         , // GUI_VAL_INP_M17_CAN_RX_CHECK    , M17_CAN_RX
    ui_ValueInput_LEVEL          , // GUI_VAL_INP_LEVEL               , VP_LEVEL
    ui_ValueInput_PHONETIC       , // GUI_VAL_INP_PHONETIC            , VP_PHONETIC
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_BATTERY_VOLTAGE
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_BATTERY_CHARGE
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_RSSI
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_USED_HEAP
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_BAND
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_VHF
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_UHF
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_HW_VERSION
#ifdef PLATFORM_TTWRPLUS
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_RADIO
    ui_ValueInput_STUBBED        , // GUI_VAL_INP_RADIO_FW
#endif // PLATFORM_TTWRPLUS
    ui_ValueInput_STUBBED          // GUI_VAL_INP_STUBBED
};

void ui_ValueInputFSM( GuiState_st* guiState )
{
    uint8_t linkSelected = guiState->uiState.menu_selected ;
    uint8_t valueNum     = guiState->layout.links[ linkSelected ].num ;

    ui_ValueInput( guiState , valueNum );

}

void ui_ValueInput( GuiState_st* guiState , uint8_t valueNum )
{
    if( valueNum >= GUI_VAL_INP_NUM_OF )
    {
        valueNum = GUI_VAL_INP_STUBBED ;
    }

    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    ui_ValueInput_Table[ valueNum ]( guiState );

}

static void ui_ValueInput_VFOMiddleInput( GuiState_st* guiState )
{
    (void)guiState;
}

#ifdef SCREEN_BRIGHTNESS
// D_BRIGHTNESS
static void ui_ValueInput_BRIGHTNESS( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    if( (   guiState->msg.keys & KEY_LEFT                                                  ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) && guiState->uiState.edit_mode )    )
    {
        _ui_changeBrightness( -5 );
        vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.brightness );
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                                                ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) && guiState->uiState.edit_mode )    )
    {
        _ui_changeBrightness( +5 );
        vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.brightness );
    }
    else
    {
        guiState->handled = false ;
    }

}
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
// D_CONTRAST
static void ui_ValueInput_CONTRAST( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    if( (   guiState->msg.keys & KEY_LEFT                      ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) &&
        guiState->uiState.edit_mode                            )    )
    {
        _ui_changeContrast( -4 );
        vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.contrast );
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                    ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) &&
             guiState->uiState.edit_mode                           )    )
    {
        _ui_changeContrast( +4 );
        vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.contrast );
    }
    else
    {
        guiState->handled = false ;
    }

}
#endif // SCREEN_CONTRAST
// D_TIMER
static void ui_ValueInput_TIMER( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    if( (   guiState->msg.keys & KEY_LEFT                      ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) &&
        guiState->uiState.edit_mode                            )    )
    {
        _ui_changeTimer( -1 );
        vp_announceDisplayTimer();
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                    ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) &&
             guiState->uiState.edit_mode                           )    )
    {
        _ui_changeTimer( +1 );
        vp_announceDisplayTimer();
    }
    else
    {
        guiState->handled = false ;
    }

}
#ifdef GPS_PRESENT
// G_ENABLED
static void ui_ValueInput_ENABLED( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    if( (   guiState->msg.keys & ( KEY_LEFT | KEY_RIGHT )                            ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT | KEY_UP | KNOB_RIGHT ) ) &&
        guiState->uiState.edit_mode                                                  )    )
    {
        if( state.settings.gps_enabled )
        {
            state.settings.gps_enabled = 0 ;
        }
        else
        {
            state.settings.gps_enabled = 1 ;
        }
        vp_announceSettingsOnOffToggle( &currentLanguage->gpsEnabled ,
                                        guiState->queueFlags , state.settings.gps_enabled );
    }
    else
    {
        guiState->handled = false ;
    }

}

// G_SET_TIME
static void ui_ValueInput_SET_TIME( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    if( (   guiState->msg.keys & ( KEY_LEFT | KEY_RIGHT )                            ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT | KEY_UP | KNOB_RIGHT ) ) &&
        guiState->uiState.edit_mode                                                  )    )
    {
        state.gps_set_time = !state.gps_set_time ;
        vp_announceSettingsOnOffToggle( &currentLanguage->gpsSetTime ,
                                        guiState->queueFlags , state.gps_set_time );
    }
    else
    {
        guiState->handled = false ;
    }

}

// G_TIMEZONE
static void ui_ValueInput_TIMEZONE( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    if( (   guiState->msg.keys & ( KEY_LEFT | KEY_RIGHT )                            ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT | KEY_UP | KNOB_RIGHT ) ) &&
        guiState->uiState.edit_mode                                                  )    )
    {
        if( guiState->msg.keys & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
        {
            state.settings.utc_timezone -= 1 ;
        }
        else if( guiState->msg.keys & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            state.settings.utc_timezone += 1 ;
        }
        vp_announceTimeZone( state.settings.utc_timezone , guiState->queueFlags );
    }
    else
    {
        guiState->handled = false ;
    }

}
#endif // GPS_PRESENT

// R_OFFSET
static void ui_ValueInput_OFFSET( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    // If the entry is selected with enter we are in edit_mode
    if( guiState->uiState.edit_mode )
    {
#ifdef UI_NO_KEYBOARD
        if( guiState->msg.long_press && ( guiState->msg.keys & KEY_ENTER ) )
        {
            // Long press on UI_NO_KEYBOARD causes digits to advance by one
            guiState->uiState.new_offset /= 10 ;
#else // UI_NO_KEYBOARD
        if( guiState->msg.keys & KEY_ENTER )
        {
#endif // UI_NO_KEYBOARD
            // Apply new offset
            state.channel.tx_frequency  = state.channel.rx_frequency + guiState->uiState.new_offset ;
            vp_queueStringTableEntry( &currentLanguage->frequencyOffset );
            vp_queueFrequency( guiState->uiState.new_offset );
            guiState->uiState.edit_mode = false ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            // Announce old frequency offset
            vp_queueStringTableEntry( &currentLanguage->frequencyOffset );
            vp_queueFrequency( (int32_t)state.channel.tx_frequency -
                               (int32_t)state.channel.rx_frequency );
        }
        else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
        {
            _ui_numberInputDel( guiState , &guiState->uiState.new_offset );
        }
#ifdef UI_NO_KEYBOARD
        else if( guiState->msg.keys & ( KNOB_LEFT | KNOB_RIGHT | KEY_ENTER ) )
#else // UI_NO_KEYBOARD
        else if( input_isNumberPressed( guiState->msg ) )
#endif // UI_NO_KEYBOARD
        {
            _ui_numberInputKeypad( guiState , &guiState->uiState.new_offset , guiState->msg );
            guiState->uiState.input_position += 1 ;
        }
        else if( guiState->msg.long_press              &&
                 ( guiState->msg.keys     & KEY_F1   ) &&
                 ( state.settings.vpLevel > VPP_BEEP )    )
        {
            vp_queueFrequency( guiState->uiState.new_offset );
            guiState->f1Handled = true ;
        }
        // If ENTER or ESC are pressed, exit edit mode, R_OFFSET is managed separately
        if( !( guiState->msg.keys & KEY_ENTER ) && ( guiState->msg.keys & KEY_ESC ) )
        {
            guiState->uiState.edit_mode = false ;
        }
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        guiState->uiState.edit_mode      = true;
        guiState->uiState.new_offset     = 0 ;
        // Reset input position
        guiState->uiState.input_position = 0 ;
    }
    else
    {
        guiState->handled = false ;
    }

}

// R_DIRECTION
static void ui_ValueInput_DIRECTION( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    // If the entry is selected with enter we are in edit_mode
    if( guiState->uiState.edit_mode )
    {
        if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KNOB_LEFT | KNOB_RIGHT ) )
        {
            // Invert frequency offset direction
            if( state.channel.tx_frequency >= state.channel.rx_frequency )
            {
                state.channel.tx_frequency -= 2 * ( (int32_t)state.channel.tx_frequency -
                                                    (int32_t)state.channel.rx_frequency );
            }
            else // Switch to positive offset
            {
                state.channel.tx_frequency -= 2 * ( (int32_t)state.channel.tx_frequency -
                                                    (int32_t)state.channel.rx_frequency );
            }
        }
        // If ENTER or ESC are pressed, exit edit mode
        if( ( guiState->msg.keys & KEY_ENTER ) ||
            ( guiState->msg.keys & KEY_ESC   )    )
        {
            guiState->uiState.edit_mode = false ;
        }
    }
    else if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        ui_States_MenuUp( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( ( guiState->msg.keys & KEY_DOWN ) || ( guiState->msg.keys & KNOB_RIGHT ) )
    {
        ui_States_MenuDown( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        guiState->uiState.edit_mode = true;
        // If we are entering R_OFFSET clear temp offset
        if( guiState->uiState.menu_selected == R_OFFSET )
        {
            guiState->uiState.new_offset = 0 ;
        }
        // Reset input position
        guiState->uiState.input_position = 0 ;
    }
    else
    {
        guiState->handled = false ;
    }

}

// R_STEP
static void ui_ValueInput_STEP( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    // If the entry is selected with enter we are in edit_mode
    if( guiState->uiState.edit_mode )
    {
        if( guiState->msg.keys & ( KEY_UP | KEY_RIGHT | KNOB_RIGHT ) )
        {
            // Cycle over the available frequency steps
            state.step_index++ ;
            state.step_index %= n_freq_steps ;
        }
        else if( guiState->msg.keys & ( KEY_DOWN | KEY_LEFT | KNOB_LEFT ) )
        {
            state.step_index += n_freq_steps ;
            state.step_index-- ;
            state.step_index %= n_freq_steps ;
        }
        // If ENTER or ESC are pressed, exit edit mode, R_OFFSET is managed separately
        if( ( ( guiState->msg.keys & KEY_ENTER ) && ( guiState->uiState.menu_selected != R_OFFSET ) ) ||
              ( guiState->msg.keys & KEY_ESC   )                                                )
        {
            guiState->uiState.edit_mode = false ;
        }
    }
    else if( guiState->msg.keys & ( KEY_UP | KNOB_LEFT ) )
    {
        ui_States_MenuUp( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( ( guiState->msg.keys & KEY_DOWN ) || ( guiState->msg.keys & KNOB_RIGHT ) )
    {
        ui_States_MenuDown( guiState , uiGetPageNumOf( guiState ) );
    }
    else if( guiState->msg.keys & KEY_ENTER )
    {
        guiState->uiState.edit_mode = true;
        // If we are entering R_OFFSET clear temp offset
        if( guiState->uiState.menu_selected == R_OFFSET )
        {
            guiState->uiState.new_offset = 0 ;
        }
        // Reset input position
        guiState->uiState.input_position = 0 ;
    }
    else
    {
        guiState->handled = false ;
    }

}

// M17_CALLSIGN
static void ui_ValueInput_CALLSIGN( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    if( guiState->uiState.edit_mode )
    {
        // Handle text input for M17 callsign
        if( guiState->msg.keys & KEY_ENTER )
        {
            ui_States_TextInputConfirm( guiState , guiState->uiState.new_callsign );
            // Save selected callsign and disable input mode
            strncpy( state.settings.callsign , guiState->uiState.new_callsign , 10 );
            guiState->uiState.edit_mode = false;
            vp_announceBuffer( &currentLanguage->callsign , false , true , state.settings.callsign );
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            // Discard selected callsign and disable input mode
            guiState->uiState.edit_mode = false ;
            vp_announceBuffer( &currentLanguage->callsign , false , true , state.settings.callsign );
        }
        else if( guiState->msg.keys & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
        {
            ui_States_TextInputDelete( guiState , guiState->uiState.new_callsign );
        }
        else if( input_isNumberPressed( guiState->msg ) )
        {
            ui_States_TextInputKeypad( guiState , guiState->uiState.new_callsign , 9 , guiState->msg , true );
        }
        else if( guiState->msg.long_press              &&
                 ( guiState->msg.keys     & KEY_F1   ) &&
                 ( state.settings.vpLevel > VPP_BEEP )    )
        {
            vp_announceBuffer( &currentLanguage->callsign , true , true , guiState->uiState.new_callsign );
            guiState->f1Handled = true ;
        }
        else
        {
            guiState->handled = false ;
        }
    }
    else
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            // Enable edit mode
            guiState->uiState.edit_mode = true;
            ui_States_TextInputReset( guiState , guiState->uiState.new_callsign );
            vp_announceBuffer( &currentLanguage->callsign , true , true , guiState->uiState.new_callsign );
        }
	    else
	    {
	        guiState->handled = false ;
    	}
    }

}

// M17_CAN
static void ui_ValueInput_CAN( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    if( guiState->uiState.edit_mode )
    {
        if( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) )
        {
            _ui_changeM17Can( -1 );
        }
        else if( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) )
        {
            _ui_changeM17Can( +1 );
        }
        else if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            guiState->uiState.edit_mode = false ;
        }
        else
        {
            guiState->handled = false ;
        }
    }
    else
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            // Enable edit mode
            guiState->uiState.edit_mode = true;
        }
        else if( guiState->msg.keys & KEY_RIGHT )
        {
            _ui_changeM17Can( +1 );
        }
        else if( guiState->msg.keys & KEY_LEFT )
        {
            _ui_changeM17Can( -1 );
        }
	    else
	    {
	        guiState->handled = false ;
	    }
    }

}

// M17_CAN_RX
static void ui_ValueInput_CAN_RX( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    if( guiState->uiState.edit_mode )
    {
        if( (   guiState->msg.keys & ( KEY_LEFT | KEY_RIGHT                       )      ) ||
            ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT | KEY_UP | KNOB_RIGHT ) ) &&
            guiState->uiState.edit_mode                                                  )    )
        {
            state.settings.m17_can_rx = !state.settings.m17_can_rx ;
        }
        else if( guiState->msg.keys & KEY_ENTER )
        {
            guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            guiState->uiState.edit_mode = false ;
        }
        else
        {
            guiState->handled = false ;
        }
    }
    else
    {
        if( guiState->msg.keys & KEY_ENTER )
        {
            // Enable edit mode
            guiState->uiState.edit_mode = true;
        }
        else if( guiState->msg.keys & KEY_ESC )
        {
            guiState->sync_rtx = true ;
            ui_States_MenuBack( guiState );
        }
        else
        {
            guiState->handled = false ;
        }
    }

}

// VP_LEVEL
static void ui_ValueInput_LEVEL( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    if( (   guiState->msg.keys & KEY_LEFT                      ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) &&
          guiState->uiState.edit_mode                          )    )
    {
        _ui_changeVoiceLevel( -1 );
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                    ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) &&
             guiState->uiState.edit_mode                           )    )
    {
        _ui_changeVoiceLevel( 1 );
    }
    else
    {
        guiState->handled = false ;
    }

}

// VP_PHONETIC
static void ui_ValueInput_PHONETIC( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;

    if( (   guiState->msg.keys & KEY_LEFT                      ) ||
        ( ( guiState->msg.keys & ( KEY_DOWN | KNOB_LEFT ) ) &&
        guiState->uiState.edit_mode                            )    )
    {
        _ui_changePhoneticSpell( false );
    }
    else if( (   guiState->msg.keys & KEY_RIGHT                    ) ||
             ( ( guiState->msg.keys & ( KEY_UP | KNOB_RIGHT ) ) &&
             guiState->uiState.edit_mode                           )    )
    {
        _ui_changePhoneticSpell( true );
    }
    else
    {
        guiState->handled = false ;
    }

}

static void ui_ValueInput_STUBBED( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;
    guiState->handled  = true ;
}
