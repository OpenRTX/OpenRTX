/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GPS_H
#define GPS_H

#include "core/datetime.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Enumeration type for GPS fix quality, according to the quality
 * indicator field of NMEA GGA sentence
 */
enum FixQuality {
    FIX_QUALITY_NO_FIX = 0,
    FIX_QUALITY_GPS,
    FIX_QUALITY_DGPS,
    FIX_QUALITY_PPS,
    FIX_QUALITY_RTK,
    FIX_QUALITY_RTK_FLOAT,
    FIX_QUALITY_ESTIMATED,
};

/**
 * Enumeration type for GPS fix type, according to the fix type field of
 * NMEA GSA sentence
 */
enum FixType {
    FIX_TYPE_NOT_AVAIL = 1,
    FIX_TYPE_2D,
    FIX_TYPE_3D,
};

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
    uint16_t   hdop;                 // Horizontal dilution of precision, in cm
}
gps_t;

/**
 * Data structure for GPS device managment.
 */
struct gpsDevice {
    void *priv;
    void (*enable)(void *priv);
    void (*disable)(void *priv);
    int  (*getSentence)(void *priv, char *buf, const size_t bufSize);
};

/**
 * Enable the GPS.
 *
 * @param dev: pointer to GPS device handle.
 */
static inline void gps_enable(const struct gpsDevice *dev)
{
    dev->enable(dev->priv);
}

/**
 * Disable the GPS.
 *
 * @param dev: pointer to GPS device handle.
 */
static inline void gps_disable(const struct gpsDevice *dev)
{
    dev->disable(dev->priv);
}

/**
 * Get a full NMEA sentence from the GPS.
 * This function is nonblocking.
 *
 * @param dev: pointer to GPS device handle.
 * @param buf: pointer to a buffer where to write the sentence.
 * @param bufSize: size of the destination buffer.
 * @return the length of the extracted sentence, -1 if the sentence is
 * longer than the maximum allowed size or zero if no sentence is available.
 */
static inline int gps_getSentence(const struct gpsDevice *dev, char *buf, const size_t bufSize)
{
    return dev->getSentence(dev->priv, buf, bufSize);
}

/**
 * This function perfoms the task of reading data from the GPS module,
 * if available, enabled and ready, decode NMEA sentences and update
 * the radio state with the retrieved data.
 */
void gps_task(const struct gpsDevice *dev);


#endif /* GPS_H */
