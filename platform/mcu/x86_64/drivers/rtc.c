/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <time.h>
#include <interfaces/rtc.h>

void rtc_init()
{
    printf("rtc_init()\n");
}

void rtc_terminate()
{
    printf("rtc_shutdown()\n");
}

void rtc_setTime(__attribute__((unused)) curTime_t t)
{
    printf("rtc_setTime(t)\n");
}

void rtc_setHour(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    printf("rtc_setHour(%d, %d, %d)\n", hours, minutes, seconds);
}

void rtc_setDate(uint8_t date, uint8_t month, uint8_t year)
{
    printf("rtc_setDate(%d, %d, %d)\n", date, month, year);
}

curTime_t rtc_getTime()
{
    curTime_t t;

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = gmtime ( &rawtime ); 
    //radio expects UTC time from GPS (well, close enough anyway), so give it gmtime instead of localtime

    t.hour = timeinfo->tm_hour;
    t.minute = timeinfo->tm_min;
    t.second = timeinfo->tm_sec;
    t.day = timeinfo->tm_wday;
    t.date = timeinfo->tm_mday;
    t.month = timeinfo->tm_mon + 1;
    // Only last two digits of the year are supported in OpenRTX
    t.year = (timeinfo->tm_year + 1900) % 100;

    return t;
}

void rtc_dstSet()
{
    printf("rtc_dstSet()\n");
}

void rtc_dstClear()
{
    printf("rtc_dstClear()\n");
}
