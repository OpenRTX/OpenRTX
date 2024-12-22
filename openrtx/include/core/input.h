/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Silvano Seva IU2KWO,                     *
 *                                Grzegorz Kaczmarek SP6HFE                *
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

#ifndef INPUT_H
#define INPUT_H

#include "interfaces/keyboard.h"
#include <inttypes.h>
#include <stdbool.h>

/**
 * Max number of keys pressed allowed
 */
static const uint8_t input_maxKeysForMultiPress = 3;

/**
 * Time interval in milliseconds after which a keypress is considered a long-press
 */
static const uint16_t input_longPressTimeout = 700;

/**
 * Time interval in milliseconds for repeat event generation after long-press was detected
 */
static const uint16_t input_repeatInterval = 250;

/**
 * This enum describes the key event type:
 * - KEY_EVENT_PRESS a button(s) press
 * - KEY_EVENT_SINGLE_LONG_PRESS single button pressed long time
 * - KEY_EVENT_SINGLE_REPEAT single button continuously pressed (more than long press)
 * - KEY_EVENT_MULTI_LONG_PRESS over 1 button pressed together long time (since last press)
 */
enum keyEventType_t
{
    KEY_EVENT_PRESS             = 0,
    KEY_EVENT_SINGLE_LONG_PRESS = 1,
    KEY_EVENT_SINGLE_REPEAT     = 2,
    KEY_EVENT_MULTI_LONG_PRESS  = 3
};

/**
 * Structure that represents a keyboard event payload
 * The maximum size of an event payload is 30 bits
 * For a keyboard events we use 2 bits
 * And the remaining 28 bits to communicate currently pressed keys
 */
typedef union
{
    struct
    {
        uint32_t event    : 2,
                 keys     : 28,
                 _padding : 2;
    };

    uint32_t value;
}
kbd_msg_t;

/**
 * Turn on or off a support of press event generation on
 * new key pressed while 1+ keys pressed at the time
 * This is used by UI when some key/s stay pressed while
 * other/s change
 * 
 * @param isEnabled: feature enabling flag
 */
void input_enableMultiKeyPressEvent(bool isEnabled);

/**
 * Looks up knob events only in provided keyboard_t data
 * Useful in a situation where no real keys are pressed
 * (keys detection is handled separately) 
 * 
 * @param keys keys pressed
 * @param msg keyboard event message to be filled
 * @return true msg contain valid event message
 * @return false msg contents are not valid
 */
bool input_handleKnobEvents(keyboard_t keys, kbd_msg_t* msg);

/**
 * Scan all the keyboard buttons to detect possible keypresses filling a
 * keyboard event data structure. The function returns true if a keyboard event
 * has been detected.
 *
 * @param timestamp: system time when keyboard scanning takes place.
 * @param msg: keyboard event message to be filled.
 * @return true if a keyboard event has been detected, false otherwise.
 */
bool input_scanKeyboard(long long timestamp, kbd_msg_t *msg);

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
