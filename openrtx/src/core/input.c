/***************************************************************************
 *   Copyright (C) 2020 - 2024 by Federico Amedeo Izzo IU2NUO,             *
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

#include <input.h>
#include <interfaces/delays.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

bool pressEventAllowedForMultipleKeys;

void input_allowPressEventOnMultilpeKeysPressed(bool isPressEventAllowed)
{
    pressEventAllowedForMultipleKeys = isPressEventAllowed;
}

bool processKnobMovementDetection(keyboard_t keys, kbd_msg_t* msg)
{
    const bool isKeyboardEvent = true;      // Return value
    const bool noKeyboardEvent = false;     // Return value

    if (msg == NULL)
    {
        return noKeyboardEvent;
    }

    if ((keys & KNOB_LEFT) || (keys & KNOB_RIGHT))
    {
        // Leave only knob-related signals
        msg->keys = (keys & ((uint32_t)(KNOB_LEFT) | (uint32_t)(KNOB_RIGHT)));

        msg->event = KEY_EVENT_PRESS;
        return isKeyboardEvent;
    }

    return noKeyboardEvent;
}

bool input_scanKeyboard(long long timestamp, kbd_msg_t* msg)
{
    static keyboard_t previousKeys      = 0;     // Previously pressed keys
    static bool longPressReached        = false; // Long-press time was reached
    static bool keysReleaseDetected     = false; // One or more keys were released
    static bool keyboardReleaseAwaiting = false; // Awaiting for keyboard release
    static long long eventStart         = 0;     // Current timeout calculation start

    const bool isKeyboardEvent           = true;      // Return value
    const bool noKeyboardEvent           = false;     // Return value
    const keyboard_t keysWithoutKnobMask = 0x3FFFFFF; // Keys mask without knob signals

    const keyboard_t keys               = kbd_getKeys(); // Current keys pressed
    const uint8_t keysNumberWithoutKnob = (uint8_t)(__builtin_popcount(keys & keysWithoutKnobMask));

    msg->value = 0; // Init out message (no keys pressed)

    // Handle keyboard release (no keys pressed, no knob movement)
    if (keys == 0)
    {
        if (previousKeys != 0)
        {
            // Clear data
            previousKeys            = 0;
            keysReleaseDetected     = false;
            longPressReached        = false;
            keyboardReleaseAwaiting = false;

            // Send empty keyset once
            msg->event = KEY_EVENT_PRESS;
            return isKeyboardEvent;
        }

        return noKeyboardEvent;
    }

    // Keyboard release awaiting (knob movements are handled)
    if (keyboardReleaseAwaiting == true)
    {
        return processKnobMovementDetection(keys, msg);
    }

    // Too many keys pressed (knob movements are handled)
    if (keysNumberWithoutKnob > input_maxKeysForMultiPress)
    {
        keyboardReleaseAwaiting = true;

        return processKnobMovementDetection(keys, msg);
    }

    // Key events handling -> remember what's pressed
    msg->keys = keys;

    // Handle continuous pressing of the same key set (knob movement is not considered)
    if ((keys & keysWithoutKnobMask) == (previousKeys & keysWithoutKnobMask))
    {
        // Key release was detected during multi-press (knob movements are handled)
        if (keysReleaseDetected == true)
        {
            return processKnobMovementDetection(keys, msg);
        }

        // Calculate timings
        bool longPressDetected = (!longPressReached &&
                             ((timestamp - eventStart) >= input_longPressTimeout));

        bool repeatDetected = (longPressReached &&
                          ((timestamp - eventStart) >= input_repeatInterval));

        // Long press event
        if (longPressDetected)
        {
            longPressReached = true;
            eventStart       = timestamp;
            msg->event       = ((keysNumberWithoutKnob > 1) ? KEY_EVENT_MULTI_LONG_PRESS : KEY_EVENT_SINGLE_LONG_PRESS);

            return isKeyboardEvent;
        }

        // Repeat event (reserved for single key press only)
        if (repeatDetected && (keysNumberWithoutKnob == 1))
        {
            eventStart = timestamp;
            msg->event = KEY_EVENT_SINGLE_REPEAT;

            return isKeyboardEvent;
        }

        // No long press, no repeat (knob movements are handled)
        return processKnobMovementDetection(keys, msg);
    }

    // Handle change of the amount of keys pressed (knob movement is not considered)
    uint8_t previousKeysNumberWithoutKnob = (uint8_t)(__builtin_popcount(previousKeys & keysWithoutKnobMask));

    // Additional button(s) pressed (still within allowed amount)
    if (keysNumberWithoutKnob > previousKeysNumberWithoutKnob)
    {
        keysReleaseDetected = false;
        previousKeys        = keys;

        // Restart timings calculation
        eventStart       = timestamp;
        longPressReached = false;

        if((keysNumberWithoutKnob == 1) || (pressEventAllowedForMultipleKeys == true))
        {
            msg->event = KEY_EVENT_PRESS;

            return isKeyboardEvent;
        }

        // No event for multi-press when pressEventAllowedForMultipleKeys == false
        return noKeyboardEvent;
    }

    // Some buttons were released (still at least 1 is pressed)
    if (keysNumberWithoutKnob < previousKeysNumberWithoutKnob)
    {
        keysReleaseDetected = true;
        previousKeys        = keys;

        // No event on buttons release
        return noKeyboardEvent;
    }

    // In case code reached here there is certainly no keyboard event
    return noKeyboardEvent;
}

bool input_isNumberPressed(kbd_msg_t msg)
{
    return msg.keys & KBD_NUM_MASK;
}

bool input_isCharPressed(kbd_msg_t msg)
{
    return msg.keys & KBD_CHAR_MASK;
}

uint8_t input_getPressedNumber(kbd_msg_t msg)
{
    uint32_t masked_input = msg.keys & KBD_NUM_MASK;
    if (masked_input == 0)
        return 0;

    return __builtin_ctz(msg.keys & KBD_NUM_MASK);
}

uint8_t input_getPressedChar(kbd_msg_t msg)
{
    uint32_t masked_input = msg.keys & KBD_CHAR_MASK;
    if (masked_input == 0)
        return 0;

    return __builtin_ctz(msg.keys & KBD_CHAR_MASK);
}
