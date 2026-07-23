/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * OpenRTX interfaces/delays.h implementation for the HR_C7000 (CK803S), built
 * on the tickless vendor Miosix kernel: getTime() returns nanoseconds (it
 * replaces the old getTick), and sleepUntil is nanoSleepUntil(absNs).  OpenRTX
 * works in milliseconds (TICK_FREQ=1000), so convert at the boundary.  Modeled
 * on the per-MCU delays.cpp of the other targets (e.g. mcu/STM32F4xx/drivers).
 */

#include "interfaces/delays.h"
#include <miosix.h>

/* The kernel's busy-wait delays live in namespace miosix; the OpenRTX interface
 * (interfaces/delays.h, extern "C") declares the GLOBAL delayMs/delayUs. */
namespace miosix
{
void delayMs(unsigned int);
void delayUs(unsigned int);
}

void delayUs(unsigned int useconds)
{
    miosix::delayUs(useconds);
}

void delayMs(unsigned int mseconds)
{
    miosix::delayMs(mseconds);
}

void sleepFor(unsigned int seconds, unsigned int mseconds)
{
    miosix::Thread::sleep(seconds * 1000u + mseconds);
}

void sleepUntil(long long timestamp)
{
    /* OpenRTX passes an absolute millisecond timestamp; the tickless kernel
     * wants absolute nanoseconds. */
    miosix::Thread::nanoSleepUntil(timestamp * 1000000LL);
}

long long getTick(void)
{
    return miosix::getTime() / 1000000LL; /* ns -> ms (TICK_FREQ=1000) */
}
