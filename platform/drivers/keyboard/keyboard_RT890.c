/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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
#include <peripherals/gpio.h>
#include <interfaces/delays.h>
#include <interfaces/keyboard.h>
#include <interfaces/platform.h>
#include "hwconfig.h"

static int8_t old_pos = 0;

void kbd_init()
{
    /* Set the two column lines as outputs */
    gpio_setMode(KB_COL0, OUTPUT);
    gpio_setMode(KB_COL1, OUTPUT);
    gpio_setMode(KB_COL2, OUTPUT);
    gpio_setMode(KB_COL3, OUTPUT);
    gpio_setMode(KB_ROW0, INPUT_PULL_UP);
    gpio_setMode(KB_ROW1, INPUT_PULL_UP);
    gpio_setMode(KB_ROW2, INPUT_PULL_UP);
    gpio_setMode(KB_ROW3, INPUT_PULL_UP);
    // gpio_setPin(KB_COL0);
    // gpio_setPin(KB_COL1);
    // gpio_setPin(KB_COL2);
    // gpio_setPin(KB_COL3);
}

void kbd_terminate()
{
    /* Back to default state */
    gpio_clearPin(KB_COL0);
    gpio_clearPin(KB_COL1);
    gpio_clearPin(KB_COL2);
    gpio_clearPin(KB_COL3);
    gpio_setMode(KB_COL0, INPUT);
    gpio_setMode(KB_COL1, INPUT);
    gpio_setMode(KB_COL2, INPUT);
    gpio_setMode(KB_COL3, INPUT);
}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;

    if(gpio_readPin(SKB_1)) keys |= KEY_MONI;
    if(gpio_readPin(SKB_2)) keys |= KEY_F1;

    /*
     * Scan keyboard by rows. Each row is set to high, and then
     * the columns are read. If a key is pressed, the corresponding column
     * will be pulled low.
     */

    gpio_setPin(KB_COL0);

    delayUs(10);
    if(gpio_readPin(KB_ROW0)) keys |= KEY_ENTER;
    if(gpio_readPin(KB_ROW1)) keys |= KEY_UP;
    if(gpio_readPin(KB_ROW2)) keys |= KEY_DOWN;
    if(gpio_readPin(KB_ROW3)) keys |= KEY_ESC;

    gpio_clearPin(KB_COL0);
    gpio_setPin(KB_COL1);

    delayUs(10);
    if(gpio_readPin(KB_ROW0)) keys |= KEY_1;
    if(gpio_readPin(KB_ROW1)) keys |= KEY_2;
    if(gpio_readPin(KB_ROW2)) keys |= KEY_3;
    if(gpio_readPin(KB_ROW3)) keys |= KEY_STAR;

    gpio_clearPin(KB_COL1);
    gpio_setPin(KB_COL2);

    delayUs(10);
    if(gpio_readPin(KB_ROW0)) keys |= KEY_4;
    if(gpio_readPin(KB_ROW1)) keys |= KEY_5;
    if(gpio_readPin(KB_ROW2)) keys |= KEY_6;
    if(gpio_readPin(KB_ROW3)) keys |= KEY_0;

    gpio_clearPin(KB_COL2);
    gpio_setPin(KB_COL3);

    delayUs(10);
    if(gpio_readPin(KB_ROW0)) keys |= KEY_7;
    if(gpio_readPin(KB_ROW1)) keys |= KEY_8;
    if(gpio_readPin(KB_ROW2)) keys |= KEY_9;
    if(gpio_readPin(KB_ROW3)) keys |= KEY_HASH;

    return keys;
}
