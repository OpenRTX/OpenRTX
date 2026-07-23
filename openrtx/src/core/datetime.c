/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/datetime.h"
#include <stdlib.h>
#include <time.h>

static const int DAYS_IN_MONTH[12] =
{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#define _DAYS_IN_MONTH(x) ((x == 2) ? days_in_feb : DAYS_IN_MONTH[x - 1])

/**
 * \internal
 * Return the number of days in a year by checking wheter the current year is
 * a leap one or not.
 *
 * @param year: current year, ranging from 0 to 99.
 * @return number of days in the year.
 */
static inline uint16_t daysInYear(uint16_t year)
{
    year += 2000;
    if( ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0)))
    {
        return 366;
    }

    return 365;
}



datetime_t utcToLocalTime(const datetime_t utc_time, const int8_t timezone)
{
    datetime_t local_time = utc_time;

    local_time.hour   += (timezone / 2);
    local_time.minute += (timezone % 2) * 30;
    realignTimeInfo(&local_time);

    return local_time;
}

datetime_t localTimeToUtc(const datetime_t local_time, const int8_t timezone)
{
    datetime_t utc_time = local_time;

    utc_time.minute -= (timezone % 2) * 30;
    utc_time.hour   -= (timezone / 2);
    realignTimeInfo(&utc_time);

    return utc_time;
}

/*
 * Function borrowed from newlib implementation of mktime, see
 * https://github.com/bminor/newlib/blob/master/newlib/libc/time/mktime.c
 *
 * Slightly modified to remove the _DAYS_IN_YEAR macro and to handle month
 * with range 1-12 instead of 0-11.
 */
void realignTimeInfo(datetime_t *time)
{
    div_t res;
    int days_in_feb = 28;

    if((time->second < 0) || (time->second > 59))
    {
        res = div(time->second, 60);
        time->minute += res.quot;

        if ((time->second = res.rem) < 0)
        {
            time->second += 60;
            time->minute -= 1;
        }
    }

    if((time->minute < 0) || (time->minute > 59))
    {
        res = div(time->minute, 60);
        time->hour += res.quot;

        if((time->minute = res.rem) < 0)
        {
            time->minute += 60;
            time->hour   -= 1;
        }
    }

    if((time->hour < 0) || (time->hour > 23))
    {
        res = div(time->hour, 24);
        time->date += res.quot;

        if((time->hour = res.rem) < 0)
        {
            time->hour += 24;
            time->date -= 1;
        }
    }

    if((time->month < 0) || (time->month > 12))
    {
        res = div (time->month, 12);
        time->year += res.quot;

        if((time->month = res.rem) < 0)
        {
            time->month += 12;
            time->year  -= 1;
        }
    }

    if(daysInYear(time->year) == 366)
        days_in_feb = 29;

    if(time->date <= 0)
    {
        while(time->date <= 0)
        {
            time->month -= 1;
            if(time->month == -1)
            {
                time->year -= 1;
                time->month = 12;
                days_in_feb = ((daysInYear(time->year) == 366) ? 29 : 28);
            }

            time->date += _DAYS_IN_MONTH (time->month);
        }
    }
    else
    {
        while(time->date > _DAYS_IN_MONTH (time->month))
        {
            time->date  -= _DAYS_IN_MONTH (time->month);
            time->month += 1;
            if (time->month > 12)
            {
                time->year += 1;
                time->month = 1;
                days_in_feb = ((daysInYear (time->year) == 366) ? 29 : 28);
            }
        }
    }
}

/*
 * Civil <-> serial-day conversion (days since 1970-01-01), branch-free after
 * Howard Hinnant's "chrono-Compatible Low-Level Date Algorithms".
 */
int32_t civilToDays(int32_t year, uint32_t month, uint32_t day)
{
    year -= (month <= 2);
    int32_t era = (year >= 0 ? year : year - 399) / 400;
    uint32_t yoe = (uint32_t)(year - era * 400);
    uint32_t doy = (153u * (month + (month > 2 ? -3u : 9u)) + 2u) / 5u + day
                 - 1u;
    uint32_t doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return era * 146097 + (int32_t)doe - 719468;
}

void daysToCivil(int32_t days, int32_t *year, uint32_t *month, uint32_t *day)
{
    days += 719468;
    int32_t era = (days >= 0 ? days : days - 146096) / 146097;
    uint32_t doe = (uint32_t)(days - era * 146097);
    uint32_t yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
    int32_t yr = (int32_t)yoe + era * 400;
    uint32_t doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
    uint32_t mp = (5u * doy + 2u) / 153u;
    *day = doy - (153u * mp + 2u) / 5u + 1u;
    *month = mp + (mp < 10u ? 3u : -9u);
    *year = yr + (*month <= 2);
}
