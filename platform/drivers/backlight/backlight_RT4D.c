/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <hwconfig.h>
#include <interfaces/platform.h>
#include <peripherals/gpio.h>
#include "backlight.h"

void backlight_init()
{
    gpio_setMode(LCD_BACKLIGHT, ALTERNATE | ALTERNATE_FUNC(3));
    CRM->apb2en_bit.tmr9en = TRUE;
    CRM->ahben1_bit.gpioaen = TRUE;

    uint32_t frequency = 200000;
    uint32_t period = ((144000000 / 2) / frequency) - 1;

    TMR9->pr_bit.pr = period;
    TMR9->div_bit.div = 0x02;
    TMR9->swevt_bit.ovfswtr = TRUE;
    TMR9->ctrl1_bit.clkdiv = TMR_CLOCK_DIV1;
    TMR9->cm1_output_bit.c2octrl = TMR_OUTPUT_CONTROL_PWM_MODE_A;
    TMR9->cctrl_bit.c2p = TMR_OUTPUT_ACTIVE_HIGH;
    TMR9->cctrl_bit.c2en = TRUE;                     // Enable channel
    TMR9->c2dt_bit.c2dt = (50 * (period - 1)) / 100; // Set duty cycles
    TMR9->cm1_output_bit.c2oben = TRUE;
    TMR9->ctrl1_bit.prben = TRUE;
    TMR9->ctrl1_bit.tmren = TRUE; // Start timer
    TMR9->brk_bit.oen = TRUE;     // Enable timer
}

void backlight_terminate()
{
    gpio_clearPin(LCD_BACKLIGHT);
}

/*
 * This function is defined in display.h
 */
void display_setBacklightLevel(uint8_t level)
{
    if (level > 100)
        level = 100;

    uint32_t frequency = 200000;
    uint32_t period = ((144000000 / 2) / frequency) - 1;
    TMR9->c2dt_bit.c2dt = (level * (period - 1)) / 100; // Set duty cycles
}
