/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#include <stdio.h>
#include <stdint.h>
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <interfaces/keyboard.h>
#include <interfaces/platform.h>
#include "hwconfig.h"

void kbd_init()
{
    /* Set the two row lines as outputs */
    gpio_setMode(KB_ROW1, OUTPUT);
    gpio_setMode(KB_ROW2, OUTPUT);
    gpio_setMode(KB_ROW3, OUTPUT);
    gpio_clearPin(KB_ROW1);
    gpio_clearPin(KB_ROW2);
    gpio_clearPin(KB_ROW3);
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

    /* Use absolute position knob to emulate left and right buttons */
    static uint8_t old_pos = 0;
    uint8_t new_pos = platform_getChSelector();
    if (old_pos != new_pos)
    {
        if (new_pos < old_pos)
            keys |= KEY_LEFT;
        else
            keys |= KEY_RIGHT;
        old_pos = new_pos;
    }

    /*
     * First of all, configure the row lines as inputs. Since they are in common
     * with the display, their configuration can have been screwed up by display
     * driver among two subsequent calls of this function.
     */
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

    delayUs(1);
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

    delayUs(1);
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

    delayUs(1);
    if(gpio_readPin(FUNC_SW)) keys |= KEY_F1;
    if(gpio_readPin(MONI_SW)) keys |= KEY_MONI;

    gpio_clearPin(KB_ROW3);
    return keys;
}
