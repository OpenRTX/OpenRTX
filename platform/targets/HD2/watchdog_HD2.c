/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 failsafe watchdog: strong override of the OpenRTX watchdog_kick() hook
 * (interfaces/watchdog.h), kicked once per RTX-thread iteration from
 * rtx_threadFunc().  Arms the HR_C7000 watchdog (~10 s) on the first kick and
 * feeds it thereafter; if the RTX thread / scheduler hangs the feeds stop and
 * the chip hardware-resets -- the only recovery over the serial cable, which
 * backpowers the board so the power switch can't cycle it.
 *
 * This lives here (not in the rtx task) so the feed survives whichever rtx
 * implementation is linked (hd2_rtx.c today, the portable rtx.cpp after the
 * OpMode convergence).  Diag op 'X' toggles g_wdt_auto / forces a reset.
 */

#include "interfaces/watchdog.h"
#include "hd2_wdt.h"
#include <stdint.h>
#include <stdbool.h>

/* Watchdog auto-heartbeat enable (diag op 'X' in hd2_diag.cpp): 1 = on. */
volatile uint32_t g_wdt_auto = 1u;

/* Vestigial radio boot-inhibit (diag op 'F').  The old hd2_rtx.c deferred radio
 * bring-up behind this (an HW-I2C-wedge diagnostic); rtx.cpp brings the radio up
 * at rtx_init, so it's now a no-op flag kept only so the 'F' op still links. */
volatile int g_radio_enabled = 1;

void watchdog_kick(void)
{
    if(g_wdt_auto == 0u)
        return;

    static bool armed = false;
    if(!armed)
    {
        hd2_wdt_arm(10u);   /* lazy arm: the boot path can take as long as it likes */
        armed = true;
    }
    hd2_wdt_feed();
}
