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
//@@@KL redact guiState->uiState.edit_mode

#include "ui.h"
#include "ui_value_arrays.h"
#include "ui_scripts.h"
#include "ui_commands.h"
#include "ui_value_display.h"
#include "ui_states.h"
#include "ui_value_input.h"

static bool GuiValInp_VFOMiddleInput( GuiState_st* guiState );
#ifdef SCREEN_BRIGHTNESS
static bool GuiValInp_ScreenBrightness( GuiState_st* guiState );
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
static bool GuiValInp_ScreenContrast( GuiState_st* guiState );
#endif // SCREEN_CONTRAST
static bool GuiValInp_Timer( GuiState_st* guiState );
#ifdef GPS_PRESENT
static bool GuiValInp_GPSEnabled( GuiState_st* guiState );
static bool GuiValInp_GPSTime( GuiState_st* guiState );
static bool GuiValInp_GPSTimeZone( GuiState_st* guiState );
#endif // GPS_PRESENT
static bool GuiValInp_Level( GuiState_st* guiState );
static bool GuiValInp_Phonetic( GuiState_st* guiState );
static bool GuiValInp_Offset( GuiState_st* guiState );
static bool GuiValInp_Direction( GuiState_st* guiState );
static bool GuiValInp_Step( GuiState_st* guiState );
static bool GuiValInp_Callsign( GuiState_st* guiState );
static bool GuiValInp_M17Can( GuiState_st* guiState );
static bool GuiValInp_M17CanRx( GuiState_st* guiState );
static bool GuiValInp_Stubbed( GuiState_st* guiState );

typedef bool (*GuiValInp_fn)( GuiState_st* guiState );

// GUI Values - Set
static const GuiValInp_fn GuiValInp_Table[ GUI_VAL_INP_NUM_OF ] =
{
    GuiValInp_Stubbed           , // GUI_VAL_BATTERY_LEVEL      0x00
    GuiValInp_Stubbed           , // GUI_VAL_LOCK_STATE         0x01
    GuiValInp_Stubbed           , // GUI_VAL_MODE_INFO          0x02
    GuiValInp_Stubbed           , // GUI_VAL_BANK_CHANNEL       0x03
    GuiValInp_Stubbed           , // GUI_VAL_FREQUENCY          0x04
    GuiValInp_Stubbed           , // GUI_VAL_RSSI_METER         0x05
#ifdef GPS_PRESENT
    GuiValInp_Stubbed           , // GUI_VAL_GPS                0x06
#endif // GPS_PRESENT
    // Settings
    // Display
#ifdef SCREEN_BRIGHTNESS
    GuiValInp_ScreenBrightness  , // GUI_VAL_BRIGHTNESS         0x07
#endif
#ifdef SCREEN_CONTRAST
    GuiValInp_ScreenContrast    , // GUI_VAL_CONTRAST           0x08
#endif
    GuiValInp_Timer             , // GUI_VAL_TIMER              0x09
    // Time and Date
    GuiValInp_Stubbed           , // GUI_VAL_DATE               0x0A
    GuiValInp_Stubbed           , // GUI_VAL_TIME               0x0B
    // GPS
#ifdef GPS_PRESENT
    GuiValInp_GPSEnabled        , // GUI_VAL_GPS_ENABLED        0x0C
    GuiValInp_GPSTime           , // GUI_VAL_GPS_TIME           0x0D
    GuiValInp_GPSTimeZone       , // GUI_VAL_GPS_TIME_ZONE      0x0E
#endif // GPS_PRESENT
    // Radio
    GuiValInp_Offset            , // GUI_VAL_RADIO_OFFSET       0x0F
    GuiValInp_Direction         , // GUI_VAL_RADIO_DIRECTION    0x10
    GuiValInp_Step              , // GUI_VAL_RADIO_STEP         0x11
    // M17
    GuiValInp_Callsign          , // GUI_VAL_M17_CALLSIGN       0x12
    GuiValInp_M17Can            , // GUI_VAL_M17_CAN            0x13
    GuiValInp_M17CanRx          , // GUI_VAL_M17_CAN_RX_CHECK   0x14
    // Accessibility - Voice
    GuiValInp_Level             , // GUI_VAL_LEVEL              0x15
    GuiValInp_Phonetic          , // GUI_VAL_PHONETIC           0x16
    // Info
    GuiValInp_Stubbed           , // GUI_VAL_BATTERY_VOLTAGE    0x17
    GuiValInp_Stubbed           , // GUI_VAL_BATTERY_CHARGE     0x18
    GuiValInp_Stubbed           , // GUI_VAL_RSSI               0x19
    GuiValInp_Stubbed           , // GUI_VAL_USED_HEAP          0x1A
    GuiValInp_Stubbed           , // GUI_VAL_BAND               0x1B
    GuiValInp_Stubbed           , // GUI_VAL_VHF                0x1C
    GuiValInp_Stubbed           , // GUI_VAL_UHF                0x1D
    GuiValInp_Stubbed           , // GUI_VAL_HW_VERSION         0x1E
#ifdef PLATFORM_TTWRPLUS
    GuiValInp_Stubbed           , // GUI_VAL_RADIO              0x1F
    GuiValInp_Stubbed           , // GUI_VAL_RADIO_FW           0x20
#endif // PLATFORM_TTWRPLUS
    GuiValInp_Stubbed           , // GUI_VAL_BACKUP_RESTORE     0x21
    GuiValInp_Stubbed           , // GUI_VAL_LOW_BATTERY        0x22
#ifdef ENABLE_DEBUG_MSG
  #ifndef DISPLAY_DEBUG_MSG
    GuiValInp_Stubbed           , // GUI_VAL_DEBUG_CH           0x23
    GuiValInp_Stubbed           , // GUI_VAL_DEBUG_GFX          0x24
  #else // DISPLAY_DEBUG_MSG
    GuiValInp_Stubbed           , // GUI_VAL_DEBUG_MSG          0x25
    GuiValInp_Stubbed           , // GUI_VAL_DEBUG_VALUES       0x26
  #endif // DISPLAY_DEBUG_MSG
#endif // ENABLE_DEBUG_MSG
    GuiValInp_Stubbed             // GUI_VAL_STUBBED            0x27
};

void GuiValInpFSM( GuiState_st* guiState )
{
}

bool GuiValInp_InputValue( GuiState_st* guiState )
{
    guiState->sync_rtx = false ;

    return GuiValInp_Table[ guiState->layout.vars[ guiState->layout.varIndex ].varNum ]( guiState );

}

static bool GuiValInp_VFOMiddleInput( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    Line_st*  line2       = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2      = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
//    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    // Add inserted number to string, skipping "Rx: "/"Tx: " and "."
    uint8_t insert_pos = guiState->uiState.input_position + 3;
    Color_st color_fg ;
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

    return handled ;

}

#ifdef SCREEN_BRIGHTNESS
// D_BRIGHTNESS
static bool GuiValInp_ScreenBrightness( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT )    )
        {
            _ui_changeBrightness( -5 );
            vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.brightness );
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            _ui_changeBrightness( +5 );
            vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.brightness );
            handled = true ;
        }
    }

    return handled ;

}
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
// D_CONTRAST
static bool GuiValInp_ScreenContrast( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
        {
            _ui_changeContrast( -4 );
            vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.contrast );
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            _ui_changeContrast( +4 );
            vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.contrast );
            handled = true ;
        }
    }

    return handled ;

}
#endif // SCREEN_CONTRAST
// D_TIMER
static bool GuiValInp_Timer( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT )    )
        {
            _ui_changeTimer( -1 );
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            _ui_changeTimer( +1 );
            handled = true ;
        }
    }

    return handled ;

}
#ifdef GPS_PRESENT
// G_ENABLED
static bool GuiValInp_GPSEnabled( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & ( KEY_LEFT  | KEY_DOWN | KNOB_LEFT  |
                                        KEY_RIGHT | KEY_UP   | KNOB_RIGHT   ) )
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
            handled = true ;
        }
    }

    return handled ;

}

// G_SET_TIME
static bool GuiValInp_GPSTime( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & ( KEY_LEFT  | KEY_DOWN | KNOB_LEFT  |
                                        KEY_RIGHT | KEY_UP   | KNOB_RIGHT   ) )
        {
            state.gps_set_time = !state.gps_set_time ;
            vp_announceSettingsOnOffToggle( &currentLanguage->gpsSetTime ,
                                            guiState->queueFlags , state.gps_set_time );
            handled = true ;
        }
    }

    return handled ;

}

// G_TIMEZONE
static bool GuiValInp_GPSTimeZone( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
        {
            state.settings.utc_timezone -= 1 ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            state.settings.utc_timezone += 1 ;
        }
        vp_announceTimeZone( state.settings.utc_timezone , guiState->queueFlags );
        handled = true ;
    }

    return handled ;

}
#endif // GPS_PRESENT

// R_OFFSET
static bool GuiValInp_Offset( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        // If the entry is selected with enter we are in edit_mode
        if( guiState->uiState.edit_mode )
        {
    #ifdef UI_NO_KEYBOARD
            if( guiState->msg.long_press && ( guiState->event.payload & KEY_ENTER ) )
            {
                // Long press on UI_NO_KEYBOARD causes digits to advance by one
                guiState->uiState.new_offset /= 10 ;
    #else // UI_NO_KEYBOARD
            if( guiState->event.payload & KEY_ENTER )
            {
    #endif // UI_NO_KEYBOARD
                // Apply new offset
                state.channel.tx_frequency  = state.channel.rx_frequency + guiState->uiState.new_offset ;
                vp_queueStringTableEntry( &currentLanguage->frequencyOffset );
                vp_queueFrequency( guiState->uiState.new_offset );
                guiState->uiState.edit_mode = false ;
                handled = true ;
            }
            else if( guiState->event.payload & KEY_ESC )
            {
                // Announce old frequency offset
                vp_queueStringTableEntry( &currentLanguage->frequencyOffset );
                vp_queueFrequency( (int32_t)state.channel.tx_frequency -
                                   (int32_t)state.channel.rx_frequency );
                handled = true ;
            }
            else if( guiState->event.payload & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
            {
                _ui_numberInputDel( guiState , &guiState->uiState.new_offset );
                handled = true ;
            }
    #ifdef UI_NO_KEYBOARD
            else if( guiState->event.payload & ( KNOB_LEFT | KNOB_RIGHT | KEY_ENTER ) )
    #else // UI_NO_KEYBOARD
            else if( input_isNumberPressed( guiState->msg ) )
    #endif // UI_NO_KEYBOARD
            {
                _ui_numberInputKeypad( guiState , &guiState->uiState.new_offset , guiState->msg );
                guiState->uiState.input_position += 1 ;
                handled = true ;
            }
            else if( guiState->msg.long_press              &&
                     ( guiState->event.payload     & KEY_F1   ) &&
                     ( state.settings.vpLevel > VPP_BEEP )    )
            {
                vp_queueFrequency( guiState->uiState.new_offset );
                guiState->f1Handled = true ;
            }
            // If ENTER or ESC are pressed, exit edit mode, R_OFFSET is managed separately
            if( !( guiState->event.payload & KEY_ENTER ) && ( guiState->event.payload & KEY_ESC ) )
            {
                guiState->uiState.edit_mode = false ;
                handled = true ;
            }
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            guiState->uiState.edit_mode      = true;
            guiState->uiState.new_offset     = 0 ;
            // Reset input position
            guiState->uiState.input_position = 0 ;
            handled = true ;
        }
    }

    return handled ;

}

// R_DIRECTION
static bool GuiValInp_Direction( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        // If the entry is selected with enter we are in edit_mode
        if( guiState->uiState.edit_mode )
        {
            if( guiState->event.payload & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KNOB_LEFT | KNOB_RIGHT ) )
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
                handled = true ;
            }
            // If ENTER or ESC are pressed, exit edit mode
            if( ( guiState->event.payload & KEY_ENTER ) ||
                ( guiState->event.payload & KEY_ESC   )    )
            {
                guiState->uiState.edit_mode = false ;
            }
        }
        else if( guiState->event.payload & ( KEY_UP | KNOB_LEFT ) )
        {
            ui_States_MenuUp( guiState );
            handled = true ;
        }
        else if( ( guiState->event.payload & KEY_DOWN ) || ( guiState->event.payload & KNOB_RIGHT ) )
        {
            ui_States_MenuDown( guiState );
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            guiState->uiState.edit_mode = true;
            // If we are entering R_OFFSET clear temp offset
            if( guiState->uiState.entrySelected == R_OFFSET )
            {
                guiState->uiState.new_offset = 0 ;
            }
            // Reset input position
            guiState->uiState.input_position = 0 ;
            handled = true ;
        }
    }

    return handled ;

}

// R_STEP
static bool GuiValInp_Step( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        // If the entry is selected with enter we are in edit_mode
        if( guiState->uiState.edit_mode )
        {
            if( guiState->event.payload & ( KEY_UP | KEY_RIGHT | KNOB_RIGHT ) )
            {
                // Cycle over the available frequency steps
                state.step_index++ ;
                state.step_index %= n_freq_steps ;
                handled = true ;
            }
            else if( guiState->event.payload & ( KEY_DOWN | KEY_LEFT | KNOB_LEFT ) )
            {
                state.step_index += n_freq_steps ;
                state.step_index-- ;
                state.step_index %= n_freq_steps ;
                handled = true ;
            }
            // If ENTER or ESC are pressed, exit edit mode, R_OFFSET is managed separately
            if( ( ( guiState->event.payload & KEY_ENTER ) && ( guiState->uiState.entrySelected != R_OFFSET ) ) ||
                  ( guiState->event.payload & KEY_ESC   )                                                )
            {
                guiState->uiState.edit_mode = false ;
            }
        }
        else if( guiState->event.payload & ( KEY_UP | KNOB_LEFT ) )
        {
            ui_States_MenuUp( guiState );
            handled = true ;
        }
        else if( ( guiState->event.payload & KEY_DOWN ) || ( guiState->event.payload & KNOB_RIGHT ) )
        {
            ui_States_MenuDown( guiState );
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            guiState->uiState.edit_mode = true;
            // If we are entering R_OFFSET clear temp offset
            if( guiState->uiState.entrySelected == R_OFFSET )
            {
                guiState->uiState.new_offset = 0 ;
            }
            // Reset input position
            guiState->uiState.input_position = 0 ;
            handled = true ;
        }
    }

    return handled ;

}

// M17_CALLSIGN
static bool GuiValInp_Callsign( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->uiState.edit_mode )
        {
            // Handle text input for M17 callsign
            if( guiState->event.payload & KEY_ENTER )
            {
                ui_States_TextInputConfirm( guiState , guiState->uiState.new_callsign );
                // Save selected callsign and disable input mode
                strncpy( state.settings.callsign , guiState->uiState.new_callsign , 10 );
                guiState->uiState.edit_mode = false;
                vp_announceBuffer( &currentLanguage->callsign , false , true , state.settings.callsign );
                handled = true ;
            }
            else if( guiState->event.payload & KEY_ESC )
            {
                // Discard selected callsign and disable input mode
                guiState->uiState.edit_mode = false ;
                vp_announceBuffer( &currentLanguage->callsign , false , true , state.settings.callsign );
                handled = true ;
            }
            else if( guiState->event.payload & ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT ) )
            {
                ui_States_TextInputDelete( guiState , guiState->uiState.new_callsign );
                handled = true ;
            }
            else if( input_isNumberPressed( guiState->msg ) )
            {
                ui_States_TextInputKeypad( guiState , guiState->uiState.new_callsign , 9 , guiState->msg , true );
                handled = true ;
            }
            else if( guiState->msg.long_press              &&
                     ( guiState->event.payload     & KEY_F1   ) &&
                     ( state.settings.vpLevel > VPP_BEEP )    )
            {
                vp_announceBuffer( &currentLanguage->callsign , true , true , guiState->uiState.new_callsign );
                guiState->f1Handled = true ;
                handled = true ;
            }
        }
        else
        {
            if( guiState->event.payload & KEY_ENTER )
            {
                // Enable edit mode
                guiState->uiState.edit_mode = true;
                ui_States_TextInputReset( guiState , guiState->uiState.new_callsign );
                vp_announceBuffer( &currentLanguage->callsign , true , true , guiState->uiState.new_callsign );
                handled = true ;
            }
        }
    }

    return handled ;

}

// M17_CAN
static bool GuiValInp_M17Can( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->uiState.edit_mode )
        {
            if( guiState->event.payload & ( KEY_DOWN | KNOB_LEFT ) )
            {
                _ui_changeM17Can( -1 );
                handled = true ;
            }
            else if( guiState->event.payload & ( KEY_UP | KNOB_RIGHT ) )
            {
                _ui_changeM17Can( +1 );
                handled = true ;
            }
            else if( guiState->event.payload & KEY_ENTER )
            {
                guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
                handled = true ;
            }
            else if( guiState->event.payload & KEY_ESC )
            {
                guiState->uiState.edit_mode = false ;
                handled = true ;
            }
        }
        else
        {
            if( guiState->event.payload & KEY_ENTER )
            {
                // Enable edit mode
                guiState->uiState.edit_mode = true;
                handled = true ;
            }
            else if( guiState->event.payload & KEY_RIGHT )
            {
                _ui_changeM17Can( +1 );
                handled = true ;
            }
            else if( guiState->event.payload & KEY_LEFT )
            {
                _ui_changeM17Can( -1 );
                handled = true ;
            }
        }
    }

    return handled ;

}

// M17_CAN_RX
static bool GuiValInp_M17CanRx( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->uiState.edit_mode )
        {
            if( (   guiState->event.payload & ( KEY_LEFT | KEY_RIGHT                       )      ) ||
                ( ( guiState->event.payload & ( KEY_DOWN | KNOB_LEFT | KEY_UP | KNOB_RIGHT ) ) &&
                guiState->uiState.edit_mode                                                  )    )
            {
                state.settings.m17_can_rx = !state.settings.m17_can_rx ;
                handled = true ;
            }
            else if( guiState->event.payload & KEY_ENTER )
            {
                guiState->uiState.edit_mode = !guiState->uiState.edit_mode ;
                handled = true ;
            }
            else if( guiState->event.payload & KEY_ESC )
            {
                guiState->uiState.edit_mode = false ;
                handled = true ;
            }
        }
        else
        {
            if( guiState->event.payload & KEY_ENTER )
            {
                // Enable edit mode
                guiState->uiState.edit_mode = true;
                handled = true ;
            }
            else if( guiState->event.payload & KEY_ESC )
            {
                guiState->sync_rtx = true ;
                ui_States_MenuBack( guiState );
                handled = true ;
            }
        }
    }

    return handled ;

}

// VP_LEVEL
static bool GuiValInp_Level( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT )    )
        {
            _ui_changeVoiceLevel( -1 );
            handled = true ;
        }
        else if( (   guiState->event.payload & KEY_RIGHT                    ) ||
                 ( ( guiState->event.payload & ( KEY_UP | KNOB_RIGHT ) ) &&
                 guiState->uiState.edit_mode                           )    )
        {
            _ui_changeVoiceLevel( 1 );
            handled = true ;
        }
    }

    return handled ;

}

// VP_PHONETIC
static bool GuiValInp_Phonetic( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT )    )
        {
            _ui_changePhoneticSpell( false );
            handled = true ;
        }
        else if( (   guiState->event.payload & KEY_RIGHT                    ) ||
                 ( ( guiState->event.payload & ( KEY_UP | KNOB_RIGHT ) ) &&
                 guiState->uiState.edit_mode                           )    )
        {
            _ui_changePhoneticSpell( true );
            handled = true ;
        }
    }

    return handled ;

}

static bool GuiValInp_Stubbed( GuiState_st* guiState )
{
    bool handled = false ;

    guiState->sync_rtx = false ;

    return handled ;

}
