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

#ifndef UI_VALUE_DISPLAY_H
#define UI_VALUE_DISPLAY_H

#include <ui/ui_default.h>

// GUI Values - Display
enum
{
    GUI_VAL_DSP_BATTERY_LEVEL    , // 0x00
    GUI_VAL_DSP_LOCK_STATE       , // 0x01
    GUI_VAL_DSP_MODE_INFO        , // 0x02
    GUI_VAL_DSP_BANK_CHANNEL     , // 0x03
    GUI_VAL_DSP_FREQUENCY        , // 0x04
    GUI_VAL_DSP_RSSI_METER       , // 0x05

    GUI_VAL_DSP_BANKS            , // 0x06
    GUI_VAL_DSP_CHANNELS         , // 0x07
    GUI_VAL_DSP_CONTACTS         , // 0x08
#ifdef GPS_PRESENT
    GUI_VAL_DSP_GPS              , // 0x09
#endif // GPS_PRESENT
    // Settings
    // Display
#ifdef SCREEN_BRIGHTNESS
    GUI_VAL_DSP_BRIGHTNESS       , // 0x0A
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
    GUI_VAL_DSP_CONTRAST         , // 0x0B
#endif // SCREEN_CONTRAST
    GUI_VAL_DSP_TIMER            , // 0x0C
    // Time and Date
    GUI_VAL_DSP_DATE             , // 0x0D
    GUI_VAL_DSP_TIME             , // 0x0E
    // GPS
#ifdef GPS_PRESENT
    GUI_VAL_DSP_GPS_ENABLED      , // 0x0F
    GUI_VAL_DSP_GPS_SET_TIME     , // 0x10
    GUI_VAL_DSP_GPS_TIME_ZONE    , // 0x11
#endif // GPS_PRESENT
    // Radio
    GUI_VAL_DSP_RADIO_OFFSET     , // 0x12
    GUI_VAL_DSP_RADIO_DIRECTION  , // 0x13
    GUI_VAL_DSP_RADIO_STEP       , // 0x14
    // M17
    GUI_VAL_DSP_M17_CALLSIGN     , // 0x15
    GUI_VAL_DSP_M17_CAN          , // 0x16
    GUI_VAL_DSP_M17_CAN_RX_CHECK , // 0x17
    // Accessibility - Voice
    GUI_VAL_DSP_LEVEL            , // 0x18
    GUI_VAL_DSP_PHONETIC         , // 0x19
    // Info
    GUI_VAL_DSP_BATTERY_VOLTAGE  , // 0x1A
    GUI_VAL_DSP_BATTERY_CHARGE   , // 0x1B
    GUI_VAL_DSP_RSSI             , // 0x1C
    GUI_VAL_DSP_USED_HEAP        , // 0x1D
    GUI_VAL_DSP_BAND             , // 0x1E
    GUI_VAL_DSP_VHF              , // 0x1F
    GUI_VAL_DSP_UHF              , // 0x20
    GUI_VAL_DSP_HW_VERSION       , // 0x21
#ifdef PLATFORM_TTWRPLUS
    GUI_VAL_DSP_RADIO            , // 0x22
    GUI_VAL_DSP_RADIO_FW         , // 0x23
#endif // PLATFORM_TTWRPLUS
    GUI_VAL_DSP_BACKUP_RESTORE   , // 0x24
    GUI_VAL_DSP_LOW_BATTERY      , // 0x25
#ifdef DISPLAY_DEBUG_MSG
    GUI_VAL_DSP_DEBUG_MSG        , // 0x26
    GUI_VAL_DSP_DEBUG_VALUES     , // 0x27
#endif // DISPLAY_DEBUG_MSG
    GUI_VAL_DSP_STUBBED          , // 0x28
    GUI_VAL_DSP_NUM_OF
};

typedef enum
{
#ifdef SCREEN_BRIGHTNESS
    D_BRIGHTNESS ,          // GUI_VAL_INP_BRIGHTNESS
#endif
#ifdef SCREEN_CONTRAST
    D_CONTRAST   ,          // GUI_VAL_INP_CONTRAST
#endif
    D_TIMER                 // GUI_VAL_INP_TIMER
}DisplayItems_en;

#ifdef GPS_PRESENT
typedef enum
{
    G_ENABLED  ,            // GUI_VAL_INP_ENABLED
    G_SET_TIME ,            // GUI_VAL_INP_SET_TIME
    G_TIMEZONE              // GUI_VAL_INP_TIMEZONE
}SettingsGPSItems_en;
#endif

typedef enum
{
    VP_LEVEL    ,           // GUI_VAL_INP_LEVEL
    VP_PHONETIC             // GUI_VAL_INP_PHONETIC
}SettingsVoicePromptItems_en;

typedef enum
{
    R_OFFSET    ,           // GUI_VAL_INP_OFFSET
    R_DIRECTION ,           // GUI_VAL_INP_DIRECTION
    R_STEP                  // GUI_VAL_INP_STEP
}SettingsRadioItems_en;

typedef enum
{
    M17_CALLSIGN ,          // GUI_VAL_INP_CALLSIGN
    M17_CAN      ,          // GUI_VAL_INP_CAN
    M17_CAN_RX              // GUI_VAL_INP_CAN_RX
}SettingsM17Items_en;

extern void GuiVal_DisplayValue( GuiState_st* guiState , uint8_t valueNum );
#ifdef DISPLAY_DEBUG_MSG
extern void GuiVal_SetDebugMessage( char* debugMsg );
extern void GuiVal_SetDebugValues( uint8_t debugVal0 , uint8_t debugVal1 ,
                                uint8_t debugVal2 , uint8_t debugVal3 ,
                                uint8_t debugVal4 , uint8_t debugVal5   );
#endif // DISPLAY_DEBUG_MSG

#endif // UI_VALUE_DISPLAY_H
