/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INPUT_H
#define INPUT_H

#include "interfaces/keyboard.h"
#include <inttypes.h>
#include <stdbool.h>

/**
 * Time interval in milliseconds after which a keypress is considered a long-press
 */
static const uint16_t input_longPressTimeout = 700;

/**
 * Structure that represents a keyboard event payload
 * The maximum size of an event payload is 30 bits
 * For a keyboard event we use 1 bit to signal a short or long press
 * And the remaining 29 bits to communicate currently pressed keys.
 */
typedef union
{
    struct
    {
        uint32_t long_press : 1,
                 keys       : 29,
                 _padding   : 2;
    };

    uint32_t value;
}
kbd_msg_t;


/**
 * Scan all the keyboard buttons to detect possible keypresses filling a
 * keyboard event data structure. The function returns true if a keyboard event
 * has been detected.
 *
 * @param msg: keyboard event message to be filled.
 * @return true if a keyboard event has been detected, false otherwise.
 */
bool input_scanKeyboard(kbd_msg_t *msg);

/**
 * This function returns true if at least one number is pressed on the
 * keyboard.
 *
 * @param msg: the keyboard queue message
 * @return true if at least a number is pressed on the keyboard
 */
bool input_isNumberPressed(kbd_msg_t msg);

/**
 * This function returns true if at least one character is pressed on the
 * keyboard.
 *
 * @param msg: the keyboard queue message
 * @return true if at least a char is pressed on the keyboard
 */
bool input_isCharPressed(kbd_msg_t msg);

/**
 * This function returns the smallest number that is pressed on the keyboard,
 * 0 if none is pressed.
 *
 * @param msg: the keyboard queue message
 * @return the smalled pressed number on the keyboard
 */
uint8_t input_getPressedNumber(kbd_msg_t msg);

/**
 * This function returns the smallest number pressed on the keyboard and
 * associated to character. If no button is pressed, zero is returned.
 *
 * @param msg: the keyboard queue message
 * @return the smallest number associated to a char.
 */
uint8_t input_getPressedChar(kbd_msg_t msg);

#endif /* INPUT_H */
