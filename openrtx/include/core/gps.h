/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Silvano Seva IU2KWO                      *
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

#ifndef GPS_H
#define GPS_H

#include <datetime.h>
#include <stdint.h>
#include <minmea.h>

/**
 * Data structure representing a single satellite as part of a GPS fix.
 */
typedef struct
{
    uint8_t  id;          // ID of the satellite
    uint8_t  elevation;   // Elevation in degrees
    uint16_t azimuth;     // Azimuth in degrees
    uint8_t  snr;         // Quality of the signal in range 0-99
}
gpssat_t;

/**
 * Data structure representing the last state received from the GPS module.
 */
typedef struct
{
    datetime_t timestamp;            // Timestamp of the latest GPS update
    uint8_t    fix_quality;          // 0: no fix, 1: GPS, 2: GPS SPS, 3: GPS PPS
    uint8_t    fix_type;             // 0: no fix, 1: 2D,  2: 3D
    uint8_t    satellites_tracked;   // Number of tracked satellites
    uint8_t    satellites_in_view;   // Satellites in view
    gpssat_t   satellites[12];       // Details about satellites in view
    uint32_t   active_sats;          // Bitmap representing which sats are part of the fix
    int32_t    latitude;             // Latitude coordinates
    int32_t    longitude;            // Longitude coordinates
    int16_t    altitude;             // Antenna altitude above mean sea level (geoid) in m
    uint16_t   speed;                // Ground speed in km/h
    int16_t    tmg_mag;              // Course over ground, degrees, magnetic
    int16_t    tmg_true;             // Course over ground, degrees, true
}
gps_t;

/**
 * This function perfoms the task of reading data from the GPS module,
 * if available, enabled and ready, decode NMEA sentences and update
 * the radio state with the retrieved data.
 */
void gps_task();


#endif /* GPS_H */
