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

#include "ui.h"
#include "ui_value_arrays.h"
#include "ui_scripts.h"
#include "ui_commands.h"
#include "ui_value_display.h"
#include "ui_states.h"
#include "ui_value_input.h"

#ifdef SCREEN_BRIGHTNESS
static bool GuiValInp_ScreenBrightness( GuiState_st* guiState );
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
static bool GuiValInp_ScreenContrast( GuiState_st* guiState );
#endif // SCREEN_CONTRAST
static bool GuiValInp_Timer( GuiState_st* guiState );
static bool GuiValInp_Date( GuiState_st* guiState );
static bool GuiValInp_Time( GuiState_st* guiState );
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
    GuiValInp_Date              , // GUI_VAL_DATE               0x0A
    GuiValInp_Time              , // GUI_VAL_TIME               0x0B
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
    (void)guiState ;
}

bool GuiValInp_InputValue( GuiState_st* guiState )
{
    (void)guiState ;
    bool handled ;

    guiState->result   = 0 ;
    guiState->sync_rtx = false ;

    handled = GuiValInp_Table[ guiState->layout.vars[ guiState->layout.varIndex ].varNum ]( guiState );
    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & KEY_ENTER )
        {
            if( handled )
            {
                guiState->result = nvm_writeSettingsAndVfo( &state.settings , &state.channel );
            }
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
            guiState->edit.settings.brightness -= 5 ;
            if( guiState->edit.brightness < 5 )
            {
                guiState->edit.settings.brightness = 5 ;
            }
            vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags ,
                                    guiState->edit.settings.brightness );
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            guiState->edit.settings.brightness += 5 ;
            if( guiState->edit.settings.brightness > 100 )
            {
                guiState->edit.settings.brightness = 100 ;
            }
            vp_announceSettingsInt( &currentLanguage->brightness , guiState->queueFlags , state.settings.brightness );
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            _ui_changeBrightness( guiState->edit.settings.brightness - state.settings.brightness );
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
            if( guiState->edit.contrast < 4 )
            {
                guiState->edit.settings.contrast = 0 ;
            }
            else
            {
                guiState->edit.settings.contrast -= 4 ;
            }
            vp_announceSettingsInt( &currentLanguage->contrast , guiState->queueFlags , state.settings.contrast );
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            if( guiState->edit.contrast > ( 255 - 4 ) )
            {
                guiState->edit.settings.contrast = 255 ;
            }
            else
            {
                guiState->edit.settings.contrast += 4 ;
            }
            vp_announceSettingsInt( &currentLanguage->contrast , guiState->queueFlags , state.settings.contrast );
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            _ui_changeContrast( guiState->edit.settings.contrast - state.settings.contrast );
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
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
        {
            if( guiState->edit.settings.display_timer > TIMER_OFF )
            {
                guiState->edit.settings.display_timer -= 1 ;
                _ui_changeTimer( -1 );
                vp_announceDisplayTimer();
            }
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            if( guiState->edit.settings.display_timer < TIMER_1H )
            {
                guiState->edit.settings.display_timer += 1 ;
                _ui_changeTimer( +1 );
                vp_announceDisplayTimer();
            }
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            handled = true ;
        }
    }

    return handled ;

}

static const uint8_t maxDaysInMonth[ 12 ] =
{ 31 , 28 , 31 , 30 , 31 , 30 , 31 , 31 , 30 , 31 , 30 , 31 };

static bool GuiValInp_Date( GuiState_st* guiState )
{
    datetime_t utc_time ;
    uint8_t    maxDay  = maxDaysInMonth[ guiState->edit.localTime.month - 1 ] ;
    bool       handled = false ;

    if( guiState->edit.localTime.month == 2 )
    {
        if( !( guiState->edit.localTime.year % 4 ) )
        {
            maxDay++ ;
        }
    }

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & KEY_UP )
        {
            if( guiState->layout.varInputSelect )
            {
                guiState->layout.varInputSelect-- ;
                handled = true ;
            }
        }
        else if( guiState->event.payload & KEY_DOWN )
        {
            if( guiState->layout.varInputSelect < VAR_INPUT_SELECT_2 )
            {
                guiState->layout.varInputSelect++ ;
                handled = true ;
            }
        }

        if( guiState->event.payload & ( KEY_LEFT  | KNOB_LEFT ) )
        {
            switch( guiState->layout.varInputSelect )
            {
                case VAR_INPUT_SELECT_0 :
                {
                    if( guiState->edit.localTime.date > 1 )
                    {
                        guiState->edit.localTime.date-- ;
                    }
                    break ;
                }
                case VAR_INPUT_SELECT_1 :
                {
                    if( guiState->edit.localTime.month > 1 )
                    {
                        guiState->edit.localTime.month-- ;
                    }
                    break ;
                }
                case VAR_INPUT_SELECT_2 :
                {
                    if( guiState->edit.localTime.year )
                    {
                        guiState->edit.localTime.year-- ;
                    }
                    break ;
                }
            }
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KNOB_RIGHT ) )
        {
            switch( guiState->layout.varInputSelect )
            {
                case VAR_INPUT_SELECT_0 :
                {
                    if( guiState->edit.localTime.date < maxDay )
                    {
                        guiState->edit.localTime.date++ ;
                    }
                    break ;
                }
                case VAR_INPUT_SELECT_1 :
                {
                    if( guiState->edit.localTime.month < 12 )
                    {
                        guiState->edit.localTime.month++ ;
                    }
                    break ;
                }
                case VAR_INPUT_SELECT_2 :
                {
                    if( guiState->edit.localTime.year < 99 )
                    {
                        guiState->edit.localTime.year++ ;
                    }
                    break ;
                }
            }
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            if( guiState->edit.localTime.date > maxDay )
            {
                guiState->edit.localTime.date = maxDay ;
            }
            utc_time = localTimeToUtc( guiState->edit.localTime    ,
                                       state.settings.utc_timezone   );
            platform_setTime( utc_time );
            handled = true ;
        }

    }

    return handled ;

}

static bool GuiValInp_Time( GuiState_st* guiState )
{
    datetime_t utc_time ;
    bool       handled  = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & KEY_UP )
        {
            if( guiState->layout.varInputSelect )
            {
                guiState->layout.varInputSelect-- ;
                handled = true ;
            }
        }
        else if( guiState->event.payload & KEY_DOWN )
        {
            if( guiState->layout.varInputSelect < VAR_INPUT_SELECT_2 )
            {
                guiState->layout.varInputSelect++ ;
                handled = true ;
            }
        }

        if( guiState->event.payload & ( KEY_LEFT  | KNOB_LEFT ) )
        {
            switch( guiState->layout.varInputSelect )
            {
                case VAR_INPUT_SELECT_0 :
                {
                    if( guiState->edit.localTime.hour )
                    {
                        guiState->edit.localTime.hour-- ;
                    }
                    break ;
                }
                case VAR_INPUT_SELECT_1 :
                {
                    if( guiState->edit.localTime.minute )
                    {
                        guiState->edit.localTime.minute-- ;
                    }
                    break ;
                }
                case VAR_INPUT_SELECT_2 :
                {
                    if( guiState->edit.localTime.second )
                    {
                        guiState->edit.localTime.second-- ;
                    }
                    break ;
                }
            }
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KNOB_RIGHT ) )
        {
            switch( guiState->layout.varInputSelect )
            {
                case VAR_INPUT_SELECT_0 :
                {
                    if( guiState->edit.localTime.hour < 23 )
                    {
                        guiState->edit.localTime.hour++ ;
                    }
                    break ;
                }
                case VAR_INPUT_SELECT_1 :
                {
                    if( guiState->edit.localTime.minute < 59 )
                    {
                        guiState->edit.localTime.minute++ ;
                    }
                    break ;
                }
                case VAR_INPUT_SELECT_2 :
                {
                    if( guiState->edit.localTime.second < 59 )
                    {
                        guiState->edit.localTime.second++ ;
                    }
                    break ;
                }
            }
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            utc_time = localTimeToUtc( guiState->edit.localTime  ,
                                       state.settings.utc_timezone   );
            platform_setTime( utc_time );
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
            if( guiState->edit.settings.gps_enabled )
            {
                guiState->edit.settings.gps_enabled = 0 ;
            }
            else
            {
                guiState->edit.settings.gps_enabled = 1 ;
            }
            vp_announceSettingsOnOffToggle( &currentLanguage->gpsEnabled ,
                                            guiState->queueFlags , guiState->edit.settings.gps_enabled );
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            state.settings.gps_enabled = guiState->edit.settings.gps_enabled ;
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
            if( guiState->edit.gps_set_time )
            {
                guiState->edit.gps_set_time = false ;
            }
            else
            {
                guiState->edit.gps_set_time = true ;
            }
            vp_announceSettingsOnOffToggle( &currentLanguage->gpsSetTime ,
                                            guiState->queueFlags , guiState->edit.gps_set_time );
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            state.gps_set_time = guiState->edit.gps_set_time ;
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
            guiState->edit.settings.utc_timezone -= 1 ;
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            guiState->edit.settings.utc_timezone += 1 ;
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            state.settings.utc_timezone = guiState->edit.settings.utc_timezone ;
            handled = true ;
        }
        vp_announceTimeZone( guiState->edit.settings.utc_timezone , guiState->queueFlags );
    }

    return handled ;

}
#endif // GPS_PRESENT

// R_OFFSET
static bool GuiValInp_Offset( GuiState_st* guiState )
{
    int32_t offset  = 0 ;
    bool    handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
        {
            if( guiState->edit.channel.tx_frequency )
            {
                guiState->edit.channel.tx_frequency -= 1 ;
            }
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            guiState->edit.channel.tx_frequency += 1 ;
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            state.channel.tx_frequency = guiState->edit.channel.tx_frequency ;
            state.channel.rx_frequency = guiState->edit.channel.rx_frequency ;
            vp_queueStringTableEntry( &currentLanguage->frequencyOffset );
            offset = (int32_t)guiState->edit.channel.tx_frequency -
                     (int32_t)guiState->edit.channel.rx_frequency   ;
            vp_queueFrequency( offset );
            guiState->f1Handled = true ;
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
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
        {
            if( !guiState->edit.direction )
            {
                guiState->edit.direction = true ;
            }
            else
            {
                guiState->edit.direction = false ;
            }
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            if( !guiState->edit.direction )
            {
                guiState->edit.direction = true ;
            }
            else
            {
                guiState->edit.direction = false ;
            }
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
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
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
        {
            guiState->edit.step_index += n_freq_steps ;
            guiState->edit.step_index-- ;
            guiState->edit.step_index %= n_freq_steps ;
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            guiState->edit.step_index++ ;
            guiState->edit.step_index %= n_freq_steps ;
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            state.step_index = guiState->edit.step_index ;
            handled = true ;
        }
    }

    return handled ;

}

// M17_CALLSIGN
static bool GuiValInp_Callsign( GuiState_st* guiState )
{
    uint8_t indexDst ;
    uint8_t indexSrc ;
    bool    handled = false ;

    guiState->sync_rtx = false ;

    if( guiState->event.type == EVENT_TYPE_KBD )
    {
        if( guiState->event.payload & KEY_UP )
        {
            if( guiState->layout.varInputSelect )
            {
                guiState->layout.varInputSelect-- ;
                handled = true ;
            }
        }
        else if( guiState->event.payload & KEY_DOWN )
        {
            if( guiState->layout.varInputSelect < VAR_INPUT_SELECT_5 )
            {
                guiState->layout.varInputSelect++ ;
                handled = true ;
            }
        }

        if( guiState->event.payload & ( KEY_LEFT  | KNOB_LEFT ) )
        {
            if( guiState->edit.settings.callsign[ guiState->layout.varInputSelect ] > 0x20 )
            {
                guiState->edit.settings.callsign[ guiState->layout.varInputSelect ]-- ;
            }
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KNOB_RIGHT ) )
        {
            if( guiState->edit.settings.callsign[ guiState->layout.varInputSelect ] < 0x7F )
            {
                guiState->edit.settings.callsign[ guiState->layout.varInputSelect ]++ ;
            }
            handled = true ;
        }
        else if( guiState->event.payload & KEY_STAR )
        {
            for( indexDst = CALLSIGN_MAX_LENGTH - 1 , indexSrc = indexDst - 1 ;
                 indexSrc > guiState->layout.varInputSelect  ;
                 indexDst-- , indexSrc-- )
            {
                guiState->edit.settings.callsign[ indexDst ] =
                guiState->edit.settings.callsign[ indexSrc ] ;
            }
            guiState->edit.settings.callsign[ indexDst ] = ' ' ;
            handled = true ;
        }
        else if( guiState->event.payload & KEY_HASH )
        {
            for( indexDst = guiState->layout.varInputSelect , indexSrc = indexDst + 1 ;
                 indexDst < CALLSIGN_MAX_LENGTH ;
                 indexDst++ , indexSrc++ )
            {
                guiState->edit.settings.callsign[ indexDst ] =
                guiState->edit.settings.callsign[ indexSrc ] ;
                if( !state.settings.callsign[ indexDst ] )
                {
                    break ;
                }
            }
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            for( indexDst = 0 ; indexDst < CALLSIGN_MAX_LENGTH ; indexDst++ )
            {
                state.settings.callsign[ indexDst ] = guiState->edit.settings.callsign[ indexDst ] ;
                if( !state.settings.callsign[ indexDst ] )
                {
                    break ;
                }
            }
            handled = true ;
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
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
        {
            guiState->edit.settings.m17_can -= 1 ;
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            guiState->edit.settings.m17_can += 1 ;
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            state.settings.m17_can = guiState->edit.settings.m17_can ;
            handled = true ;
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
        if( guiState->event.payload & ( KEY_LEFT | KEY_DOWN | KNOB_LEFT ) )
        {
            if( !guiState->edit.settings.m17_can_rx )
            {
                guiState->edit.settings.m17_can_rx = true ;
            }
            else
            {
                guiState->edit.settings.m17_can_rx = false ;
            }
            handled = true ;
        }
        else if( guiState->event.payload & ( KEY_RIGHT | KEY_UP | KNOB_RIGHT ) )
        {
            if( !guiState->edit.settings.m17_can_rx )
            {
                guiState->edit.settings.m17_can_rx = true ;
            }
            else
            {
                guiState->edit.settings.m17_can_rx = false ;
            }
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            state.settings.m17_can_rx = guiState->edit.settings.m17_can_rx ;
            handled = true ;
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
            if( guiState->edit.settings.vpLevel )
            {
                guiState->edit.settings.vpLevel-- ;
            }
            handled = true ;
        }
        else if( ( guiState->event.payload & KEY_RIGHT               ) ||
                 ( guiState->event.payload & ( KEY_UP | KNOB_RIGHT ) )    )
        {
            if( guiState->edit.settings.vpLevel < 255 )
            {
                guiState->edit.settings.vpLevel++ ;
            }
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            _ui_changeVoiceLevel( state.settings.vpLevel - guiState->edit.settings.vpLevel );
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
            guiState->edit.settings.vpPhoneticSpell = false ;
            handled = true ;
        }
        else if( ( guiState->event.payload & KEY_RIGHT               ) ||
                 ( guiState->event.payload & ( KEY_UP | KNOB_RIGHT ) )    )
        {
            guiState->edit.settings.vpPhoneticSpell = true ;
            handled = true ;
        }
        else if( guiState->event.payload & KEY_ENTER )
        {
            _ui_changePhoneticSpell( guiState->edit.settings.vpPhoneticSpell );
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
