/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdint.h>
#include "peripherals/gpio.h"
#include "interfaces/delays.h"
#include "interfaces/keyboard.h"
#include "interfaces/platform.h"
#include "hwconfig.h"

static int8_t old_pos = 0;

void kbd_init()
{
    /* Set the two row lines as outputs */
    gpio_setMode(KB_ROW1, OUTPUT);
    gpio_setMode(KB_ROW2, OUTPUT);
    gpio_setMode(KB_ROW3, OUTPUT);
    gpio_clearPin(KB_ROW1);
    gpio_clearPin(KB_ROW2);
    gpio_clearPin(KB_ROW3);

    /* Initialise old position */
    old_pos = platform_getChSelector();
}

void kbd_terminate()
{
    /* Back to default state */
    gpio_clearPin(KB_ROW1);
    gpio_clearPin(KB_ROW2);
    gpio_clearPin(KB_ROW3);
    gpio_setMode(KB_ROW1, INPUT);
    gpio_setMode(KB_ROW2, INPUT);
    gpio_setMode(KB_ROW3, INPUT);
}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;

    /* Read channel knob to send KNOB_LEFT and KNOB_RIGHT events */
    int8_t new_pos = platform_getChSelector();
    if (old_pos != new_pos)
    {
        int8_t diff = new_pos - old_pos;
        if (diff < 0)
            keys |= KNOB_LEFT;
        else if (diff > 0)
            keys |= KNOB_RIGHT;
        else
        {
            if (old_pos < 0)
                keys |= KNOB_LEFT;
            else
                keys |= KNOB_RIGHT;
        }
        old_pos = new_pos;
    }


    /*
     * The row lines are in common with the display, so we have to configure
     * them as inputs before scanning. However, before configuring them as inputs,
     * we put them as outputs and force a low logical level in order to be sure
     * that any residual charge on both the display controller's inputs and in
     * the capacitors in parallel to the Dx lines is dissipated.
     */
    gpio_setMode(LCD_D0, OUTPUT);
    gpio_setMode(LCD_D1, OUTPUT);
    gpio_setMode(LCD_D2, OUTPUT);
    gpio_setMode(LCD_D3, OUTPUT);
    gpio_setMode(LCD_D4, OUTPUT);
    gpio_setMode(LCD_D5, OUTPUT);
    gpio_setMode(LCD_D6, OUTPUT);
    gpio_setMode(LCD_D7, OUTPUT);

    gpio_clearPin(LCD_D0);
    gpio_clearPin(LCD_D1);
    gpio_clearPin(LCD_D2);
    gpio_clearPin(LCD_D3);
    gpio_clearPin(LCD_D4);
    gpio_clearPin(LCD_D5);
    gpio_clearPin(LCD_D6);
    gpio_clearPin(LCD_D7);


    gpio_setMode(LCD_D0, INPUT_PULL_DOWN);
    gpio_setMode(LCD_D1, INPUT_PULL_DOWN);
    gpio_setMode(LCD_D2, INPUT_PULL_DOWN);
    gpio_setMode(LCD_D3, INPUT_PULL_DOWN);
    gpio_setMode(LCD_D4, INPUT_PULL_DOWN);
    gpio_setMode(LCD_D5, INPUT_PULL_DOWN);
    gpio_setMode(LCD_D6, INPUT_PULL_DOWN);
    gpio_setMode(LCD_D7, INPUT_PULL_DOWN);

    /*
     * Scan keyboard by coloumns.
     * For key configuration, see: https://www.qsl.net/dl4yhf/RT3/md380_hw.html#keyboard
     *
     * Keys coloumns (LCD_D...) have 1k series resistor and a 10pF capacitor
     * connected to ground, making a low-pass filter with a settling time of
     * ~50ns. CPU runs at 168MHz and 50ns are approximately eigth instructions,
     * this means that we have to put a small delay before reading the GPIOs to
     * allow voltage to settle.
     */
    gpio_setPin(KB_ROW1);

    delayUs(10);
    if(gpio_readPin(LCD_D7)) keys |= KEY_STAR;
    if(gpio_readPin(LCD_D2)) keys |= KEY_3;
    if(gpio_readPin(LCD_D1)) keys |= KEY_2;
    if(gpio_readPin(LCD_D0)) keys |= KEY_1;
    if(gpio_readPin(LCD_D6)) keys |= KEY_0;
    if(gpio_readPin(LCD_D5)) keys |= KEY_6;
    if(gpio_readPin(LCD_D4)) keys |= KEY_5;
    if(gpio_readPin(LCD_D3)) keys |= KEY_4;

    gpio_clearPin(KB_ROW1);
    gpio_setPin(KB_ROW2);

    delayUs(10);
    if(gpio_readPin(LCD_D7)) keys |= KEY_ESC;
    if(gpio_readPin(LCD_D2)) keys |= KEY_DOWN;
    if(gpio_readPin(LCD_D1)) keys |= KEY_UP;
    if(gpio_readPin(LCD_D0)) keys |= KEY_ENTER;
    if(gpio_readPin(LCD_D6)) keys |= KEY_HASH;
    if(gpio_readPin(LCD_D5)) keys |= KEY_9;
    if(gpio_readPin(LCD_D4)) keys |= KEY_8;
    if(gpio_readPin(LCD_D3)) keys |= KEY_7;

    gpio_clearPin(KB_ROW2);
    gpio_setPin(KB_ROW3);

    delayUs(10);
    if(gpio_readPin(FUNC_SW)) keys |= KEY_F1;
    if(gpio_readPin(MONI_SW)) keys |= KEY_MONI;

    gpio_clearPin(KB_ROW3);
    return keys;
}
