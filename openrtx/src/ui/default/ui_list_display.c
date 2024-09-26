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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <utils.h>
#include <ui.h>
#include <ui/ui_default.h>
#include "ui_states.h"
#include "ui_commands.h"
#include "ui_list_display.h"
#include <interfaces/nvmem.h>
#include <interfaces/cps_io.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <memory_profiling.h>
#include <ui/ui_strings.h>
#include <core/voicePromptUtils.h>
#include "ui_value_display.h"
#include "ui_value_arrays.h"

static void List_GetNumOfEntries_Script( GuiState_st* guiState );
static void List_EntryDisplay_Script( GuiState_st* guiState );
static void List_EntrySelect_Script( GuiState_st* guiState );

static void List_GetNumOfEntries_Banks( GuiState_st* guiState );
static void List_EntryDisplay_Banks( GuiState_st* guiState );
static void List_EntrySelect_Banks( GuiState_st* guiState );

static void List_GetNumOfEntries_Channels( GuiState_st* guiState );
static void List_EntryDisplay_Channels( GuiState_st* guiState );
static void List_EntrySelect_Channels( GuiState_st* guiState );

static void List_GetNumOfEntries_Contacts( GuiState_st* guiState );
static void List_EntryDisplay_Contacts( GuiState_st* guiState );
static void List_EntrySelect_Contacts( GuiState_st* guiState );

static void List_GetNumOfEntries_Stubbed( GuiState_st* guiState );
static void List_EntryDisplay_Stubbed( GuiState_st* guiState );
static void List_EntrySelect_Stubbed( GuiState_st* guiState );

typedef void (*ui_List_GetNumOfEntries_fn)( GuiState_st* guiState );
typedef void (*ui_List_EntryDisplay_fn)( GuiState_st* guiState );
typedef void (*ui_List_EntrySelect_fn)( GuiState_st* guiState );

typedef struct
{
    ui_List_GetNumOfEntries_fn  getNumOfEntries ;
    ui_List_EntryDisplay_fn     entryDisplay ;
    ui_List_EntrySelect_fn      entrySelect ;
}ListDisplay_st;

static const ListDisplay_st ui_ListDisplay_Table[ LIST_DATA_SOURCE_NUM_OF + 1 ] =
{
    { List_GetNumOfEntries_Script     , List_EntryDisplay_Script     , List_EntrySelect_Script     } ,
    { List_GetNumOfEntries_Banks    , List_EntryDisplay_Banks    , List_EntrySelect_Banks    } ,
    { List_GetNumOfEntries_Channels , List_EntryDisplay_Channels , List_EntrySelect_Channels } ,
    { List_GetNumOfEntries_Contacts , List_EntryDisplay_Contacts , List_EntrySelect_Contacts } ,
    { List_GetNumOfEntries_Stubbed  , List_EntryDisplay_Stubbed  , List_EntrySelect_Stubbed  }
};

void List_GetNumOfEntries( GuiState_st* guiState )
{
    ui_ListDisplay_Table[ guiState->layout.list.dataSource ].getNumOfEntries( guiState );
}

void List_EntryDisplay( GuiState_st* guiState )
{
    ui_ListDisplay_Table[ guiState->layout.list.dataSource ].entryDisplay( guiState );
}

void List_EntrySelect( GuiState_st* guiState )
{
    ui_ListDisplay_Table[ guiState->layout.list.dataSource ].entrySelect( guiState );
}

/*******************************************************************************
*   Script
*******************************************************************************/

static void List_GetNumOfEntries_Script( GuiState_st* guiState )
{
    GuiCmd_List_GetNumOfEntries_Script( guiState );
}

static void List_EntryDisplay_Script( GuiState_st* guiState )
{
    GuiCmd_List_EntryDisplay_Script( guiState );
}

static void List_EntrySelect_Script( GuiState_st* guiState )
{
    GuiCmd_List_EntrySelect_Script( guiState );
}

/*******************************************************************************
*   Banks
*******************************************************************************/

static void List_GetNumOfEntries_Banks( GuiState_st* guiState )
{
    bankHdr_t bank ;
    int       result  = 0 ;

    guiState->layout.list.numOfEntries = 0 ;

    do
    {
        result = cps_readBankHeader( &bank , guiState->layout.list.numOfEntries );
        // note - first bank "All channels" is not read from flash - hence size is +1
        guiState->layout.list.numOfEntries++ ;
    }while( result != -1 );

}

static void List_EntryDisplay_Banks( GuiState_st* guiState )
{
    bankHdr_t bank ;
    char      entryBuf[ MAX_ENTRY_LEN ] = "" ;
    uint8_t   index  = guiState->layout.list.offset + guiState->layout.list.index ;
    int       result = 0 ;

    // first bank "All channels" is not read from flash
    if( index == 0 )
    {
        strncpy( entryBuf , currentLanguage->allChannels , MAX_ENTRY_LEN );
    }
    else
    {
        result = cps_readBankHeader( &bank , index - 1 );
        if( result != -1 )
        {
            snprintf( entryBuf , MAX_ENTRY_LEN , "%s" , bank.name );
        }
    }

    GuiCmd_List_EntryDisplay_TextString( guiState , entryBuf );

}

static void List_EntrySelect_Banks( GuiState_st* guiState )
{
    bankHdr_t newbank ;
    int       result   = 0 ;
    bool      sync_rtx = false ;

    // If "All channels" is selected, load default bank
    if( guiState->layout.list.selection == 0 )
    {
        state.bank_enabled = false ;
    }
    else
    {
        state.bank_enabled = true;
        result = cps_readBankHeader( &newbank , guiState->layout.list.selection - 1 );
    }
    if( result != -1 )
    {
        state.bank = guiState->layout.list.selection - 1 ;
        // If we were in VFO mode, save VFO channel
        if( guiState->page.num == PAGE_MAIN_VFO )
        {
            state.vfo_channel = state.channel ;
        }
        // Load bank first channel
        FSM_LoadChannel( 0 , &sync_rtx );
    }

}

/*******************************************************************************
*   Channels
*******************************************************************************/

static void List_GetNumOfEntries_Channels( GuiState_st* guiState )
{
    channel_t channel ;
    int       result  = 0 ;

    guiState->layout.list.numOfEntries = 0 ;

    while( true )
    {
        result = cps_readChannel( &channel , guiState->layout.list.numOfEntries );
        if( result != -1 )
        {
            guiState->layout.list.numOfEntries++ ;
        }
        else
        {
            break ;
        }
    }

}

static void List_EntryDisplay_Channels( GuiState_st* guiState )
{
    channel_t channel ;
    char      entryBuf[ MAX_ENTRY_LEN ] = "" ;
    uint8_t   index  = guiState->layout.list.offset + guiState->layout.list.index ;
    int       result = cps_readChannel( &channel , index );

    if( result != -1 )
    {
        snprintf( entryBuf , MAX_ENTRY_LEN , "%s" , channel.name );
    }

    GuiCmd_List_EntryDisplay_TextString( guiState , entryBuf );

}

static void List_EntrySelect_Channels( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    // If we were in VFO mode, save VFO channel
    if( guiState->page.num == PAGE_MAIN_VFO )
    {
        state.vfo_channel = state.channel;
    }
    FSM_LoadChannel( guiState->layout.list.selection , &sync_rtx );

}

/*******************************************************************************
*   Contacts
*******************************************************************************/

static void List_GetNumOfEntries_Contacts( GuiState_st* guiState )
{
    contact_t contact ;
    int       result  = 0 ;

    guiState->layout.list.numOfEntries = 0 ;

    while( true )
    {
        result = cps_readContact( &contact , guiState->layout.list.numOfEntries );
        if( result != -1 )
        {
            guiState->layout.list.numOfEntries++ ;
        }
        else
        {
            break ;
        }
    }
}

static void List_EntryDisplay_Contacts( GuiState_st* guiState )
{
    contact_t contact ;
    char      entryBuf[ MAX_ENTRY_LEN ] = "" ;
    uint8_t   index  = guiState->layout.list.offset + guiState->layout.list.index ;
    int       result = cps_readContact( &contact , index );

    if( result != -1 )
    {
        snprintf( entryBuf , MAX_ENTRY_LEN , "%s" , contact.name );
    }

    GuiCmd_List_EntryDisplay_TextString( guiState , entryBuf );

}

static void List_EntrySelect_Contacts( GuiState_st* guiState )
{
    bool sync_rtx = false ;

    _ui_fsm_loadContact( guiState->layout.list.selection , &sync_rtx );

}

/*******************************************************************************
*   Stubbed
*******************************************************************************/

static void List_GetNumOfEntries_Stubbed( GuiState_st* guiState )
{
    (void)guiState;
}

static void List_EntryDisplay_Stubbed( GuiState_st* guiState )
{
    GuiCmd_List_EntryDisplay_Stubbed( guiState );
}

static void List_EntrySelect_Stubbed( GuiState_st* guiState )
{
}
