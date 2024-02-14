/***************************************************************************
 *   Copyright (C) 2023 by Silvano Seva IU2KWO                             *
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

#include <interfaces/delays.h>
#include <zephyr/kernel.h>
#include <stdint.h>


void delayUs(unsigned int useconds)
{
    k_usleep(useconds);
}

void delayMs(unsigned int mseconds)
{
    k_msleep(mseconds);
}

void sleepFor(unsigned int seconds, unsigned int mseconds)
{
    uint64_t duration = (seconds * 1000) + mseconds;
    k_sleep(K_MSEC(duration));
}

void sleepUntil(long long timestamp)
{
    if(timestamp <= k_uptime_get())
        return;

    k_sleep(K_TIMEOUT_ABS_MS(timestamp));
}

long long getTimeMs()
{
    // k_uptime_get() returns the elapsed time since the system booted,
    // in milliseconds.
    return k_uptime_get();
}
