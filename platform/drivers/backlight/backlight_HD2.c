/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 LCD-backlight driver: the LED is driven by PWM channel 0 of the
 * HR_C7000 PWM block (0x140c0000).  The channel-start register order
 * replicates the vendor V2.1.3 pwm_channel_start sequence exactly.
 *
 * Owns the OpenRTX display_setBacklightLevel() entry point (the backlight
 * driver owns the PWM; the display driver only owns the LCD bus).
 */

#include "interfaces/display.h"
#include "registers.h"
#include <stdint.h>

#define BACKLIGHT_PWM_HZ 10000u /* perception-flat dimming frequency */

static void pwm_program(PWM_Channel_TypeDef *p, unsigned freq_hz,
                        unsigned duty_per_10000)
{
    /* Vendor V2.1.3 channel-start order: clear enable/mode bits, select the
     * gated source, set period + duty, then re-enable. */
    p->CFG &= ~1u;
    p->CFG &= ~4u;
    p->CFG &= ~8u;
    p->CFG &= ~0x30u;
    p->CFG |= 0x20u;
    p->CFG |= 0x100u;
    p->CFG |= 0x200u;
    p->RPT = 5;
    p->PER = freq_hz ? (PWM_TIMER_HZ / freq_hz) : 1u;
    p->FP = (p->PER * duty_per_10000) / 10000u;
    /* Never let duty == period: on this comparator that wraps to a
     * permanently-LOW output (LED off) instead of "always on".  Full
     * brightness (level 100 -> duty_per_10000 10000) must map to period-1. */
    if (p->FP >= p->PER)
        p->FP = p->PER - 1u;
    p->CFG |= 4u;
    p->CFG |= 1u;
}

void backlight_init(void)
{
    /* Restore the PWM ch0 duty-gating control words (STATUS @0x10 and the three
     * reserved words @0x14-0x1c) to their V2.1.3 boot value.  Left at 0 (as the
     * kernel/IAP hand-off leaves them for us) the duty cycle is squashed and the
     * LED never lights, regardless of the period/duty we program below. */
    PWM_CH0->STATUS = PWM_CH0_GATE_ON;
    PWM_CH0->RESERVED_HW[0] = PWM_CH0_GATE_ON;
    PWM_CH0->RESERVED_HW[1] = PWM_CH0_GATE_ON;
    PWM_CH0->RESERVED_HW[2] = PWM_CH0_GATE_ON;
}

void backlight_terminate(void)
{
    pwm_program(PWM_CH0, BACKLIGHT_PWM_HZ, 0); /* duty 0 = dark */
}

void display_setBacklightLevel(uint8_t level)
{
    if (level > 100)
        level = 100;

    /* Keep the PWM running and zero the duty when off: halting the counter
     * would hold the output pin at its last (possibly high) level, leaving
     * the LED lit. */
    pwm_program(PWM_CH0, BACKLIGHT_PWM_HZ, (unsigned)level * 100u);
}
