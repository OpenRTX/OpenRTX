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

/**
 * Data structure representing a single satellite as part of a GPS fix.
 */
typedef struct
{
    uint8_t id;           // ID of the satellite
    uint8_t elevation;    // Elevation in degrees
    uint16_t azimuth;     // Azimuth in degrees
    uint8_t snr;          // Quality of the signal in range 0-99
}
sat_t;

/**
 * Data structure representing the last state received from the GPS module.
 */
typedef struct
{
    curTime_t timestamp;  // Timestamp of the latest GPS update
    uint8_t fix_quality;  // 0: no fix, 1: GPS, 2: GPS SPS, 3: GPS PPS
    uint8_t fix_type;     // 0: no fix, 1: 2D,  2: 3D
    uint8_t satellites_tracked; // Number of tracked satellites
    uint8_t satellites_in_view; // Satellites in view
    sat_t satellites[12]; // Details about satellites in view
    uint32_t active_sats; // Bitmap representing which sats are part of the fix
    float latitude;       // Latitude coordinates
    float longitude;      // Longitude coordinates
    float altitude;       // Antenna altitude above mean sea level (geoid) in m
    float speed;          // Ground speed in km/h
    float tmg_mag;        // Course over ground, degrees, magnetic
    float tmg_true;       // Course over ground, degrees, true
}
gps_t;

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
    float charge;
    float rssi;

    uint8_t ui_screen;
    uint8_t tuner_mode;

    //time_t rx_status_tv;
    //bool rx_status;

    //time_t tx_status_tv;
    //bool tx_status;

    uint16_t channel_index;
    channel_t channel;
    channel_t vfo_channel;
    bool zone_enabled;
    zone_t zone;
    uint8_t rtxStatus;
    // Squelch steps from 0 to 15
    uint8_t sqlLevel;
    uint8_t voxLevel;

    bool emergency;
    settings_t settings;
    gps_t gps_data;
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

state_t state;

/**
 * This function initializes the Radio state, acquiring the information
 * needed to populate it from device drivers.
 */
void state_init();

/**
 * This function terminates the Radio state,
 * Saving persistent settings to flash.
 */
void state_terminate();

/**
 * This function applies the selected timezone after reading the time from
 * the RTC.
 */
void state_applyTimezone();

#endif /* STATE_H */
