/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Silvano Seva IU2KWO,                            *
 *                         Frederik Saraci IU2NRO                          *
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

#ifndef DATETIME_H
#define DATETIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Date and time type
 *
 * Data type representing current date and time, optimized for minimum space
 * occupancy.
 */
typedef struct
{
    uint8_t hour   : 5;    // Hours (0-23)
    uint8_t minute : 6;    // Minutes (0-59)
    uint8_t second : 6;    // Seconds (0-59)
    uint8_t day    : 3;    // Day of the week (1-7)
    uint8_t date   : 5;    // Day of the month (1-31)
    uint8_t month  : 4;    // Month (1-12)
    uint8_t year   : 7;    // Year (0-99)
    uint8_t        : 4;    // Padding to 40 bits
}
datetime_t;

/**
 * Convert from UTC time to local time, given the destination timezone.
 *
 * @param utc_time: UTC time.
 * @param timezone: destination time zone.
 * @return converted local time.
 */
datetime_t utcToLocalTime(const datetime_t utc_time, const int8_t timezone);

/**
 * Convert local time to UTC, given the source timezone.
 *
 * @param local_time: local time.
 * @param timezone: source time zone.
 * @return converted UTC time.
 */
datetime_t localTimeToUtc(const datetime_t local_time, const int8_t timezone);

#ifdef __cplusplus
}
#endif

#endif /* DATATYPES_H */
