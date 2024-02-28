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
    GUI_VAL_DSP_INP_STUBBED_0    , // GUI_VAL_INP_VFO_MIDDLE_INPUT

    GUI_VAL_DSP_CURRENT_TIME     ,
    GUI_VAL_DSP_BATTERY_LEVEL    ,
    GUI_VAL_DSP_LOCK_STATE       ,
    GUI_VAL_DSP_MODE_INFO        ,
    GUI_VAL_DSP_FREQUENCY        ,
    GUI_VAL_DSP_RSSI_METER       ,

    GUI_VAL_DSP_BANKS            ,
    GUI_VAL_DSP_CHANNELS         ,
    GUI_VAL_DSP_CONTACTS         ,
    GUI_VAL_DSP_GPS              ,
    // Settings
    // Display
#ifdef SCREEN_BRIGHTNESS
    GUI_VAL_DSP_BRIGHTNESS       ,
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
    GUI_VAL_DSP_CONTRAST         ,
#endif // SCREEN_CONTRAST
    GUI_VAL_DSP_TIMER            ,
    // Time and Date
    GUI_VAL_DSP_DATE             ,
    GUI_VAL_DSP_TIME             ,
    // GPS
    GUI_VAL_DSP_GPS_ENABLED      ,
    GUI_VAL_DSP_GPS_SET_TIME     ,
    GUI_VAL_DSP_GPS_TIME_ZONE    ,
    // Radio
    GUI_VAL_DSP_RADIO_OFFSET     ,
    GUI_VAL_DSP_RADIO_DIRECTION  ,
    GUI_VAL_DSP_RADIO_STEP       ,
    // M17
    GUI_VAL_DSP_M17_CALLSIGN     ,
    GUI_VAL_DSP_M17_CAN          ,
    GUI_VAL_DSP_M17_CAN_RX_CHECK ,
    // Accessibility - Voice
    GUI_VAL_DSP_LEVEL            ,
    GUI_VAL_DSP_PHONETIC         ,
    // Info
    GUI_VAL_DSP_BATTERY_VOLTAGE  ,
    GUI_VAL_DSP_BATTERY_CHARGE   ,
    GUI_VAL_DSP_RSSI             ,
    GUI_VAL_DSP_USED_HEAP        ,
    GUI_VAL_DSP_BAND             ,
    GUI_VAL_DSP_VHF              ,
    GUI_VAL_DSP_UHF              ,
    GUI_VAL_DSP_HW_VERSION       ,
#ifdef PLATFORM_TTWRPLUS
    GUI_VAL_DSP_RADIO            ,
    GUI_VAL_DSP_RADIO_FW         ,
#endif // PLATFORM_TTWRPLUS
    GUI_VAL_DSP_STUBBED          ,
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

extern void GuiVal_DisplayValue( GuiState_st* guiState , char* valueBuffer );

#endif // UI_VALUE_DISPLAY_H
