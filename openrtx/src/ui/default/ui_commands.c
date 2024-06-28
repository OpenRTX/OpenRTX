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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <input.h>
#include <hwconfig.h>
#include <voicePromptUtils.h>
#include <ui.h>
#include <ui/ui_default.h>
#include <rtx.h>
#include <interfaces/platform.h>
#include <interfaces/display.h>
#include <interfaces/cps_io.h>
#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include <string.h>
#include <battery.h>
#include <utils.h>
#include <beeps.h>
#include <memory_profiling.h>

#ifdef PLATFORM_TTWRPLUS
#include <SA8x8.h>
#endif

//@@@KL #include "ui_m17.h"

#include "ui.h"
#include "ui_value_arrays.h"
#include "ui_scripts.h"
#include "ui_commands.h"
#include "ui_value_display.h"
#include "ui_states.h"
#include "ui_value_input.h"

static const uint32_t ColorTable[ COLOR_NUM_OF ] =
{
    COLOR_TABLE
};

static void RunScript( GuiState_st* guiState , uint8_t scriptPageNum );

static bool GuiCmd_Null( GuiState_st* guiState );
static bool GuiCmd_EventStart( GuiState_st* guiState );
static bool GuiCmd_EventEnd( GuiState_st* guiState );
static bool GuiCmd_TimerCheck( GuiState_st* guiState );
static bool GuiCmd_TimerSet( GuiState_st* guiState );
static bool GuiCmd_GoToLine( GuiState_st* guiState );
static bool GuiCmd_GoToPos( GuiState_st* guiState );
static bool GuiCmd_LoadStyle( GuiState_st* guiState );
static bool GuiCmd_BGColor( GuiState_st* guiState );
static bool GuiCmd_FGColor( GuiState_st* guiState );
static bool GuiCmd_FontSize( GuiState_st* guiState );
static bool GuiCmd_Align( GuiState_st* guiState );
static bool GuiCmd_LineEnd( GuiState_st* guiState );
static bool GuiCmd_RunScript( GuiState_st* guiState );
static bool GuiCmd_Link( GuiState_st* guiState );
static bool GuiCmd_LinkEnd( GuiState_st* guiState );
static bool GuiCmd_Page( GuiState_st* guiState );
static bool GuiCmd_Title( GuiState_st* guiState );
static bool GuiCmd_Text( GuiState_st* guiState );
static bool GuiCmd_ValueDisplay( GuiState_st* guiState );
static bool GuiCmd_ValueInput( GuiState_st* guiState );
static bool GuiCmd_DrawLine( GuiState_st* guiState );
static bool GuiCmd_DrawRect( GuiState_st* guiState );
static bool GuiCmd_DrawRectFilled( GuiState_st* guiState );
#ifdef DISPLAY_DEBUG_MSG
static bool GuiCmd_SetDbgMsg( GuiState_st* guiState );
#endif // DISPLAY_DEBUG_MSG
static bool GuiCmd_PageEnd( GuiState_st* guiState );
static bool GuiCmd_Stubbed( GuiState_st* guiState );

static bool    GuiCmd_LinkStart( GuiState_st* guiState );

static uint8_t GuiCmd_LdVal( GuiState_st* guiState );
static void    GuiCmd_AdvToNextCmd( GuiState_st* guiState );

static void    GuiCmd_Print( GuiState_st* guiState , Color_st color , char* scriptPtr );

typedef bool (*ui_GuiCmd_fn)( GuiState_st* guiState );

static const ui_GuiCmd_fn ui_GuiCmd_Table[ GUI_CMD_NUM_OF ] =
{
    GuiCmd_Null           , // GUI_CMD_NULL              0x00
    GuiCmd_EventStart     , // GUI_CMD_EVENT_START       0x01
    GuiCmd_EventEnd       , // GUI_CMD_EVENT_END         0x02
    GuiCmd_TimerCheck     , // GUI_CMD_TIMER_CHECK       0x03
    GuiCmd_TimerSet       , // GUI_CMD_TIMER_SET         0x04
    GuiCmd_GoToLine       , // GUI_CMD_GOTO_LINE         0x05
    GuiCmd_GoToPos        , // GUI_CMD_GOTO_POS          0x06
    GuiCmd_LoadStyle      , // GUI_CMD_LOAD_STYLE        0x07
    GuiCmd_BGColor        , // GUI_CMD_BG_COLOR          0x08
    GuiCmd_FGColor        , // GUI_CMD_FG_COLOR          0x09
    GuiCmd_LineEnd        , // GUI_CMD_LINE_END          0x0A
    GuiCmd_FontSize       , // GUI_CMD_FONT_SIZE         0x0B
    GuiCmd_Align          , // GUI_CMD_ALIGN             0x0C
    GuiCmd_RunScript      , // GUI_CMD_RUN_SCRIPT        0x0D
    GuiCmd_Link           , // GUI_CMD_LINK              0x0E
    GuiCmd_LinkEnd        , // GUI_CMD_LINK_END          0x0F
    GuiCmd_Page           , // GUI_CMD_PAGE              0x10
    GuiCmd_Title          , // GUI_CMD_TITLE             0x11
    GuiCmd_Text           , // GUI_CMD_TEXT              0x12
    GuiCmd_ValueDisplay   , // GUI_CMD_VALUE_DSP         0x13
    GuiCmd_ValueInput     , // GUI_CMD_VALUE_INP         0x14
    GuiCmd_DrawLine       , // GUI_CMD_DRAW_LINE         0x15
    GuiCmd_DrawRect       , // GUI_CMD_DRAW_RECT         0x16
    GuiCmd_DrawRectFilled , // GUI_CMD_DRAW_RECT_FILLED  0x17
    GuiCmd_Stubbed        , // 0x18
    GuiCmd_Stubbed        , // 0x19
    GuiCmd_Stubbed        , // 0x1A
    GuiCmd_Stubbed        , // 0x1B
    GuiCmd_Stubbed        , // 0x1C
#ifndef DISPLAY_DEBUG_MSG
    GuiCmd_Stubbed        , // GUI_CMD_STUBBED           0x1D
#else // DISPLAY_DEBUG_MSG
    GuiCmd_SetDbgMsg      , // GUI_CMD_SET_DBG_MSG       0x1D
#endif // DISPLAY_DEBUG_MSG
    GuiCmd_Stubbed        , // 0x1E
    GuiCmd_PageEnd          // GUI_CMD_PAGE_END          0x1F
};

bool ui_DisplayPage( GuiState_st* guiState )
{
    bool     displayPage   = false ;
    uint16_t count ;
    uint8_t  cmd ;
    bool     exit ;
    bool     pageDisplayed = false ;

    if( !guiState->update                               ||
        ( guiState->update && guiState->pageHasEvents )    )
    {
        displayPage = true ;
    }

    if( displayPage )
    {
        if( !guiState->update )
        {
            ui_InitGuiStateLayoutLinks( &guiState->layout );
            ui_InitGuiStateLayoutVars( &guiState->layout );
            gfx_clearScreen();
        }

        if( !guiState->update )
        {
            if( guiState->page.level == 0 )
            {
                guiState->page.levelList[ guiState->page.level ] = guiState->page.num ;
                guiState->page.level++ ;
            }
            else
            {
                if( guiState->page.level < MAX_PAGE_DEPTH )
                {
                    if( guiState->page.num != guiState->page.levelList[ guiState->page.level - 1 ] )
                    {
                        guiState->page.levelList[ guiState->page.level ] = guiState->page.num ;
                        guiState->page.level++ ;
                    }
                }
                else
                {
                    guiState->page.levelList[ guiState->page.level - 1 ] = guiState->page.num ;
                }
            }

        }

        guiState->timeStamp           = getTick();
        guiState->layout.inSelect     = false ;

        guiState->page.ptr            = (uint8_t*)uiPageTable[ guiState->page.num ] ;

        guiState->layout.numOfEntries = 0 ;

        guiState->layout.lineIndex    = 0 ;

        guiState->layout.linkNumOf    = 0 ;
        guiState->layout.linkIndex    = 0 ;
        guiState->layout.varIndex     = 0 ;

        guiState->layout.scrollOffset = 0 ;//@@@KL handle scrolling

        guiState->page.index          = 0 ;
        guiState->page.cmdIndex       = 0 ;

        guiState->layout.line         = guiState->layout.lines[ GUI_LINE_INITIAL ] ;
        guiState->layout.style        = guiState->layout.styles[ GUI_STYLE_INITIAL ] ;

        guiState->page.renderPage     = false ;

        if( !guiState->update )
        {
            guiState->displayEnabledInitial = true ;
            guiState->displayEnabled        = true ;
            guiState->layout.varNumOf       = 0 ;
            guiState->pageHasEvents         = false ;
        }
        else
        {
            guiState->displayEnabledInitial = false ;
            guiState->displayEnabled        = false ;
        }

        for( exit = false , count = 256 ; !exit && count ; count-- )
        {
            cmd = guiState->page.ptr[ guiState->page.index ] ;

            if( cmd < GUI_CMD_DATA_AREA )
            {
                guiState->layout.itemPos.y  = 0 ;
                guiState->layout.itemPos.x  = 0 ;
                guiState->layout.itemPos.h  = 0 ;
                guiState->layout.itemPos.w  = 0 ;
                exit = ui_GuiCmd_Table[ cmd ]( guiState );
                guiState->layout.line.pos.x = guiState->layout.itemPos.x +
                                              guiState->layout.itemPos.w ;
                guiState->page.cmdIndex++ ;
            }
            else
            {
                guiState->page.index++ ;
            }
        }
        pageDisplayed = true ;
    }

    if( guiState->page.renderPage )
    {
        redraw_needed = true ;
    }

    return pageDisplayed ;

}

static void RunScript( GuiState_st* guiState , uint8_t scriptPageNum )
{
    Page_st  parentPage = guiState->page ;
    uint16_t count ;
    uint8_t  cmd ;
    bool     exit ;

    guiState->page.ptr   = (uint8_t*)uiPageTable[ scriptPageNum ] ;

    guiState->page.index = 0 ;

    for( exit = false , count = 256 ; !exit && count ; count-- )
    {
        cmd = guiState->page.ptr[ guiState->page.index ] ;

        if( cmd < GUI_CMD_DATA_AREA )
        {
            exit = ui_GuiCmd_Table[ cmd ]( guiState );
            guiState->page.cmdIndex++ ;
        }
        else
        {
            guiState->page.index++ ;
        }
    }

    guiState->page = parentPage ;

}

// Please Note :- Although NULL is a string end it is also handled as a 'command'
static bool GuiCmd_Null( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_EventStart( GuiState_st* guiState )
{
    uint8_t  eventType ;
    uint8_t  eventPayload ;
    bool     pageEnd = false ;

    guiState->page.index++ ;
    eventType                       = GuiCmd_LdVal( guiState );
    eventPayload                    = GuiCmd_LdVal( guiState );

    guiState->pageHasEvents         = true ;

    guiState->displayEnabledInitial = guiState->displayEnabled ;

    if( !guiState->update )
    {
        guiState->inEventArea = true ;
    }
    else
    {
        if( ( guiState->event.type                       == eventType    ) &&
            ( ( guiState->event.payload & eventPayload ) == eventPayload )    )
        {
            guiState->inEventArea    = true ;
            guiState->displayEnabled = true ;
        }
    }

    return pageEnd ;

}

static bool GuiCmd_EventEnd( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    guiState->inEventArea    = false ;
    guiState->displayEnabled = guiState->displayEnabledInitial ;

    return pageEnd ;

}

static bool GuiCmd_TimerCheck( GuiState_st* guiState )
{
    uint8_t scriptPageNum ;
    bool    pageEnd       = false ;

    guiState->page.index++ ;
    scriptPageNum = GuiCmd_LdVal( guiState );

    if( guiState->initialPageDisplay )
    {
        guiState->timer.timeOut       = 0 ;
        guiState->timer.scriptPageNum = PAGE_STUBBED ;
    }

    if( guiState->timer.timeOut == 0 )
    {
        RunScript( guiState , scriptPageNum );
    }
    else
    {
        if( guiState->timeStamp >= guiState->timer.timeOut )
        {
            guiState->timer.timeOut = 0 ;
            RunScript( guiState , guiState->timer.scriptPageNum );
        }
    }

    return pageEnd ;

}

static bool GuiCmd_TimerSet( GuiState_st* guiState )
{
    uint8_t  period_u ;
    uint8_t  period_l ;
    uint16_t period ;
    uint8_t  scriptPageNum ;
    bool     pageEnd       = false ;

    guiState->page.index++ ;
    scriptPageNum = GuiCmd_LdVal( guiState );
    period_u      = GuiCmd_LdVal( guiState );
    period_l      = GuiCmd_LdVal( guiState );
    period        = ( (uint16_t)period_u * 100 ) + (uint16_t)period_l ;

    if( guiState->timer.timeOut == 0 )
    {
        guiState->timer.timeOut       = guiState->timeStamp + (long long int)period ;
        guiState->timer.scriptPageNum = scriptPageNum ;
    }

    return pageEnd ;

}

static bool GuiCmd_GoToLine( GuiState_st* guiState )
{
    uint8_t select ;
    bool    pageEnd = false ;

    guiState->page.index++ ;

    select                = GuiCmd_LdVal( guiState );

    if( select >= GUI_LINE_NUM_OF )
    {
        select = GUI_LINE_DEFAULT ;
    }

    guiState->layout.line = guiState->layout.lines[ select ] ;

    return pageEnd ;

}

static bool GuiCmd_GoToPos( GuiState_st* guiState )
{
    Pos_st  pos     = guiState->layout.line.pos ;
    uint8_t select ;
    uint8_t val ;
    bool    pageEnd = false ;

    guiState->page.index++ ;

    select = GuiCmd_LdVal( guiState );

    guiState->page.index++ ;

    if( select & GUI_CMD_GOTO_POS_PARA_X )
    {
        val = GuiCmd_LdVal( guiState );
        if( val >= SCREEN_WIDTH )
        {
            pos.x = SCREEN_WIDTH - 1 ;
        }
        else
        {
            pos.x = val ;
        }
        guiState->page.index++ ;
    }
    if( select & GUI_CMD_GOTO_ADD_PARA_X )
    {
        val = GuiCmd_LdVal( guiState );
        if( ( pos.x + val ) >= SCREEN_WIDTH )
        {
            pos.x  = SCREEN_WIDTH - 1 ;
        }
        else
        {
            pos.x += val ;
        }
        guiState->page.index++ ;
    }
    if( select & GUI_CMD_GOTO_SUB_PARA_X )
    {
        val = GuiCmd_LdVal( guiState );
        if( val > pos.x )
        {
            pos.x  = 0 ;
        }
        else
        {
            pos.x -= val ;
        }
        pos.x -= val ;
        guiState->page.index++ ;
    }
    if( select & GUI_CMD_GOTO_POS_PARA_Y )
    {
        val = GuiCmd_LdVal( guiState );
        if( val >= SCREEN_HEIGHT )
        {
            pos.y = SCREEN_HEIGHT - 1 ;
        }
        else
        {
            pos.y = val ;
        }
        guiState->page.index++ ;
    }
    if( select & GUI_CMD_GOTO_ADD_PARA_Y )
    {
        val = GuiCmd_LdVal( guiState );
        if( ( pos.y + val ) >= SCREEN_HEIGHT )
        {
            pos.y  = SCREEN_HEIGHT - 1 ;
        }
        else
        {
            pos.y += val ;
        }
        guiState->page.index++ ;
    }
    if( select & GUI_CMD_GOTO_SUB_PARA_Y )
    {
        val = GuiCmd_LdVal( guiState );
        if( val > pos.y )
        {
            pos.y  = 0 ;
        }
        else
        {
            pos.y -= val ;
        }
        guiState->page.index++ ;
    }

    guiState->layout.line.pos = pos ;

    return pageEnd ;

}

static bool GuiCmd_LoadStyle( GuiState_st* guiState )
{
    uint8_t select ;
    bool    pageEnd = false ;

    guiState->page.index++ ;

    select                 = GuiCmd_LdVal( guiState );

    if( select >= GUI_STYLE_NUM_OF )
    {
        select = GUI_STYLE_DEFAULT ;
    }

    guiState->layout.style = guiState->layout.styles[ select ] ;

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_BGColor( GuiState_st* guiState )
{
    uint8_t color ;
    bool    pageEnd = false ;

    guiState->page.index++ ;

    color = GuiCmd_LdVal( guiState );

    if( color >= COLOR_NUM_OF )
    {
        color = COLOR_RED ;
    }

    guiState->layout.style.colorBG = color ;

    return pageEnd ;

}

static bool GuiCmd_FGColor( GuiState_st* guiState )
{
    uint8_t color ;
    bool    pageEnd = false ;

    guiState->page.index++ ;

    color = GuiCmd_LdVal( guiState );

    if( color >= COLOR_NUM_OF )
    {
        color = COLOR_RED ;
    }

    guiState->layout.style.colorFG = color ;

    return pageEnd ;

}

static bool GuiCmd_LineEnd( GuiState_st* guiState )
{
    bool pageEnd = false ;

    guiState->page.index++ ;

    if( ( guiState->layout.lineIndex + 1 ) < GUI_LINE_NUM_OF )
    {
        guiState->layout.lineIndex++ ;
    }

    guiState->layout.line.pos.y += guiState->layout.menu_h ;
    guiState->layout.line        = guiState->layout.lines[ guiState->layout.lineIndex ] ;
    guiState->layout.style       = guiState->layout.styles[ guiState->layout.lineIndex ] ;

    return pageEnd ;

}

static bool GuiCmd_FontSize( GuiState_st* guiState )
{
    uint8_t fontSize ;
    bool    pageEnd  = false ;

    guiState->page.index++ ;

    fontSize = GuiCmd_LdVal( guiState );

    if( fontSize >= FONT_SIZE_NUM_OF )
    {
        fontSize = FONT_SIZE_5PT ;
    }

    guiState->layout.style.font.size = fontSize ;

    guiState->page.index++ ;

    return pageEnd ;

}

static bool GuiCmd_Align( GuiState_st* guiState )
{
    Style_st style   = guiState->layout.style ;
    uint8_t  select ;
    bool     pageEnd = false ;

    guiState->page.index++ ;

    select = GuiCmd_LdVal( guiState );

    guiState->page.index++ ;

    style.align = 0 ;

    switch( select & GUI_CMD_ALIGN_PARA_MASK_X )
    {
        case GUI_CMD_ALIGN_PARA_LEFT :
        {
            style.align |= ALIGN_LEFT ;
            guiState->page.index++ ;
            break ;
        }
        case GUI_CMD_ALIGN_PARA_CENTER :
        {
            style.align |= ALIGN_CENTER ;
            guiState->page.index++ ;
            break ;
        }
        case GUI_CMD_ALIGN_PARA_RIGHT :
        {
            style.align |= ALIGN_RIGHT ;
            guiState->page.index++ ;
            break ;
        }
    }
#ifdef ENABLE_ALIGN_VERTICAL
    switch( select & GUI_CMD_ALIGN_PARA_MASK_Y )
    {
        case GUI_CMD_ALIGN_PARA_TOP :
        {
            style.align |= ALIGN_TOP ;
            guiState->page.index++ ;
            break ;
        }
        case GUI_CMD_ALIGN_PARA_MIDDLE :
        {
            style.align |= ALIGN_MIDDLE ;
            guiState->page.index++ ;
            break ;
        }
        case GUI_CMD_ALIGN_PARA_BOTTOM :
        {
            style.align |= ALIGN_BOTTOM ;
            guiState->page.index++ ;
            break ;
        }
    }
#endif // ENABLE_ALIGN_VERTICAL
    guiState->layout.style = style ;

    return pageEnd ;

}

static bool GuiCmd_RunScript( GuiState_st* guiState )
{
    uint8_t scriptPageNum ;
    bool    pageEnd       = false ;

    guiState->page.index++ ;
    scriptPageNum = GuiCmd_LdVal( guiState );
    guiState->page.index++ ;

    if( !guiState->update )
    {
        RunScript( guiState , scriptPageNum );
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

    pageEnd = GuiCmd_LinkStart( guiState );

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

    linkNum = GuiCmd_LdVal( guiState );
    guiState->layout.links[ guiState->layout.linkIndex ].type = LINK_TYPE_PAGE ;
    guiState->layout.links[ guiState->layout.linkIndex ].num  = linkNum ;
    guiState->layout.links[ guiState->layout.linkIndex ].amt  = 0 ;

    guiState->page.index++ ;

    return pageEnd ;
}

static bool GuiCmd_Title( GuiState_st* guiState )
{
    uint8_t* scriptPtr ;
    Color_st color_fg ;
    bool     pageEnd   = false ;

    guiState->page.index++ ;

    scriptPtr = &guiState->page.ptr[ guiState->page.index ] ;

    guiState->layout.line  = guiState->layout.lines[ GUI_LINE_TOP ] ;
    guiState->layout.style = guiState->layout.styles[ GUI_STYLE_TOP ] ;

    ui_ColorLoad( &color_fg , guiState->layout.style.colorFG );

    GuiCmd_AdvToNextCmd( guiState );

    if( guiState->displayEnabled )
    {
        // print the title on the top bar
        GuiCmd_Print( guiState , color_fg , (char*)scriptPtr );
        guiState->page.renderPage = true ;
    }

    return pageEnd ;

}

static bool GuiCmd_Text( GuiState_st* guiState )
{
    uint8_t* scriptPtr ;
    Color_st color_fg ;
    Color_st color_bg ;
    Color_st color_text ;
    bool     pageEnd    = false ;

    ui_ColorLoad( &color_fg , guiState->layout.style.colorFG );
    ui_ColorLoad( &color_bg , guiState->layout.style.colorBG );
    color_text = color_fg ;

    guiState->page.index++ ;

    scriptPtr  = &guiState->page.ptr[ guiState->page.index ] ;

    if( guiState->layout.inSelect )
    {
        color_text      = color_bg ;
        // Draw rectangle under selected item, compensating for text height
        Pos_st rect_pos = { 0 , guiState->layout.line.pos.y - guiState->layout.menu_h + 3 ,
                            SCREEN_WIDTH , guiState->layout.menu_h };
        gfx_drawRect( &rect_pos , &color_fg , true );
    }

    GuiCmd_AdvToNextCmd( guiState );

    if( guiState->displayEnabled )
    {
//@@@KL            announceMenuItemIfNeeded( entryBuf , NULL , false );
        GuiCmd_Print( guiState , color_text , (char*)scriptPtr );
        guiState->page.renderPage = true ;
    }

    return pageEnd ;

}

static bool GuiCmd_ValueDisplay( GuiState_st* guiState )
{
    uint8_t valueNum ;
    bool    pageEnd  = false ;

    guiState->page.index++ ;

    valueNum = GuiCmd_LdVal( guiState );

    if( valueNum >= GUI_VAL_DSP_NUM_OF )
    {
        valueNum = GUI_VAL_DSP_STUBBED ;
    }

    if( !guiState->update )
    {
        guiState->layout.vars[ guiState->layout.varIndex ].varNum = valueNum ;
        guiState->layout.varNumOf++ ;
        guiState->layout.links[ guiState->layout.linkIndex ].type = LINK_TYPE_VALUE ;
        guiState->layout.links[ guiState->layout.linkIndex ].num  = valueNum ;
        guiState->layout.links[ guiState->layout.linkIndex ].amt  = 0 ;
        guiState->layout.linkIndex++ ;
    }

    guiState->layout.varIndex++ ;

    if( guiState->displayEnabled )
    {
        GuiVal_DisplayValue( guiState , valueNum );
        guiState->page.renderPage = true ;
    }

    return pageEnd ;

}

static bool GuiCmd_ValueInput( GuiState_st* guiState )
{
    uint8_t valueNum ;
    bool    pageEnd  = false ;

    guiState->page.index++ ;

    valueNum = GuiCmd_LdVal( guiState );

    ui_ValueInput( guiState , valueNum );

    guiState->page.renderPage = true ;

    return pageEnd ;

}

static bool GuiCmd_DrawLine( GuiState_st* guiState )
{
    Pos_st   pos ;
    Pos_st   startPos = guiState->layout.line.pos ;
    Pos_st   endPos   = startPos ;
    Color_st color ;
    bool     pageEnd  = false ;

    ui_ColorLoad( &color , guiState->layout.style.colorFG );

    pos.w = GuiCmd_LdVal( guiState );
    pos.h = GuiCmd_LdVal( guiState );

    endPos.x += pos.w ;
    endPos.y += pos.h ;

    if( endPos.x > SCREEN_WIDTH )
    {
        endPos.x = SCREEN_WIDTH - 1 ;
    }

    if( endPos.y > SCREEN_HEIGHT )
    {
        endPos.y = SCREEN_HEIGHT - 1 ;
    }

    gfx_drawLine( &startPos , &endPos , &color );

    guiState->layout.line.pos = endPos ;

    return pageEnd ;

}

static bool GuiCmd_DrawRect( GuiState_st* guiState )
{
    Pos_st   pos      = guiState->layout.line.pos ;
    Color_st color ;
    bool     pageEnd  = false ;

    ui_ColorLoad( &color , guiState->layout.style.colorFG );

    pos.w = GuiCmd_LdVal( guiState );
    pos.h = GuiCmd_LdVal( guiState );

    gfx_drawRect( &pos , &color , false );

    return pageEnd ;

}

static bool GuiCmd_DrawRectFilled( GuiState_st* guiState )
{
    Pos_st   pos      = guiState->layout.line.pos ;
    Color_st color ;
    bool     pageEnd  = false ;

    ui_ColorLoad( &color , guiState->layout.style.colorFG );

    pos.w = GuiCmd_LdVal( guiState );
    pos.h = GuiCmd_LdVal( guiState );

    gfx_drawRect( &pos , &color , true );

    return pageEnd ;

}

#ifdef DISPLAY_DEBUG_MSG
static bool GuiCmd_SetDbgMsg( GuiState_st* guiState )
{
    uint8_t* scriptPtr ;
    bool     pageEnd   = false ;

    guiState->page.index++ ;

    scriptPtr = &guiState->page.ptr[ guiState->page.index ] ;

    GuiVal_SetDebugMessage( (char*)scriptPtr );

    GuiCmd_AdvToNextCmd( guiState );

    return pageEnd ;

}
#endif // DISPLAY_DEBUG_MSG

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

static bool GuiCmd_LinkStart( GuiState_st* guiState )
{
    bool    pageEnd = false ;

    if( guiState->layout.linkIndex == 0 )
    {
        // Number of menu entries that fit in the screen height
        guiState->layout.numOfEntries = ( SCREEN_HEIGHT - 1 - guiState->layout.line.pos.y ) /
                                        guiState->layout.menu_h + 1 ;
    }

    // If selection is off the screen, scroll screen
    if( guiState->uiState.entrySelected >= guiState->layout.numOfEntries )
    {
        guiState->layout.scrollOffset = guiState->uiState.entrySelected -
                                        guiState->layout.numOfEntries + 1 ;
    }

    guiState->layout.inSelect = false ;

    if( (   guiState->layout.linkIndex >= guiState->layout.scrollOffset ) &&
        ( ( guiState->layout.linkIndex  - guiState->layout.scrollOffset )  < guiState->layout.numOfEntries ) )
    {
        if( guiState->layout.linkIndex == guiState->uiState.entrySelected )
        {
            guiState->layout.inSelect = true ;
        }
    }

    return pageEnd ;

}

#ifndef ENABLE_SYSTEM_VARIABLES
static uint8_t GuiCmd_LdVal( GuiState_st* guiState )
{
    uint8_t value ;

    value = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->page.index++ ;

    return value ;
}
#else // ENABLE_SYSTEM_VARIABLES
static uint8_t GuiCmd_LdVal( GuiState_st* guiState )
{
    bool    selectorPresent = false ;
    uint8_t selector ;
    uint8_t operation ;
    bool    modifierPresent = false ;
    uint8_t modifier ;
    uint8_t value ;

    value     = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->page.index++ ;

    selector  = value ;

    value    &= GUI_CMD_PARA_VALUE_MASK ;

    if( selector & GUI_CMD_PARA_SYSTEM_FLAG )
    {
        selectorPresent = true ;

        if( selector & GUI_CMD_PARA_MODIFIER_FLAG )
        {
            operation  = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
            guiState->page.index++ ;

            modifier   = operation & GUI_CMD_PARA_VALUE_MASK ;
            operation &= ~GUI_CMD_PARA_VALUE_MASK ;

            if( modifier & GUI_CMD_PARA_SIGNED_FLAG )
            {
                modifier |= ~GUI_CMD_PARA_VALUE_MASK ;
            }

            modifierPresent = true ;

        }

        if( value & GUI_CMD_PARA_SIGNED_FLAG )
        {
            value |= ~GUI_CMD_PARA_VALUE_MASK ;
        }

    }
    else
    {
        if( value & ( GUI_CMD_PARA_SIGNED_FLAG << 1 ) )
        {
            value |= ~ ( GUI_CMD_PARA_VALUE_MASK | GUI_CMD_PARA_SIGNED_FLAG ) ;
        }
    }

    selector &= GUI_CMD_PARA_VALUE_MASK ;

    // determine whether the value is a system variable selector present
    if( selectorPresent )
    {
        // if so - set the value to the specified system variable
        switch( selector )
        {
            case GUI_CMD_PARA_SYS_SCREEN_WIDTH :
            {
                value = SCREEN_WIDTH ;
                break ;
            }
            case GUI_CMD_PARA_SYS_SCREEN_HEIGHT :
            {
                value = GUI_CMD_PARA_SYS_SCREEN_HEIGHT ;
                break ;
            }
            case GUI_CMD_PARA_SYS_LINE_HEIGHT :
            {
                value = (uint8_t)guiState->layout.line.height ;
                break ;
            }
            case GUI_CMD_PARA_SYS_FONT_SIZE :
            {
                value = (uint8_t)guiState->layout.style.font.size ;
                break ;
            }
            case GUI_CMD_PARA_SYS_BG_COLOR :
            {
                value = (uint8_t)guiState->layout.style.colorBG ;
                break ;
            }
            case GUI_CMD_PARA_SYS_FG_COLOR :
            {
                value = (uint8_t)guiState->layout.style.colorFG ;
                break ;
            }
        }
        // determine whether there is a modifier present
        if( modifierPresent )
        {
            // modify the variable
            switch( operation )
            {
                case GUI_CMD_PARA_MOD_ADD :
                {
                    value += modifier ;
                    break ;
                }
                case GUI_CMD_PARA_MOD_SUB :
                {
                    value -= modifier ;
                    break ;
                }
            }
        }
    }
    return value ;
}
#endif // ENABLE_SYSTEM_VARIABLES

static void GuiCmd_AdvToNextCmd( GuiState_st* guiState )
{
    uint16_t count = 256 ;

    if( guiState->page.ptr[ guiState->page.index ] < GUI_CMD_DATA_AREA )
    {
        guiState->page.index++ ;
    }

    while( guiState->page.ptr[ guiState->page.index ] >= GUI_CMD_DATA_AREA )
    {
        count-- ;
        if( count == 0 )
        {
            break ;
        }
        guiState->page.index++ ;
    }

}

static void GuiCmd_Print( GuiState_st* guiState , Color_st color , char* scriptPtr )
{
    guiState->layout.itemPos  = gfx_print( &guiState->layout.line.pos       ,
                                           guiState->layout.style.font.size ,
                                           guiState->layout.style.align     ,
                                           &color , scriptPtr );
    guiState->page.renderPage = true ;

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
