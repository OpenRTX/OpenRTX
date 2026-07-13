/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Glue between OpenRTX and the (tickless) vendor Miosix kernel for the HD2,
 * plus UI-scope stubs so the app links without the radio/audio/DMR stack.
 *
 * delays: OpenRTX's interfaces/delays.h is implemented per-MCU on top of the
 * kernel primitives (cf. STM32H7xx/drivers/delays.cpp). The vendor kernel is
 * TICKLESS: getTime() returns ns and REPLACES the old getTick(); sleepUntil is
 * nanoSleepUntil(absNs). OpenRTX works in ms (TICK_FREQ=1000), so convert.
 */

#include <miosix.h>

// The vendor kernel's delayMs/delayUs live in namespace miosix. OpenRTX's
// interfaces/delays.h declares GLOBAL delayMs/delayUs (and both repos ship an
// "interfaces/delays.h", so include-path order can shadow one). Forward-declare
// the kernel symbols here and provide the global OpenRTX-facing wrappers.
namespace miosix { void delayMs(unsigned int); void delayUs(unsigned int); }

extern "C" {

// ---- raw UART0 trace for bring-up (UART0 @ 0x14030000, already 57600 by
//      platform_init). Bounded spin so a stuck UART can't hang a thread. -----
void hd2_trace(const char *s)
{
    volatile unsigned int *thr = (volatile unsigned int *)0x14030000u;
    volatile unsigned int *lsr = (volatile unsigned int *)0x14030014u;
    while(*s)
    {
        for(unsigned int g = 0; g < 200000u && (*lsr & 0x20u) == 0u; ++g) {}
        *thr = (unsigned char)*s++;
    }
}

// ---- OpenRTX delays interface (global symbols OpenRTX code calls) ----------
void delayMs(unsigned int ms) { miosix::delayMs(ms); }
void delayUs(unsigned int us) { miosix::delayUs(us); }

void sleepFor(unsigned int seconds, unsigned int mseconds)
{
    miosix::Thread::sleep(seconds * 1000u + mseconds);
}

void sleepUntil(long long timestampMs)
{
    // OpenRTX timestamps are ms (getTick units); kernel wants absolute ns.
    miosix::Thread::nanoSleepUntil(timestampMs * 1000000LL);
}

long long getTick(void)
{
    // ns -> ms to match OpenRTX's TICK_FREQ=1000 expectation.
    return miosix::getTime() / 1000000LL;
}

} // extern "C"
