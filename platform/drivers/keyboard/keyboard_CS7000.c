/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
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

#include <stdint.h>
#include <peripherals/gpio.h>
#include <interfaces/delays.h>
#include <interfaces/keyboard.h>
#include <hwconfig.h>


/*
 * Keyboard map:
 *
 *           R902    R903    R904    R909    R926
 *         +-------+-------+-------+-------+-------+
 *         | Col 1 | Col 2 | Col 3 | Col 4 | Col 5 |
 * +-------+-------+-------+-------+-------+-------+
 * | Row 1 |   1   |   2   |   3   |   k1  |   k5  | R905
 * +-------+-------+-------+-------+-------+-------+
 * | Row 2 |   4   |   5   |   6   |   k2  |   k6  | R906
 * +-------+-------+-------+-------+-------+-------+
 * | Row 3 |   7   |   8   |   9   |   k3  |       | R907
 * +-------+-------+-------+-------+-------+-------+
 * | Row 4 |   *   |   0   |   #   |   k4  |       | R908
 * +-------+-------+-------+-------+-------+-------+
 *
 */


void kbd_init()
{
    gpio_setMode(KB_COL5,   INPUT_PULL_DOWN);
    gpio_setMode(SIDE_KEY1, INPUT);
    gpio_setMode(SIDE_KEY2, INPUT);
    gpio_setMode(SIDE_KEY3, INPUT);
    gpio_setMode(ALARM_KEY, INPUT);
}

void kbd_terminate()
{

}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;

    /*
     * Rows and columns lines are in common with the display, so we have to
     * configure them as inputs before scanning. However, before configuring them
     * as inputs, we put them as outputs and force a low logical level in order
     * to be sure that any residual charge on both the display controller's inputs
     * and in the capacitors in parallel to the Dx lines is dissipated.
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


    gpio_setMode(KB_COL1, INPUT_PULL_DOWN);
    gpio_setMode(KB_COL2, INPUT_PULL_DOWN);
    gpio_setMode(KB_COL3, INPUT_PULL_DOWN);
    gpio_setMode(KB_COL4, INPUT_PULL_DOWN);

    gpio_setPin(KB_ROW1);

    delayUs(10);
    if(gpio_readPin(KB_COL1) != 0) keys |= KEY_1;
    if(gpio_readPin(KB_COL2) != 0) keys |= KEY_2;
    if(gpio_readPin(KB_COL3) != 0) keys |= KEY_3;
    if(gpio_readPin(KB_COL4) != 0) keys |= KEY_ENTER;
    if(gpio_readPin(KB_COL5) != 0) keys |= KEY_UP;

    gpio_clearPin(KB_ROW1);
    gpio_setPin(KB_ROW2);

    delayUs(10);
    if(gpio_readPin(KB_COL1) != 0) keys |= KEY_4;
    if(gpio_readPin(KB_COL2) != 0) keys |= KEY_5;
    if(gpio_readPin(KB_COL3) != 0) keys |= KEY_6;
    if(gpio_readPin(KB_COL4) != 0) keys |= KEY_ESC;
    if(gpio_readPin(KB_COL5) != 0) keys |= KEY_DOWN;

    gpio_clearPin(KB_ROW2);
    gpio_setPin(KB_ROW3);

    delayUs(10);
    if(gpio_readPin(KB_COL1) != 0) keys |= KEY_7;
    if(gpio_readPin(KB_COL2) != 0) keys |= KEY_8;
    if(gpio_readPin(KB_COL3) != 0) keys |= KEY_9;
    if(gpio_readPin(KB_COL4) != 0) keys |= KEY_F4;

    gpio_clearPin(KB_ROW3);
    gpio_setPin(KB_ROW4);

    delayUs(10);
    if(gpio_readPin(KB_COL1) != 0) keys |= KEY_STAR;
    if(gpio_readPin(KB_COL2) != 0) keys |= KEY_0;
    if(gpio_readPin(KB_COL3) != 0) keys |= KEY_HASH;
    if(gpio_readPin(KB_COL4) != 0) keys |= KEY_F5;

    gpio_clearPin(KB_ROW4);

    if(gpio_readPin(SIDE_KEY1) == 0) keys |= KEY_MONI;
    if(gpio_readPin(SIDE_KEY2) == 0) keys |= KEY_F1;
    if(gpio_readPin(SIDE_KEY3) == 0) keys |= KEY_F2;
    if(gpio_readPin(ALARM_KEY) == 0) keys |= KEY_F3;

    return keys;
}
