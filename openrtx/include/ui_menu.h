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

#ifndef UI_MENU_H
#define UI_MENU_H

#include <ui.h>

void _ui_drawMenuList(point_t pos, const char *entries[],
                      uint8_t num_entries, uint8_t selected);
void _ui_drawChannelList(point_t pos, uint8_t selected);
void _ui_drawMenuTop(ui_state_t* ui_state);
void _ui_drawMenuChannel(ui_state_t* ui_state);
void _ui_drawMenuSettings(ui_state_t* ui_state);
#ifdef HAS_RTC
void _ui_drawSettingsTimeDate(state_t* last_state, ui_state_t* ui_state);
void _ui_drawSettingsTimeDateSet(state_t* last_state, ui_state_t* ui_state);
#endif
bool _ui_drawMenuMacro(state_t* last_state);

#endif /* UI_MENU_H */
