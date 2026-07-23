/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
    int8_t hour;      // Hours (0-23)
    int8_t minute;    // Minutes (0-59)
    int8_t second;    // Seconds (0-59)
    int8_t day;       // Day of the week (1-7)
    int8_t date;      // Day of the month (1-31)
    int8_t month;     // Month (1-12)
    uint8_t year;     // Year (0-99)
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

/**
 * Adjust the values of the members of datetime_t if they are off-range or if
 * they have values that do not match the date described by the other members.
 *
 * @param time: pointer to a datetime_t struct.
 */
void realignTimeInfo(datetime_t *time);

/**
 * Convert a civil (proleptic Gregorian) date to a serial day count relative to
 * the Unix epoch (1970-01-01 = day 0).  Branch-free Howard Hinnant algorithm,
 * valid for years well outside the datetime_t range.
 *
 * @param year: full year, e.g. 2026.
 * @param month: month, 1-12.
 * @param day: day of the month, 1-31.
 * @return days since 1970-01-01 (negative for earlier dates).
 */
int32_t civilToDays(int32_t year, uint32_t month, uint32_t day);

/**
 * Inverse of civilToDays(): split a serial day count (days since 1970-01-01)
 * back into a civil year/month/day.
 *
 * @param days: days since 1970-01-01.
 * @param year: output, full year.
 * @param month: output, month 1-12.
 * @param day: output, day of the month 1-31.
 */
void daysToCivil(int32_t days, int32_t *year, uint32_t *month, uint32_t *day);

#ifdef __cplusplus
}
#endif

#endif /* DATATYPES_H */
