/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
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

#include <ui/ui_default.h>
#include "ui_scripts.h"

static const uint32_t ColorTable[ COLOR_NUM_OF ] =
{
    COLOR_TABLE
};

static bool GuiCmd_Null( GuiState_st* guiState );
static bool GuiCmd_EventStart( GuiState_st* guiState );
static bool GuiCmd_EventEnd( GuiState_st* guiState );
static bool GuiCmd_GoToLine( GuiState_st* guiState );
static bool GuiCmd_AlignLeft( GuiState_st* guiState );
static bool GuiCmd_AlignCenter( GuiState_st* guiState );
static bool GuiCmd_AlignRight( GuiState_st* guiState );
static bool GuiCmd_FontSize( GuiState_st* guiState );
static bool GuiCmd_LineEnd( GuiState_st* guiState );
static bool GuiCmd_Link( GuiState_st* guiState );
static bool GuiCmd_LinkEnd( GuiState_st* guiState );
static bool GuiCmd_Page( GuiState_st* guiState );
static bool GuiCmd_Value( GuiState_st* guiState );
static bool GuiCmd_Title( GuiState_st* guiState );
static bool GuiCmd_Text( GuiState_st* guiState );
static bool GuiCmd_PageEnd( GuiState_st* guiState );
static bool GuiCmd_Stubbed( GuiState_st* guiState );

static bool GuiCmd_Select( GuiState_st* guiState );
static void GuiCmd_DisplayValue( GuiState_st* guiState , uint8_t valueNum );
static void GuiCmd_AdvToNextCmd( GuiState_st* guiState );

typedef bool (*ui_GuiCmd_fn)( GuiState_st* guiState );

static const ui_GuiCmd_fn ui_GuiCmd_Table[ GUI_CMD_NUM_OF ] =
{
    GuiCmd_Null        , // 0x00
    GuiCmd_EventStart  , // 0x01
    GuiCmd_EventEnd    , // 0x02
    GuiCmd_GoToLine    , // 0x03
    GuiCmd_AlignLeft   , // 0x04
    GuiCmd_AlignCenter , // 0x05
    GuiCmd_AlignRight  , // 0x06
    GuiCmd_FontSize    , // 0x07
    GuiCmd_Stubbed     , // 0x08
    GuiCmd_Stubbed     , // 0x09
    GuiCmd_LineEnd     , // 0x0A
    GuiCmd_Link        , // 0x0B
    GuiCmd_LinkEnd     , // 0x0C
    GuiCmd_Stubbed     , // 0x0D
    GuiCmd_Page        , // 0x0E
    GuiCmd_Value       , // 0x0F
    GuiCmd_Title       , // 0x10
    GuiCmd_Text        , // 0x11
    GuiCmd_Stubbed     , // 0x12
    GuiCmd_Stubbed     , // 0x13
    GuiCmd_Stubbed     , // 0x14
    GuiCmd_Stubbed     , // 0x15
    GuiCmd_Stubbed     , // 0x16
    GuiCmd_Stubbed     , // 0x17
    GuiCmd_Stubbed     , // 0x18
    GuiCmd_Stubbed     , // 0x19
    GuiCmd_Stubbed     , // 0x1A
    GuiCmd_Stubbed     , // 0x1B
    GuiCmd_Stubbed     , // 0x1C
    GuiCmd_Stubbed     , // 0x1D
    GuiCmd_Stubbed     , // 0x1E
    GuiCmd_PageEnd       // 0x1F
};

bool ui_DisplayPage( GuiState_st* guiState )
{
    uint16_t count ;
    uint8_t  cmd ;
    bool     exit ;
    bool     pageDisplayed = false ;

    guiState->layout.printDisplayOn = true ;
    guiState->layout.inSelect       = false ;

    if( guiState->page.num != guiState->page.levelList[ guiState->page.level ] )
    {
        if( guiState->page.level < MAX_PAGE_DEPTH )
        {
            guiState->page.level++ ;
        }
        guiState->page.levelList[ guiState->page.level ] = guiState->page.num ;
    }

    guiState->page.ptr              = (uint8_t*)uiPageTable[ guiState->page.num ] ;

    guiState->layout.numOfEntries   = 0 ;
    guiState->layout.lineIndex      = 0 ;

    guiState->layout.linkNumOf      = 0 ;
    guiState->layout.linkIndex      = 0 ;

    if( guiState->page.ptr[ 0 ] != GUI_CMD_PAGE_END )
    {
        guiState->layout.line.pos.y   = 0 ;
        guiState->layout.line.pos.x   = 0 ;

        guiState->layout.scrollOffset = 0 ;

	    guiState->page.index          = 0 ;

	    if( !guiState->update )
	    {
            guiState->pageHasEvents = false ;
	    }

	    if( !guiState->pageHasEvents )
	    {
            gfx_clearScreen();
            for( exit = false , count = 256 ; !exit && count ; count-- )
            {
                cmd = guiState->page.ptr[ guiState->page.index ] ;

                if( cmd < GUI_CMD_DATA_AREA )
                {
                    exit = ui_GuiCmd_Table[ cmd ]( guiState );
                }
                else
                {
                    guiState->page.index++ ;
                }
            }
	    }
	    else
	    {
            for( exit = false , count = 256 ; !exit && count ; count-- )
            {
                cmd = guiState->page.ptr[ guiState->page.index ] ;

                if( cmd < GUI_CMD_DATA_AREA )
                {
                    if( ( cmd == GUI_CMD_EVENT_START ) ||
                        guiState->inEventArea             )
                    {
                        exit = ui_GuiCmd_Table[ cmd ]( guiState );
                    }
                    else
                    {
                        guiState->page.index++ ;
                    }
                }
                else
                {
                    guiState->page.index++ ;
                }
            }
	    }

	    pageDisplayed = true ;

	}

    return pageDisplayed ;

}

static bool GuiCmd_Null( GuiState_st* guiState )
{
    bool pageEnd = false ;

    printf( "Cmd NULL" );

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_EventStart( GuiState_st* guiState )
{
    uint8_t  eventType ;
    uint8_t  eventPayload ;
    uint16_t count   = 256 ;
    bool     pageEnd = false ;

    guiState->page.index++ ;
    eventType    = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->page.index++ ;
    eventPayload = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->page.index++ ;

    if( !( ( guiState->event.type    == eventType    ) &&
           ( guiState->event.payload == eventPayload )    ) )
    {
        while( ( guiState->page.ptr[ guiState->page.index ] != GUI_CMD_EVENT_END ) &&
               ( guiState->page.ptr[ guiState->page.index ] != GUI_CMD_PAGE_END  )    )
        {
            count-- ;
            if( count == 0 )
            {
                break ;
            }
            guiState->page.index++ ;
        }
    }
    else
    {
        guiState->inEventArea = true ;
    }

    guiState->pageHasEvents = true ;

    return pageEnd ;

}

static bool GuiCmd_EventEnd( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    guiState->inEventArea = false ;

    return pageEnd ;

}

static bool GuiCmd_GoToLine( GuiState_st* guiState )
{
    uint8_t lineIndex ;
    bool    pageEnd = false ;

    guiState->page.index++ ;

    lineIndex                  = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->layout.lineIndex = lineIndex ;
    guiState->layout.line 	   = guiState->layout.lineStyle[ lineIndex ] ;

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_AlignLeft( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    guiState->layout.line.align = ALIGN_LEFT ;

    return pageEnd ;

}

static bool GuiCmd_AlignCenter( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    guiState->layout.line.align = ALIGN_CENTER ;

    return pageEnd ;

}

static bool GuiCmd_AlignRight( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    guiState->layout.line.align = ALIGN_RIGHT ;

    return pageEnd ;

}

static bool GuiCmd_FontSize( GuiState_st* guiState )
{
    uint8_t fontSize ;
    bool    pageEnd  = false ;

    guiState->page.index++ ;

    fontSize                        = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->layout.line.font.size = fontSize ;

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_LineEnd( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    if( guiState->layout.printDisplayOn )
    {
        guiState->layout.line.pos.y += guiState->layout.menu_h ;
    }

    if( ( guiState->layout.lineIndex + 1 ) < GUI_LINE_NUM_OF )
    {
        guiState->layout.lineIndex++ ;
    }

    return pageEnd ;

}

static bool GuiCmd_Link( GuiState_st* guiState )
{
    bool pageEnd ;

    guiState->page.index++ ;

    if( guiState->layout.linkNumOf < LINK_MAX_NUM_OF )
    {
        guiState->layout.linkNumOf++ ;
    }

    pageEnd = GuiCmd_Select( guiState );

    return pageEnd ;
}

static bool GuiCmd_LinkEnd( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    if( guiState->layout.linkNumOf < LINK_MAX_NUM_OF )
    {
        guiState->layout.linkIndex++ ;
    }

    guiState->layout.inSelect = false ;

    return pageEnd ;

}

static bool GuiCmd_Page( GuiState_st* guiState )
{
    uint8_t linkNum ;
    bool    pageEnd = false ;

    guiState->page.index++ ;

    linkNum = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->layout.links[ guiState->layout.linkIndex ].type = LINK_TYPE_PAGE ;
    guiState->layout.links[ guiState->layout.linkIndex ].num  = linkNum ;
    guiState->layout.links[ guiState->layout.linkIndex ].amt  = 0 ;

    guiState->page.index++ ;

    return pageEnd ;
}

static bool GuiCmd_Value( GuiState_st* guiState )
{
    uint8_t valueNum ;
    bool    pageEnd  = false ;

    guiState->page.index++ ;

    valueNum = LD_VAL( guiState->page.ptr[ guiState->page.index ] );

    if( valueNum >= GUI_VAL_DSP_NUM_OF )
    {
        valueNum = GUI_VAL_DSP_STUBBED ;
    }

    guiState->layout.links[ guiState->layout.linkIndex ].type = LINK_TYPE_VALUE ;
    guiState->layout.links[ guiState->layout.linkIndex ].num  = valueNum ;
    guiState->layout.links[ guiState->layout.linkIndex ].amt  = 0 ;

    GuiCmd_DisplayValue( guiState , valueNum );

    GuiCmd_AdvToNextCmd( guiState );

    return pageEnd ;

}

static bool GuiCmd_Title( GuiState_st* guiState )
{
    uint8_t* scriptPtr ;
    Color_st color_fg ;
    bool     pageEnd   = false ;

    guiState->page.index++ ;

    scriptPtr = &guiState->page.ptr[ guiState->page.index ] ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    guiState->layout.lineIndex = GUI_LINE_TOP ;

    // print the title on the top bar
    gfx_print( guiState->layout.line.pos       ,
               guiState->layout.line.font.size ,
               guiState->layout.line.align     ,
               color_fg , (char*)scriptPtr );

    GuiCmd_AdvToNextCmd( guiState );

    return pageEnd ;

}

static bool GuiCmd_Text( GuiState_st* guiState )
{
    uint8_t* scriptPtr ;
    Color_st color_fg ;
    Color_st color_bg ;
    Color_st color_text ;
    bool     pageEnd    = false ;

    ui_ColorLoad( &color_fg , COLOR_FG );
    ui_ColorLoad( &color_bg , COLOR_BG );
    color_text = color_fg ;

    while( guiState->page.ptr[ guiState->page.index ] < GUI_CMD_NUM_OF )
    {
        guiState->page.index++ ;
    }

    scriptPtr = &guiState->page.ptr[ guiState->page.index ] ;

    if( guiState->layout.printDisplayOn )
    {
        if( guiState->layout.inSelect )
        {
            color_text      = color_bg ;
            // Draw rectangle under selected item, compensating for text height
            Pos_st rect_pos = { 0 , guiState->layout.line.pos.y - guiState->layout.menu_h + 3 };
            gfx_drawRect( rect_pos , SCREEN_WIDTH , guiState->layout.menu_h , color_fg , true );
        }
//@@@KL            announceMenuItemIfNeeded( entryBuf , NULL , false );
        gfx_print( guiState->layout.line.pos       ,
                   guiState->layout.line.font.size ,
                   guiState->layout.line.align     ,
                   color_text , (char*)scriptPtr );
    }

    GuiCmd_AdvToNextCmd( guiState );

    return pageEnd ;

}

static bool GuiCmd_PageEnd( GuiState_st* guiState )
{
    bool pageEnd = true ;

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_Stubbed( GuiState_st* guiState )
{
    bool pageEnd = false ;

    printf( "Cmd Stubbed" );

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_Select( GuiState_st* guiState )
{
    bool    pageEnd = false ;

    if( guiState->layout.linkIndex == 0 )
    {
        // Number of menu entries that fit in the screen height
        guiState->layout.numOfEntries = ( SCREEN_HEIGHT - 1 - guiState->layout.line.pos.y ) /
                                        guiState->layout.menu_h + 1 ;
    }

    // If selection is off the screen, scroll screen
    if( guiState->uiState.menu_selected >= guiState->layout.numOfEntries )
    {
        guiState->layout.scrollOffset = guiState->uiState.menu_selected -
                                        guiState->layout.numOfEntries + 1 ;
    }

    guiState->layout.printDisplayOn = false ;
    guiState->layout.inSelect       = false ;

    if( (   guiState->layout.linkIndex >= guiState->layout.scrollOffset ) &&
        ( ( guiState->layout.linkIndex  - guiState->layout.scrollOffset )  < guiState->layout.numOfEntries ) )
    {
        if( guiState->layout.linkIndex == guiState->uiState.menu_selected )
        {
            guiState->layout.inSelect = true ;
        }
        guiState->layout.printDisplayOn = true ;
    }

    return pageEnd ;

}

static void GuiCmd_AdvToNextCmd( GuiState_st* guiState )
{
    uint16_t count = 256 ;

    while( ( guiState->page.ptr[ guiState->page.index ] != GUI_CMD_NULL      ) &&
           ( guiState->page.ptr[ guiState->page.index ]  > GUI_CMD_PAGE_END  )    )
    {
        count-- ;
        if( count == 0 )
        {
            break ;
        }
        guiState->page.index++ ;
    }

}

void ui_ColorLoad( Color_st* color , ColorSelector_en colorSelector )
{
    ColorSelector_en colorSel = colorSelector ;

    if( colorSel > COLOR_NUM_OF )
    {
        colorSel = COLOR_AL ;
    }

    COLOR_LD( color , ColorTable[ colorSel ] )
}

void ui_SetPageNum( GuiState_st* guiState , uint8_t pageNum )
{
    uint8_t pgNum = pageNum ;

    if( pgNum >= PAGE_NUM_OF )
    {
        pgNum = PAGE_STUBBED ;
    }

    guiState->page.num = pgNum ;
    state.ui_page      = pgNum ;
}


