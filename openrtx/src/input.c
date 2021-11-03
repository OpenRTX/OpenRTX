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

#include <input.h>
#include <interfaces/keyboard.h>
#include <inttypes.h>
#include <stdbool.h>

bool input_isNumberPressed(kbd_msg_t msg)
{
    return msg.keys & kbd_num_mask;
}

uint8_t input_getPressedNumber(kbd_msg_t msg)
{
    uint32_t masked_input = msg.keys & kbd_num_mask;
    if (masked_input == 0) return 0;
    return __builtin_ctz(msg.keys & kbd_num_mask);
}
