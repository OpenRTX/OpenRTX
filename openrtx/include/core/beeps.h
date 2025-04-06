/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                Joseph Stephen VK7JS                     *
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
#ifndef BEEPS_H_INCLUDED
#define BEEPS_H_INCLUDED
// Duration in tenths of a second.
#define SHORT_BEEP 3
#define LONG_BEEP 7

// Beep frequencies
#define BEEP_MENU_FIRST_ITEM 500
#define BEEP_MENU_ITEM 1000
#define BEEP_FUNCTION_LATCH_ON 800
#define BEEP_FUNCTION_LATCH_OFF 400
#define BEEP_KEY_GENERIC 750
extern const uint16_t BOOT_MELODY[];

#endif // BEEPS_H_INCLUDED
