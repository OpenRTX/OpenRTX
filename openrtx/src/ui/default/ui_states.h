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

extern long long last_event_tick ;
extern State_st  last_state ;
extern bool      macro_latched ;
extern bool      macro_menu ;
extern bool      redraw_needed ;
extern uint8_t   evQueue_rdPos ;
extern uint8_t   evQueue_wrPos ;
extern Event_st  evQueue[ MAX_NUM_EVENTS ] ;

extern bool _ui_Draw_DarkOverlay( void );
extern void _ui_changePhoneticSpell( bool newVal );
extern void _ui_changeVoiceLevel( int variation );
extern void _ui_changeM17Can( int variation );
extern void _ui_numberInputDel( GuiState_st* guiState , uint32_t* num );
extern void _ui_numberInputKeypad( GuiState_st* guiState , uint32_t* num , kbd_msg_t msg );
extern void _ui_changeTimer( int variation );

extern void    ui_States_MenuUp( GuiState_st* guiState );
extern void    ui_States_MenuDown( GuiState_st* guiState );
extern void    ui_States_MenuBack( GuiState_st* guiState );
extern uint8_t ui_States_GetPageNumOfEntries( GuiState_st* guiState );

extern void ui_States_TextInputConfirm( GuiState_st* guiState , char* buf );
extern void ui_States_TextInputDelete( GuiState_st* guiState , char* buf );
extern void ui_States_TextInputKeypad( GuiState_st* guiState , char* buf , uint8_t max_len , kbd_msg_t msg , bool callsign );
extern void ui_States_TextInputReset( GuiState_st* guiState , char* buf );

extern int     FSM_LoadChannel( int16_t channel_index , bool* sync_rtx );
extern int     _ui_fsm_loadContact( int16_t contact_index , bool* sync_rtx );

extern void ui_States_SelectPage( GuiState_st* guiState );

#endif // UI_STATES_H
