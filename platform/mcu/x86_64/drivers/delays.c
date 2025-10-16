/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/delays.h"
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
    long long delta = timestamp - getTick();
    if(delta <= 0) return;
    delayMs(delta);
}

long long getTick()
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
