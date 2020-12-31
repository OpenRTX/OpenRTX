/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#include <os.h>
#include <interfaces/rtc.h>

/*
 * NOTE: even if the MK22FN512 MCU has an RTC, it is unusable in GDx platforms
 * because they lacks of the proper hardware necessary to run the RTC also when
 * the MCU is powered off.
 * We thus provide a stub implementation of the RTC API to avoid cluttering the
 * main code with #ifdefs checking wheter or not the RTC can be actually used.
 */

void rtc_init() { }

void rtc_terminate() { }

void rtc_setTime(curTime_t t)
{
    (void) t;
}

void rtc_setHour(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    (void) hours;
    (void) minutes;
    (void) seconds;
}

void rtc_setDate(uint8_t date, uint8_t month, uint8_t year)
{
    (void) date;
    (void) month;
    (void) year;
}

curTime_t rtc_getTime()
{
    curTime_t t;

    t.hour   = 12;
    t.minute = 12;
    t.second = 12;
    t.year  = 20;
    t.day   = 4;
    t.month = 12;
    t.date  = 12;

    return t;
}

void rtc_dstSet() { }

void rtc_dstClear() { }
