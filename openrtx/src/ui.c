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

/*
 * The graphical user interface (GUI) works by splitting the screen in 
 * horizontal rows, with row height depending on vertical resolution.
 * 
 * The general screen layout is composed by an upper status bar at the
 * top of the screen and a lower status bar at the bottom.
 * The central portion of the screen is filled by two big text/number rows
 * And a small row.
 *
 * Below is shown the row height for two common display densities.
 *
 *        160x128 display (MD380)
 *      ┌─────────────────────────┐
 *      │  top_status_bar (16px)  │
 *      ├─────────────────────────┤
 *      │      Line 1 (32px)      │
 *      │                         │
 *      │      Line 2 (32px)      │
 *      │                         │
 *      │      Line 3 (32px)      │
 *      │                         │
 *      ├─────────────────────────┤
 *      │bottom_status_bar (16px) │
 *      └─────────────────────────┘
 *
 *         128x64 display (GD77)
 *      ┌─────────────────────────┐
 *      │  top_status_bar (8px)   │
 *      ├─────────────────────────┤
 *      │      Line 1 (16px)      │
 *      │      Line 2 (16px)      │
 *      │      Line 3 (16px)      │
 *      ├─────────────────────────┤
 *      │ bottom_status_bar (8px) │
 *      └─────────────────────────┘
 */

#include <stdio.h>
#include <ui.h>

void ui_init()
{
}

bool ui_update(state_t state, uint16_t keys)
{
    return true;
}

void ui_terminate()
{
}
