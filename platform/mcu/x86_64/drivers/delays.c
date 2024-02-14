/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Frederik Saraci IU2NRO                   *
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
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

/**
 * Implementation of the delay functions for x86_64.
 */

void delayUs(unsigned int useconds)
{
    usleep(useconds);
}

void delayMs(unsigned int mseconds)
{
    usleep(mseconds*1000);
}

void sleepFor(unsigned int seconds, unsigned int mseconds)
{
    unsigned int time = (seconds * 1000) + mseconds;
    delayMs(time);
}

void sleepUntil(long long timestamp)
{
    long long delta = timestamp - getTimeMs();
    if(delta <= 0) return;
    delayMs(delta);
}

long long getTimeMs()
{
    /*
     * Return current time in milliseconds, consistently with the miosix kernel
     * having a tick rate of 1kHz.
     */

    struct timeval te;
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}
