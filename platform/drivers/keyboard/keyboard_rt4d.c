/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <interfaces/keyboard.h>
#include <interfaces/platform.h>
#include <peripherals/gpio.h>
#include <hwconfig.h>
#include <interfaces/delays.h>

void kbd_init()
{
    gpio_setMode(KBD_COL1, OUTPUT);
    gpio_setMode(KBD_COL2, OUTPUT);
    gpio_setMode(KBD_COL3, OUTPUT);
    gpio_setMode(KBD_COL4, OUTPUT);

    gpio_setMode(KBD_ROW1, INPUT_PULL_UP);
    gpio_setMode(KBD_ROW2, INPUT_PULL_UP);
    gpio_setMode(KBD_ROW3, INPUT_PULL_UP);
    gpio_setMode(KBD_ROW4, INPUT_PULL_UP);
}

void kbd_terminate()
{
    /* Back to default state */
    gpio_clearPin(KBD_COL1);
    gpio_clearPin(KBD_COL2);
    gpio_clearPin(KBD_COL3);
    gpio_clearPin(KBD_COL4);
    gpio_setMode(KBD_COL1, INPUT);
    gpio_setMode(KBD_COL2, INPUT);
    gpio_setMode(KBD_COL3, INPUT);
    gpio_setMode(KBD_COL4, INPUT);
}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;
    bool skip_row1 = false;
    bool skip_row2 = false;
    bool skip_row3 = false;

    if (!gpio_readPin(KBD_ROW3)) {
        keys |= KEY_MONI;
        skip_row3 = true;
    }
    if (!gpio_readPin(KBD_ROW2)) {
        keys |= KEY_F1;
        skip_row2 = true;
    }
    if (!gpio_readPin(KBD_ROW1)) {
        keys |= KEY_F2;
        skip_row1 = true;
    }

    gpio_clearPin(KBD_COL1);

    delayUs(10);
    if (!gpio_readPin(KBD_ROW1) && !skip_row1)
        keys |= KEY_ENTER;
    if (!gpio_readPin(KBD_ROW2) && !skip_row2)
        keys |= KEY_1;
    if (!gpio_readPin(KBD_ROW3) && !skip_row3)
        keys |= KEY_4;
    if (!gpio_readPin(KBD_ROW4))
        keys |= KEY_7;

    gpio_setPin(KBD_COL1);
    gpio_clearPin(KBD_COL2);

    delayUs(10);
    if (!gpio_readPin(KBD_ROW1) && !skip_row1)
        keys |= KEY_UP;
    if (!gpio_readPin(KBD_ROW2) && !skip_row2)
        keys |= KEY_2;
    if (!gpio_readPin(KBD_ROW3) && !skip_row3)
        keys |= KEY_5;
    if (!gpio_readPin(KBD_ROW4))
        keys |= KEY_8;

    gpio_setPin(KBD_COL2);
    gpio_clearPin(KBD_COL3);

    delayUs(10);
    if (!gpio_readPin(KBD_ROW1) && !skip_row1)
        keys |= KEY_DOWN;
    if (!gpio_readPin(KBD_ROW2) && !skip_row2)
        keys |= KEY_3;
    if (!gpio_readPin(KBD_ROW3) && !skip_row3)
        keys |= KEY_6;
    if (!gpio_readPin(KBD_ROW4))
        keys |= KEY_9;

    gpio_setPin(KBD_COL3);
    gpio_clearPin(KBD_COL4);

    delayUs(10);
    if (!gpio_readPin(KBD_ROW1) && !skip_row1)
        keys |= KEY_ESC;
    if (!gpio_readPin(KBD_ROW2) && !skip_row2)
        keys |= KEY_STAR;
    if (!gpio_readPin(KBD_ROW3) && !skip_row3)
        keys |= KEY_0;
    if (!gpio_readPin(KBD_ROW4))
        keys |= KEY_HASH;
    gpio_setPin(KBD_COL4);

    delayUs(10);

    return keys;
}