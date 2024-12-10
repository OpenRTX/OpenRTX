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

bool input_scanKeyboard(long long timestamp, kbd_msg_t* msg)
{
    static bool longPressReached   = false;  // asdadasd
    static keyboard_t previousKeys = 0;      // Previous keyboard status
    static long long eventStart    = 0;      // asdasd
    static bool keyboardReleaseAwaiting = false;

    const bool isKeyboardEvent           = true;
    const bool noKeyboardEvent           = false;
    const keyboard_t keysWithoutKnobMask = 0x3FFFFFF;

    const keyboard_t keys               = kbd_getKeys();
    const uint8_t keysNumberWithoutKnob = (uint8_t)(__builtin_popcount(keys & keysWithoutKnobMask));

    msg->value = 0;

    // Handle keyboard release (no keys pressed, no knob movement)
    if (keys == 0)
    {
        if (previousKeys != 0)
        {
            // Clear data
            longPressReached        = false;
            previousKeys            = 0;
            keyboardReleaseAwaiting = false;
        }

        return noKeyboardEvent;
    }

    // No event until keyboard is released (in certain conditions)
    if (keyboardReleaseAwaiting == true)
    {
        return noKeyboardEvent;
    }

    // Remember pressed keys
    msg->keys = keys;

    // Handle knob movement only (keys is != 0)
    if (keysNumberWithoutKnob == 0)
    {
        msg->event = KEY_EVENT_SINGLE_PRESS;
        return isKeyboardEvent;
    }

    // Handle continuous pressing of the same key set (knob movement is not considered)
    if ((keys & keysWithoutKnobMask) == (previousKeys & keysWithoutKnobMask))
    {
        // When knob was turned we want only that event to go through (to not wrongly repeat the keys)
        if ((keys & KNOB_LEFT) || (keys & KNOB_RIGHT))
        {
            msg->keys &= ~keysWithoutKnobMask;
            msg->event = KEY_EVENT_SINGLE_PRESS;
            return isKeyboardEvent;
        }

        // Calculate timings
        bool longPressDetected = (!longPressReached &&
                             ((timestamp - eventStart) >= input_longPressTimeout));

        bool repeatDetected = (longPressReached &&
                          ((timestamp - eventStart) >= input_repeatInterval));

        // Long press event is possible for single and multi key presses
        if (longPressDetected)
        {
            longPressReached = true;
            eventStart       = timestamp;
            msg->event       = ((keysNumberWithoutKnob > 1) ? KEY_EVENT_MULTI_LONG_PRESS : KEY_EVENT_SINGLE_LONG_PRESS);

            return isKeyboardEvent;
        }

        // Repeat event is reserved for single key press only
        if (repeatDetected && (keysNumberWithoutKnob == 1))
        {
            eventStart = timestamp;
            msg->event = KEY_EVENT_SINGLE_REPEAT;

            return isKeyboardEvent;
        }
    }

    // Handle pressed keys set change (knob movement is not considered)
    uint8_t previousKeysNumberWithoutKnob = (uint8_t)(__builtin_popcount(previousKeys & keysWithoutKnobMask));

    switch (previousKeysNumberWithoutKnob)
    {
        // Keyboard was released, now something is pressed
        case 0:
        /* fallthrough */
        // Single press was before
        case 1:
            eventStart   = timestamp;
            previousKeys = keys;

            if (keysNumberWithoutKnob > 1)
            {
                // Multi press started - waiting for long press timeout
                return noKeyboardEvent;
            }
            else
            {
                // Single press (keysNumberWithoutKnob == 1)
                msg->event = KEY_EVENT_SINGLE_REPEAT;
                return isKeyboardEvent;
            }
            break;
        // Multi press was before
        default:
            if (keysNumberWithoutKnob > 1)
            {
                if (keysNumberWithoutKnob >= previousKeysNumberWithoutKnob)
                {
                    // Multi press keys modified (keys pressed number increased or same)
                    eventStart   = timestamp;
                    previousKeys = keys;
                    return noKeyboardEvent;
                }
                else
                {
                    // Keys pressed number decreased
                    keyboardReleaseAwaiting = true;
                    return noKeyboardEvent;
                }
            }
            else
            {
                // Can't go back to single press - need to release keyboard first
                keyboardReleaseAwaiting = true;
                return noKeyboardEvent;
            }
            break;
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
