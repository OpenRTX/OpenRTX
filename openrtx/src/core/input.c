/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
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

#include <interfaces/delays.h>
#include <inttypes.h>
#include <stdbool.h>
#include <input.h>

static long long  keyTs[KBD_NUM_KEYS];  // Timestamp of each keypress
static uint32_t   longPressSent;        // Flags to manage long-press events
static keyboard_t prevKeys = 0;         // Previous keyboard status

bool input_scanKeyboard(kbd_msg_t *msg)
{
    msg->value     = 0;
    bool kbd_event = false;

    keyboard_t keys = kbd_getKeys();
    long long now   = getTick();

    // The key status has changed
    if(keys != prevKeys)
    {
        // Find newly pressed keys
        keyboard_t newKeys = (keys ^ prevKeys) & keys;

        // Save timestamp
        for(uint8_t k = 0; k < KBD_NUM_KEYS; k++)
        {
            keyboard_t mask = 1 << k;
            if((newKeys & mask) != 0)
            {
                keyTs[k]       = now;
                longPressSent &= ~mask;
            }
        }

        // New keyboard event
        msg->keys = keys;
        kbd_event = true;
    }
    // Some key is kept pressed
    else if(keys != 0)
    {
        // Check for saved timestamp to trigger long-presses
        for(uint8_t k = 0; k < KBD_NUM_KEYS; k++)
        {
            keyboard_t mask = 1 << k;

            // The key is pressed and its long-press timer is over
            if(((keys & mask) != 0)          &&
               ((longPressSent & mask) == 0) &&
               ((now - keyTs[k]) >= input_longPressTimeout))
            {
                msg->long_press = 1;
                msg->keys       = keys;
                kbd_event       = true;
                longPressSent  |= mask;
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

uint8_t input_getPressedNumber(kbd_msg_t msg)
{
    uint32_t masked_input = msg.keys & KBD_NUM_MASK;
    if (masked_input == 0)
        return 0;

    return __builtin_ctz(msg.keys & KBD_NUM_MASK);
}
