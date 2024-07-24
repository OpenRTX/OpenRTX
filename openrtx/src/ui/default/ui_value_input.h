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

#ifndef UI_VALUE_INPUT_H
#define UI_VALUE_INPUT_H

#include <ui/ui_default.h>

// GUI Values - Input
enum
{
    GUI_VAL_INP_VFO_MIDDLE_INPUT ,

    GUI_VAL_INP_CURRENT_TIME     ,
    GUI_VAL_INP_BATTERY_LEVEL    ,
    GUI_VAL_INP_LOCK_STATE       ,
    GUI_VAL_INP_MODE_INFO        ,
    GUI_VAL_INP_FREQUENCY        ,
    GUI_VAL_INP_RSSI_METER       ,

    GUI_VAL_INP_BANKS            ,
    GUI_VAL_INP_CHANNELS         ,
    GUI_VAL_INP_CONTACTS         ,
    GUI_VAL_INP_GPS              ,
#ifdef SCREEN_BRIGHTNESS
    GUI_VAL_INP_BRIGHTNESS       , // D_BRIGHTNESS
#endif // SCREEN_BRIGHTNESS
#ifdef SCREEN_CONTRAST
    GUI_VAL_INP_CONTRAST         , // D_CONTRAST
#endif // SCREEN_CONTRAST
    GUI_VAL_INP_TIMER            , // D_TIMER
    GUI_VAL_INP_DATE             ,
    GUI_VAL_INP_TIME             ,
    GUI_VAL_INP_GPS_ENABLED      , // G_ENABLED
    GUI_VAL_INP_GPS_SET_TIME     , // G_SET_TIME
    GUI_VAL_INP_GPS_TIME_ZONE    , // G_TIMEZONE
    GUI_VAL_INP_RADIO_OFFSET     , // R_OFFSET
    GUI_VAL_INP_RADIO_DIRECTION  , // R_DIRECTION
    GUI_VAL_INP_RADIO_STEP       , // R_STEP
    GUI_VAL_INP_M17_CALLSIGN     , // M17_CALLSIGN
    GUI_VAL_INP_M17_CAN          , // M17_CAN
    GUI_VAL_INP_M17_CAN_RX_CHECK , // M17_CAN_RX
    GUI_VAL_INP_LEVEL            , // VP_LEVEL
    GUI_VAL_INP_PHONETIC         , // VP_PHONETIC
    GUI_VAL_INP_BATTERY_VOLTAGE  ,
    GUI_VAL_INP_BATTERY_CHARGE   ,
    GUI_VAL_INP_RSSI             ,
    GUI_VAL_INP_USED_HEAP        ,
    GUI_VAL_INP_BAND             ,
    GUI_VAL_INP_VHF              ,
    GUI_VAL_INP_UHF              ,
    GUI_VAL_INP_HW_VERSION       ,
#ifdef PLATFORM_TTWRPLUS
    GUI_VAL_INP_RADIO            ,
    GUI_VAL_INP_RADIO_FW         ,
#endif // PLATFORM_TTWRPLUS
    GUI_VAL_INP_STUBBED          ,

    GUI_VAL_INP_NUM_OF
};

extern bool GuiValInp_InputValue( GuiState_st* guiState );
extern void ui_ValueInputFSM( GuiState_st* guiState );


#endif // UI_VALUE_INPUT_H
