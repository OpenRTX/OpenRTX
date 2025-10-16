/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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

