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

#ifndef UI_LIST_DISPLAY_H
#define UI_LIST_DISPLAY_H

#include <ui/ui_default.h>

// List Data Source
enum
{
    LIST_DATA_SOURCE_SCRIPT   ,
    LIST_DATA_SOURCE_BANKS    ,
    LIST_DATA_SOURCE_CHANNELS ,
    LIST_DATA_SOURCE_CONTACTS ,
    LIST_DATA_SOURCE_NUM_OF   ,
    LIST_DATA_SOURCE_STUBBED  = LIST_DATA_SOURCE_NUM_OF
};

extern void List_GetNumOfEntries( GuiState_st* guiState );
extern void List_EntryDisplay( GuiState_st* guiState );
extern void List_EntrySelect( GuiState_st* guiState );

extern void _ui_Draw_MenuList( GuiState_st* guiState , uiPageNum_en currentEntry );
extern void _ui_Draw_MenuListValue( GuiState_st* guiState ,
                                    uiPageNum_en currentEntry , uiPageNum_en currentEntryValue );
extern void _ui_reset_menu_anouncement_tracking( void );


#endif // UI_LIST_DISPLAY_H
