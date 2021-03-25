/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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
    gpio_setMode(KB_COL1, INPUT_PULL_UP);
    gpio_setMode(KB_COL2, INPUT_PULL_UP);
    gpio_setMode(KB_COL3, INPUT_PULL_UP);
    gpio_setMode(KB_COL4, INPUT_PULL_UP);

    gpio_setMode(KB_ROW1, OUTPUT);
    gpio_setMode(KB_ROW2, OUTPUT);
    gpio_setMode(KB_ROW3, OUTPUT);

    gpio_setPin(KB_ROW1);
    gpio_setPin(KB_ROW2);
    gpio_setPin(KB_ROW3);
}

void kbd_terminate()
{
    /* Back to default state */
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
    * Mapping of front buttons:
    *
    *       +------+-----+-------+-----+
    *       | PD0  | PD1 |  PE0  | PE1 |
    * +-----+------+-----+-------+-----+
    * | PD2 | ENT  |     |   P1  | P4  |
    * +-----+------+-----+-------+-----+
    * | PD3 | down |     |  red  | P3  |
    * +-----+------+-----+-------+-----+
    * | PD4 | ESC  | up  | green | P2  |
    * +-----+------+-----+-------+-----+
    *
    * The coloumn lines have pull-up resistors, thus the detection of a button
    * press follows an active-low logic.
    *
    */

    gpio_clearPin(KB_ROW1);

    delayUs(10);
    if(gpio_readPin(KB_COL1) == 0) keys |= KEY_ENTER;
    if(gpio_readPin(KB_COL3) == 0) keys |= KEY_F1;
    if(gpio_readPin(KB_COL4) == 0) keys |= KEY_F4;
    gpio_setPin(KB_ROW1);

    /* Row 2: button on col. 3 ("red") is not mapped */
    gpio_clearPin(KB_ROW2);
    delayUs(10);

    if(gpio_readPin(KB_COL1) == 0) keys |= KEY_DOWN;
    if(gpio_readPin(KB_COL4) == 0) keys |= KEY_F3;
    gpio_setPin(KB_ROW2);

    /* Row 3: button on col. 3 ("green") is not mapped */
    gpio_clearPin(KB_ROW3);
    delayUs(10);

    if(gpio_readPin(KB_COL1) == 0) keys |= KEY_ESC;
    if(gpio_readPin(KB_COL2) == 0) keys |= KEY_UP;
    if(gpio_readPin(KB_COL4) == 0) keys |= KEY_F2;
    gpio_setPin(KB_ROW3);

    return keys;
}
