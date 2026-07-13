/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 backlight driver.  Drives the LCD backlight LED via PWM ch0
 * (HR_C7000 PWM block at MMIO 0x140c0000).  Mirrors the vendor V2.1.3
 * sequence from `pwm_channel_start(0, 10000, duty)` -- live-verified
 * against the real PWM ch0 register state on the running radio
 * (see memory project-hd2-v213-mmio-snapshot).
 *
 * Conforms to OpenRTX's backlight.h API (backlight_init / _terminate)
 * + the display.h `display_setBacklightLevel(level)` function.  The
 * higher-level "inactivity timeout" / brightness-LUT logic lives in
 * the application -- not in this driver -- per the OpenRTX pattern.
 */

#include "backlight.h"
#include "interfaces/display.h"
#include "hd2_regs.h"

#include <stdint.h>

/* PWM_CH0_BASE (ch0 = backlight) + PWM_TIMER_HZ (42 MHz, V2.1.3 boot
 * source DAT_03059db0) come from hd2_regs.h. */

/* Driver runs PWM at this frequency; perception-flat dimming threshold. */
#define BACKLIGHT_PWM_HZ    10000u

/* Track running state so backlight_init / display_setBacklightLevel
 * stay idempotent.  No re-init needed if already configured. */
static int  bl_initialised   = 0;
static int  bl_currently_on  = 0;
static uint8_t bl_last_level = 0;

static void pwm_program(volatile uint32_t *p, unsigned freq_hz,
                        unsigned duty_per_10000)
{
    /* Replicates V2.1.3 pwm_channel_start exactly.  See memory
     * project-hd2-v213-mmio-snapshot for the verified register order. */
    p[0] &= ~1u;
    p[0] &= ~4u;
    p[0] &= ~8u;
    p[0] &= ~0x30u;
    p[0] |=  0x20u;
    p[0] |=  0x100u;
    p[0] |=  0x200u;
    p[1] = 5;
    p[2] = PWM_TIMER_HZ / freq_hz;
    p[3] = (p[2] * duty_per_10000) / 10000u;
    p[0] |= 4u;
    p[0] |= 1u;
}

static void pwm_halt(volatile uint32_t *p)
{
    p[0] &= ~1u;
    p[0] |=  2u;
}

void backlight_init(void)
{
    /* IAP-default state on HD2 already has the SoC-level routing
     * needed to drive PWM ch0 output to the backlight LED -- our
     * platform_init mirrors the V2.1.3 socsys/GPIO state, and that's
     * enough.  Just mark the driver initialised; the first
     * display_setBacklightLevel() call will turn the LED on. */
    bl_initialised   = 1;
    bl_currently_on  = 0;
    bl_last_level    = 0;
}

void backlight_terminate(void)
{
    if (bl_initialised && bl_currently_on) {
        pwm_halt(PWM_CH0_BASE);
        bl_currently_on = 0;
    }
    bl_initialised = 0;
}

void display_setBacklightLevel(uint8_t level)
{
    if (!bl_initialised) return;

    if (level > 100u) level = 100u;

    /* Idempotent: skip the PWM re-program when the level is unchanged.  The UI
     * calls this on every standby enter/exit, and a noisy RSSI flickering the
     * squelch can fire that many times a second -- re-running pwm_program each
     * time (MMIO writes + divides) is needless bus thrash and part of the
     * cable-noise lockup path.  Only touch the PWM on an actual level change. */
    if (level == bl_last_level) return;
    bl_last_level = level;

    /* Keep the PWM running and just zero the duty when off -- pwm_halt
     * stops the counter but holds the output pin at its last state
     * (often mid-high-cycle = LED still lit).  Re-running pwm_program
     * each time is what the vendor does in backlight_tick. */
    pwm_program(PWM_CH0_BASE, BACKLIGHT_PWM_HZ, (unsigned)level * 100u);
    bl_currently_on = (level != 0u);
}

/* Local accessor used by the dynamic-loader main.c (avoids storing
 * brightness in another global).  Not part of the OpenRTX driver API. */
uint8_t hd2_backlight_last_level(void)
{
    return bl_last_level;
}
