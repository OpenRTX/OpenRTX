/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#include "peripherals/gpio.h"
#include "interfaces/delays.h"
#include "interfaces/keyboard.h"
#include "hwconfig.h"

void kbd_init()
{
    gpio_setMode(KB_ROW0, INPUT_PULL_UP);
    gpio_setMode(KB_ROW1, INPUT_PULL_UP);
    gpio_setMode(KB_ROW2, INPUT_PULL_UP);
    gpio_setMode(KB_ROW3, INPUT_PULL_UP);
    gpio_setMode(KB_ROW4, INPUT_PULL_UP);
    gpio_setMode(KB_COL0, OUTPUT);
    gpio_setMode(KB_COL1, OUTPUT);
    gpio_setMode(KB_COL2, OUTPUT);
    gpio_setMode(KB_COL3, OUTPUT);

    gpio_setMode(FUNC_SW, INPUT);
    gpio_setMode(FUNC2_SW, INPUT);
    gpio_setMode(MONI_SW, INPUT);

}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;

    gpio_clearPin(KB_COL0);
    delayUs(1);
    if(gpio_readPin(KB_ROW0) == 0) keys |= KEY_1;
    if(gpio_readPin(KB_ROW1) == 0) keys |= KEY_2;
    if(gpio_readPin(KB_ROW2) == 0) keys |= KEY_3;
    if(gpio_readPin(KB_ROW3) == 0) keys |= KEY_ENTER;
    #ifdef PLATFORM_DM1801
    if(gpio_readPin(KB_ROW4) == 0) keys |= KEY_F4;
    #else
    if(gpio_readPin(KB_ROW4) == 0) keys |= KEY_RIGHT;
    #endif
    gpio_setPin(KB_COL0);

    gpio_clearPin(KB_COL1);
    delayUs(1);
    if(gpio_readPin(KB_ROW0) == 0) keys |= KEY_4;
    if(gpio_readPin(KB_ROW1) == 0) keys |= KEY_5;
    if(gpio_readPin(KB_ROW2) == 0) keys |= KEY_6;
    if(gpio_readPin(KB_ROW3) == 0) keys |= KEY_UP;
    #ifdef PLATFORM_DM1801
    if(gpio_readPin(KB_ROW4) == 0) keys |= KEY_F3;
    #else
    if(gpio_readPin(KB_ROW4) == 0) keys |= KEY_LEFT;
    #endif
    gpio_setPin(KB_COL1);

    gpio_clearPin(KB_COL2);
    delayUs(1);
    if(gpio_readPin(KB_ROW0) == 0) keys |= KEY_7;
    if(gpio_readPin(KB_ROW1) == 0) keys |= KEY_8;
    if(gpio_readPin(KB_ROW2) == 0) keys |= KEY_9;
    if(gpio_readPin(KB_ROW3) == 0) keys |= KEY_DOWN;
    #ifdef PLATFORM_DM1801
    if(gpio_readPin(KB_ROW4) == 0) keys |= KEY_RIGHT;
    #endif
    gpio_setPin(KB_COL2);

    gpio_clearPin(KB_COL3);
    delayUs(1);
    if(gpio_readPin(KB_ROW0) == 0) keys |= KEY_STAR;
    if(gpio_readPin(KB_ROW1) == 0) keys |= KEY_0;
    if(gpio_readPin(KB_ROW2) == 0) keys |= KEY_HASH;
    if(gpio_readPin(KB_ROW3) == 0) keys |= KEY_ESC;
    #ifdef PLATFORM_DM1801
    if(gpio_readPin(KB_ROW4) == 0) keys |= KEY_LEFT;
    #endif
    gpio_setPin(KB_COL3);

    if(gpio_readPin(FUNC_SW) == 0)  keys |= KEY_F1;
    if(gpio_readPin(FUNC2_SW) == 0) keys |= KEY_F2;
    if(gpio_readPin(MONI_SW) == 0)  keys |= KEY_MONI;

    return keys;
}

