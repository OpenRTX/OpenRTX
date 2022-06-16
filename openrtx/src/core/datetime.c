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

#include <datetime.h>


datetime_t utcToLocalTime(const datetime_t utc_time, const int8_t timezone)
{
    datetime_t local_time = utc_time;

    if(local_time.hour + timezone >= 24)
    {
        local_time.hour = local_time.hour - 24 + timezone;
        local_time.date += 1;
    }
    else if(local_time.hour + timezone < 0)
    {
        local_time.hour = local_time.hour + 24 + timezone;
        local_time.date -= 1;
    }
    else
    {
        local_time.hour += timezone;
    }

    return local_time;
}

datetime_t localTimeToUtc(const datetime_t local_time, const int8_t timezone)
{
    datetime_t utc_time = local_time;

    if(utc_time.hour - timezone >= 24)
    {
        utc_time.hour = utc_time.hour - 24 - timezone;
        utc_time.date += 1;
    }
    else if(utc_time.hour - timezone < 0)
    {
        utc_time.hour = utc_time.hour + 24 - timezone;
        utc_time.date -= 1;
    }
    else
    {
        utc_time.hour -= timezone;
    }

    return utc_time;
}
