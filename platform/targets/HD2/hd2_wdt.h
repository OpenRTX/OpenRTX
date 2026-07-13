/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HR_C7000 watchdog (manual §4.10, regs @ 0x1401_0000).  Used two ways:
 *  - reboot-on-command (diag op 'X' 0): arm a ~15 ms timeout and spin -- a
 *    full-chip reset, then BOOTROM -> IAP -> (no DFU keys held) -> app.  The
 *    only way to restart the radio over serial: the power switch does not
 *    actually cycle the board while the serial cable backpowers it.
 *  - auto-recovery during the hard-lock hunt: rtx_task arms ~10 s and feeds
 *    every pass; a system lock stops the feeds and the chip resets itself.
 *
 * Counter model (manual §4.10.5): a 35-bit up-counter {OT_H[2:0], OT_L[31:0]}
 * counts from the LOAD values to overflow; overflow = timeout.  Default load
 * 0x7:0xc0000000 (0x4000_0000 ticks) is documented as ~60 s, giving
 * ~17.9 M ticks/s.  WDG_KNOCK=0x55aadd22 restarts counting from the LOAD
 * values.  All registers are behind WDG_LOCK (write 0x5ada7200 to unlock);
 * we leave it unlocked so the fast-path feed is a single store.
 */

#pragma once

#include <stdint.h>

#define HD2_WDG_REG(off)  (*(volatile uint32_t *)(0x14010000u + (off)))
#define HD2_WDG_LOCK      HD2_WDG_REG(0x00u)   /* 0x5ada7200 = unlock        */
#define HD2_WDG_OT_H      HD2_WDG_REG(0x04u)   /* timeout counter load, [2:0] */
#define HD2_WDG_OT_L      HD2_WDG_REG(0x08u)   /* timeout counter load, low  */
#define HD2_WDG_EN        HD2_WDG_REG(0x10u)   /* bit0 = enable              */
#define HD2_WDG_KNOCK     HD2_WDG_REG(0x14u)   /* 0x55aadd22 = feed          */

#define HD2_WDG_UNLOCK_KEY 0x5ada7200u
#define HD2_WDG_FEED_KEY   0x55aadd22u

/* ~17.9 M ticks/s, from the documented "0x4000_0000 ticks ~= 60 s" default.
 * Coarse (the manual gives no clock source) -- fine for both use cases. */
#define HD2_WDT_TICKS_PER_S 0x01111111u

static inline void hd2_wdt_feed(void)
{
    HD2_WDG_KNOCK = HD2_WDG_FEED_KEY;
}

/* Arm with a timeout of ~`seconds` (1..119) and leave the block unlocked so
 * hd2_wdt_feed() works.  With OT_H=0x7 the remaining ticks to the 35-bit
 * overflow are just 2^32 - OT_L. */
static inline void hd2_wdt_arm(uint32_t seconds)
{
    uint32_t ticks = seconds * HD2_WDT_TICKS_PER_S;

    HD2_WDG_LOCK = HD2_WDG_UNLOCK_KEY;
    HD2_WDG_EN   = 0u;
    HD2_WDG_OT_H = 0x7u;
    HD2_WDG_OT_L = 0u - ticks;
    HD2_WDG_EN   = 1u;
    hd2_wdt_feed();                    /* restart the counter from the load */
}

static inline void hd2_wdt_off(void)
{
    HD2_WDG_LOCK = HD2_WDG_UNLOCK_KEY;
    HD2_WDG_EN   = 0u;
}

/* Full-chip reset in ~15 ms.  Does not return. */
static inline void hd2_wdt_force_reset(void)
{
    HD2_WDG_LOCK = HD2_WDG_UNLOCK_KEY;
    HD2_WDG_EN   = 0u;
    HD2_WDG_OT_H = 0x7u;
    HD2_WDG_OT_L = 0u - (HD2_WDT_TICKS_PER_S / 64u);
    HD2_WDG_EN   = 1u;
    hd2_wdt_feed();
    for (;;) { }
}
