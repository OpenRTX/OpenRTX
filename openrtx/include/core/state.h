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
#include <interfaces/rtc.h>
#include <cps.h>
#include <settings.h>
#include <gps.h>

/**
 * Data structure representing the settings of the M17 mode.
 */
typedef struct
{
    char dst_addr[10];
}
m17_t;


/**
 * Part of this structure has been commented because the corresponding
 * functionality is not yet implemented.
 * Uncomment once the related feature is ready
 */
typedef struct
{
    curTime_t  time;
    uint16_t   v_bat;
    uint8_t    charge;
    float      rssi;

    uint8_t    ui_screen;
    uint8_t    tuner_mode;

    uint16_t   channel_index;
    channel_t  channel;
    channel_t  vfo_channel;
    bool       bank_enabled;
    uint16_t   bank;
    uint8_t    rtxStatus;
    bool       rtxShutdown;

    bool       emergency;
    settings_t settings;
    gps_t      gps_data;
    bool       gps_set_time;
    m17_t      m17_data;
}
state_t;

enum TunerMode
{
    VFO = 0,
    CH,
    SCAN,
    CHSCAN
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

/**
 * This function terminates the radio state saving persistent settings to flash.
 */
void state_terminate();

/**
 * Update radio state fetching data from device drivers.
 */
void state_update();

/**
 * Reset the fields of radio state containing user settings and VFO channel.
 */
void state_resetSettingsAndVfo();

/**
 * The RTC and state.time are set to UTC time
 * Use this function to get local time from UTC time based on timezone setting
 */
curTime_t state_getLocalTime(curTime_t utc_time);

/**
 * The RTC and state.time are set to UTC time
 * Use this function to get UTC time from local time based on timezone setting
 */
curTime_t state_getUTCTime(curTime_t local_time);


#endif /* STATE_H */
