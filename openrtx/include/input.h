/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
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

#ifndef INPUT_H
#define INPUT_H

#include <inttypes.h>
#include <stdbool.h>

/* This function returns true if at least one number is pressed on the
 * keyboard.
 * @param msg: the keyboard queue message
 * @return true if at least a number is pressed on the keyboard
 */
bool input_isNumberPressed(kbd_msg_t msg);

/* This function returns the smallest number that is pressed on the keyboard,
 * 0 if none is pressed.
 * @param msg: the keyboard queue message
 * @return the smalled pressed number on the keyboard
 */
uint8_t input_getPressedNumber(kbd_msg_t msg);

#endif /* INPUT_H */
