/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
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

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    KEY_NUM_0 ,
    KEY_NUM_1 ,
    KEY_NUM_2 ,
    KEY_NUM_3 ,
    KEY_NUM_4 ,
    KEY_NUM_5 ,
    KEY_NUM_6 ,
    KEY_NUM_7 ,
    KEY_NUM_8 ,
    KEY_NUM_9
}KeyNum_en;

/**
 * The following enum provides a set of flags to be used to check which buttons
 * are pressed by bit-masking the uint32_t value returned by kbd_getKeys().
 * They are handled as a uint32_t. See: kbd_msg_t in core/input.h .
 */
enum key
{
    KEY_0       = (1 << 0),   // Keypad digit "0"                  0x00000001
    KEY_1       = (1 << 1),   // Keypad digit "1"                  0x00000002
    KEY_2       = (1 << 2),   // Keypad digit "2"                  0x00000004
    KEY_3       = (1 << 3),   // Keypad digit "3"                  0x00000008
    KEY_4       = (1 << 4),   // Keypad digit "4"                  0x00000010
    KEY_5       = (1 << 5),   // Keypad digit "5"                  0x00000020
    KEY_6       = (1 << 6),   // Keypad digit "6"                  0x00000040
    KEY_7       = (1 << 7),   // Keypad digit "7"                  0x00000080
    KEY_8       = (1 << 8),   // Keypad digit "8"                  0x00000100
    KEY_9       = (1 << 9),   // Keypad digit "9"                  0x00000200
    KEY_STAR    = (1 << 10),  // Keypad digit "*"                  0x00000400
    KEY_HASH    = (1 << 11),  // Keypad digit "#"                  0x00000800
    KEY_ENTER   = (1 << 12),  // Keypad green button/enter         0x00001000
    KEY_ESC     = (1 << 13),  // Keypad red button/esc             0x00002000
    KEY_UP      = (1 << 14),  // Keypad upward arrow               0x00004000
    KEY_DOWN    = (1 << 15),  // Keypad downward arrow             0x00008000
    KEY_LEFT    = (1 << 16),  // Keypad leftward arrow             0x00010000
    KEY_RIGHT   = (1 << 17),  // Keypad rightward arrow            0x00020000
    KEY_MONI    = (1 << 18),  // Monitor button                    0x00040000
    KEY_F1      = (1 << 19),  // Function button                   0x00080000
    KEY_F2      = (1 << 20),  // Function button (device specific) 0x00100000
    KEY_F3      = (1 << 21),  // Function button (device specific) 0x00200000
    KEY_F4      = (1 << 22),  // Function button (device specific) 0x00400000
    KEY_F5      = (1 << 23),  // Function button (device specific) 0x00800000
    KEY_F6      = (1 << 24),  // Function button (device specific) 0x01000000
    KEY_VOLUP   = (1 << 25),  // Volume increase button            0x02000000
    KEY_VOLDOWN = (1 << 26),  // Volume decrease button            0x04000000
    KNOB_LEFT   = (1 << 27),  // Knob rotated counter clockwise    0x08000000
    KNOB_RIGHT  = (1 << 28),  // Knob rotated clockwise            0x10000000
};

/**
 * Number of supported keys
 */
#define KBD_NUM_KEYS 29

/**
 * Mask for the numeric keys in a key map
 * Numeric keys: bit0->bit11 = 0xFFF
 */
#define KBD_NUM_MASK 0x0FFF

/**
 * We encode the status of all the keys with a uint32_t value
 * To check which buttons are pressed one can bit-mask the
 * keys value with one of the enum values defined in key.
 * Example:
 * keyboard_t keys = kbd_getKeys();
 * if(keys & KEY_ENTER) do_stuff();
 */
typedef uint32_t keyboard_t;

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
keyboard_t kbd_getKeys();

#endif /* KEYBOARD_H */
