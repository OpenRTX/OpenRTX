/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/delays.h"
#include <inttypes.h>
#include <stdbool.h>
#include "core/input.h"

static long long keyTs[KBD_NUM_KEYS]; // Timestamp of each keypress
static uint32_t longPressSent;        // Flags to manage long-press events
static keyboard_t prevKeys = 0;       // Previous keyboard status

// Keys currently eligible for auto-repeat while held, see
// input_setRepeatableKeys()
static keyboard_t repeatableKeys = INPUT_DEFAULT_REPEATABLE_KEYS;

void input_setRepeatableKeys(keyboard_t mask)
{
    repeatableKeys = mask;
}

bool input_scanKeyboard(kbd_msg_t *msg)
{
    msg->value = 0;
    bool kbd_event = false;

    keyboard_t keys = kbd_getKeys();
    long long now = getTick();

    // The key status has changed
    if (keys != prevKeys) {
        // Find newly pressed keys
        keyboard_t newKeys = (keys ^ prevKeys) & keys;

        // Save timestamp
        for (uint8_t k = 0; k < KBD_NUM_KEYS; k++) {
            keyboard_t mask = 1 << k;
            if ((newKeys & mask) != 0) {
                keyTs[k] = now;
                longPressSent &= ~mask;
            }
        }

        // New keyboard event
        msg->keys = keys;
        kbd_event = true;
    }
    // Some key is kept pressed
    else if (keys != 0) {
        // Check for saved timestamp to trigger long-presses and, once a key
        // has been held past the long-press threshold, keep generating
        // auto-repeat events at a fixed rate for as long as it stays pressed.
        for (uint8_t k = 0; k < KBD_NUM_KEYS; k++) {
            keyboard_t mask = 1 << k;

            if ((keys & mask) == 0)
                continue;

            long long elapsed = now - keyTs[k];

            if ((longPressSent & mask) == 0) {
                // The key is pressed and its long-press timer is over:
                // send the (one-shot) long-press event and start the
                // auto-repeat timer.
                if (elapsed >= input_longPressTimeout) {
                    msg->long_press = 1;
                    msg->keys = keys;
                    kbd_event = true;
                    longPressSent |= mask;
                    keyTs[k] = now;
                }
            } else if (((mask & repeatableKeys) != 0)
                       && (elapsed >= input_repeatInterval)) {
                // Long-press event already sent and this key supports
                // auto-repeat: keep repeating plain keypress events so
                // value-stepping menus keep advancing.
                msg->keys = keys;
                kbd_event = true;
                keyTs[k] = now;
            }
        }
    }

    prevKeys = keys;

    return kbd_event;
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
