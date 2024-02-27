/***************************************************************************
 *   Copyright (C) 2020 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                Kim Lyon VK6KL                           *
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

#ifndef UI_STATES_H
#define UI_STATES_H

#include <ui/ui_default.h>

extern State_st last_state ;
extern bool     macro_latched ;

extern void ui_States_MenuUp( GuiState_st* guiState , uint8_t menu_entries );
extern void ui_States_MenuDown( GuiState_st* guiState , uint8_t menu_entries );
extern void ui_States_MenuBack( GuiState_st* guiState );

extern void ui_States_TextInputConfirm( GuiState_st* guiState , char* buf );
extern void ui_States_TextInputDelete( GuiState_st* guiState , char* buf );
extern void ui_States_TextInputKeypad( GuiState_st* guiState , char* buf , uint8_t max_len , kbd_msg_t msg , bool callsign );
extern void ui_States_TextInputReset( GuiState_st* guiState , char* buf );

extern void ui_SetPageNum( GuiState_st* guiState , uint8_t pageNum );

#endif // UI_STATES_H
