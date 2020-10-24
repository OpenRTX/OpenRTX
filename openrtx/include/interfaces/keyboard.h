/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Silvano Seva IU2KWO                             *
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

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

/**
 * The following enum provides a set of flags to be used to check whose buttons
 * are pressed by bit-masking the uint32_t value returned by kbd_getKeys().
 */
enum keys
{
    KEY_0     = (1 << 0),
    KEY_1     = (1 << 1),
    KEY_2     = (1 << 2),
    KEY_3     = (1 << 3),
    KEY_4     = (1 << 4),
    KEY_5     = (1 << 5),
    KEY_6     = (1 << 6),
    KEY_7     = (1 << 7),
    KEY_8     = (1 << 8),
    KEY_9     = (1 << 9),
    KEY_STAR  = (1 << 10),
    KEY_HASH  = (1 << 11),
    KEY_ENTER = (1 << 12),
    KEY_ESC   = (1 << 13),
    KEY_UP    = (1 << 14),
    KEY_DOWN  = (1 << 15),
    KEY_LEFT  = (1 << 16),
    KEY_RIGHT = (1 << 17),
    KEY_F1    = (1 << 18),
    KEY_F2    = (1 << 19),
    KEY_F3    = (1 << 20),
    KEY_F4    = (1 << 21),
    KEY_F5    = (1 << 22),
    KEY_F6    = (1 << 23),
    KEY_F7    = (1 << 24),
    KEY_F8    = (1 << 25),
    KEY_F9    = (1 << 26),
    KEY_F10   = (1 << 27),
    KEY_F11   = (1 << 28),
    KEY_F12   = (1 << 29),
    KEY_F13   = (1 << 30),
    KEY_F14   = (1 << 31)
};

/**
 * This function initialises the keyboard subsystem, configuring the GPIOs as
 * needed.
 */
void kbd_init();

/**
 * When called, this function terminates the keyboard driver.
 */
void kbd_terminate();

/**
 * When called, this function takes a snapshot of the current configuration of
 * all the keyboard buttons and returns it as a 32-bit variable.
 * 
 * @return an uint32_t representing the current keyboard configuration.
 */
uint32_t kbd_getKeys();

#endif /* KEYBOARD_H */
