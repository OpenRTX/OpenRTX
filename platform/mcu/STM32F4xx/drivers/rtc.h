/***************************************************************************
 *   Copyright (C) 2020 by Silvano Seva IU2KWO and Niccolò Izzo IU2KIN     *
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

#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include "stm32f4xx.h"

/**
 * Driver for STM32 real time clock, providing both calendar and clock
 * functionalities.
 *
 * RTC is active also when radio is powered off, thanks to the internal
 * lithium backup battery.
 */

typedef struct
{
    uint8_t hour   : 5;    /* Hours (0-23)            */
    uint8_t minute : 6;    /* Minutes (0-59)          */
    uint8_t second : 6;    /* Seconds (0-59)          */
    uint8_t day    : 3;    /* Day of the week (1-7)   */
    uint8_t date   : 4;    /* Day of the month (1-31) */
    uint8_t month  : 4;    /* Month (1-12)            */
    uint8_t year   : 7;    /* Year (0-99)             */
    uint8_t        : 5;    /* Padding to 40 bits      */
}curTime_t;

/**
 * Initialise and start RTC, which uses as clock source the external 32.768kHz
 * crystal connected to PC14 and PC15 (indicated with LSE in STM32 reference
 * manual).
 */
void rtc_init();

/**
 * Shutdown RTC and external 32.768kHz clock source.
 */
void rtc_shutdown();

/**
 * Set RTC time and calendar registers to a given value.
 * @param t: struct of type curTime_t, whose content is used to initialise both
 * clock and calendar registers.
 */
void rtc_setTime(curTime_t t);

/**
 * Set RTC clock keeping untouched the calendar part.
 * @param hours: new value for hours, between 0 and 23.
 * @param minutes: new value for minutes, between 0 and 59.
 * @param seconds: new value for seconds, between 0 and 59.
 */
void rtc_setHour(uint8_t hours, uint8_t minutes, uint8_t seconds);

/**
 * Set RTC calendar keeping untouched the clock part.
 * @param date: new value for the date, between 1 and 31.
 * @param month: new value for the month, between 1 and 12.
 * @param year: new value for the year, between 00 and 99.
 */
void rtc_setDate(uint8_t date, uint8_t month, uint8_t year);

/**
 * Get current date and time.
 * @return structure of type curTime_t with current clock and calendar values.
 */
curTime_t rtc_getTime();

/**
 * Activate daylight saving time (DST), adding one hour to the current time.
 * This function can be safely called multiple times: calls following the one
 * which firstly activates DST have no effect.
 */
void rtc_dstSet();

/**
 * Switch back from daylight saving time (DST), removing one hour from the
 * current time.
 * This function can be safely called multiple times: calls following the one
 * which firstly dectivates DST have no effect.
 */
void rtc_dstClear();

#endif /* RTC_H */
