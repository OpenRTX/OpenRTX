/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO                             *
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

#ifndef STATE_H
#define STATE_H

#include <datatypes.h>
#include <stdbool.h>
#include <rtc.h>
#include <cps.h>

/**
 * Part of this structure has been commented because the corresponding
 * functionality is not yet implemented.
 * Uncomment once the related feature is ready
 */
typedef struct
{
    bool radioStateUpdated;
    curTime_t time;
    float v_bat;

    uint8_t ui_screen;
    uint8_t tuner_mode;
    uint8_t radio_mode;
    
    //time_t rx_status_tv;
    //bool rx_status;
    
    //time_t tx_status_tv;
    //bool tx_status;

    bool channelInfoUpdated;
    channel_t channel;
    uint8_t rtxStatus;
    uint8_t sqlLevel;
    uint8_t voxLevel;
}
state_t;

enum TunerMode
{
    VFO = 0,
    CH,
    SCAN,
    CHSCAN
};

enum RadioMode
{
    MODE_FM = 0,
    MODE_NFM,
    MODE_DMR,
};

enum RtxStatus
{
    RTX_OFF = 0,
    RTX_RX,
    RTX_TX
};

extern state_t state;

/**
 * This function initializes the Radio state, acquiring the information
 * needed to populate it from device drivers.
 */
void state_init();

#endif /* STATE_H */
