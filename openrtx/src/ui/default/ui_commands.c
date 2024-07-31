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
#include "ui_list_display.h"
#include "ui_value_display.h"
#include "ui_states.h"
#include "ui_value_input.h"

enum
{
    MAX_NUM_PAGE_BYTES = 256
};

static const uint32_t ColorTable[ COLOR_NUM_OF ] =
{
    COLOR_TABLE
};

static void ui_DisplayPage( GuiState_st* guiState );

static void RunScript( GuiState_st* guiState , uint8_t scriptPageNum );
static void DisplayList( GuiState_st* guiState );

static bool GuiCmd_Null( GuiState_st* guiState );
static bool GuiCmd_EventStart( GuiState_st* guiState );
static bool GuiCmd_EventEnd( GuiState_st* guiState );
static bool GuiCmd_OnEvent( GuiState_st* guiState );
static bool GuiCmd_TimerCheck( GuiState_st* guiState );
static bool GuiCmd_TimerSet( GuiState_st* guiState );
static bool GuiCmd_GoToTextLine( GuiState_st* guiState );
static bool GuiCmd_LoadStyle( GuiState_st* guiState );
static bool GuiCmd_BGColor( GuiState_st* guiState );
static bool GuiCmd_FGColor( GuiState_st* guiState );
static bool GuiCmd_LineEnd( GuiState_st* guiState );
static bool GuiCmd_FontSize( GuiState_st* guiState );
static bool GuiCmd_Align( GuiState_st* guiState );
static bool GuiCmd_RunScript( GuiState_st* guiState );
static bool GuiCmd_List( GuiState_st* guiState );
static bool GuiCmd_ListElement( GuiState_st* guiState );
static bool GuiCmd_Link( GuiState_st* guiState );
static bool GuiCmd_Title( GuiState_st* guiState );
static bool GuiCmd_Text( GuiState_st* guiState );
static bool GuiCmd_ValueDisplay( GuiState_st* guiState );
static bool GuiCmd_ValueInput( GuiState_st* guiState );
static bool GuiCmd_GoToPosX( GuiState_st* guiState );
static bool GuiCmd_GoToPosY( GuiState_st* guiState );
static bool GuiCmd_AddToPosX( GuiState_st* guiState );
static bool GuiCmd_AddToPosY( GuiState_st* guiState );
static bool GuiCmd_DrawGraphic( GuiState_st* guiState );
static bool GuiCmd_DrawLine( GuiState_st* guiState );
static bool GuiCmd_DrawRect( GuiState_st* guiState );
static bool GuiCmd_DrawRectFilled( GuiState_st* guiState );
static bool GuiCmd_DrawCircle( GuiState_st* guiState );
static bool GuiCmd_Operation( GuiState_st* guiState );
// debug fns
#ifdef ENABLE_DEBUG_MSG
  #ifndef DISPLAY_DEBUG_MSG
static bool GuiCmd_DispDgbVal( GuiState_st* guiState );
  #else // DISPLAY_DEBUG_MSG
static bool GuiCmd_SetDbgMsg( GuiState_st* guiState );
  #endif // DISPLAY_DEBUG_MSG
#endif // ENABLE_DEBUG_MSG
static bool GuiCmd_PageEnd( GuiState_st* guiState );
static bool GuiCmd_Stubbed( GuiState_st* guiState );

static void     LineStart( GuiState_st* guiState );
static void     LineEnd( GuiState_st* guiState );
static uint8_t  GuiCmd_LdValUI( GuiState_st* guiState );
static int8_t   GuiCmd_LdValI( GuiState_st* guiState );
static uint16_t GuiCmd_LdValUIWord( GuiState_st* guiState );
static uint32_t GuiCmd_LdValUILong( GuiState_st* guiState );

static void     GuiCmd_AdvToNextCmd( GuiState_st* guiState );

static void     GuiCmd_Print( GuiState_st* guiState , Color_st color , char* scriptPtr );
static void     BlankLine( GuiState_st* guiState );

typedef bool (*ui_GuiCmd_fn)( GuiState_st* guiState );

static const ui_GuiCmd_fn ui_GuiCmd_Table[ GUI_CMD_NUM_OF ] =
{
    GuiCmd_Null           , // GUI_CMD_NULL              0x00
    GuiCmd_EventStart     , // GUI_CMD_EVENT_START       0x01
    GuiCmd_EventEnd       , // GUI_CMD_EVENT_END         0x02
    GuiCmd_OnEvent        , // GUI_CMD_ON_EVENT          0x03
    GuiCmd_TimerCheck     , // GUI_CMD_TIMER_CHECK       0x04
    GuiCmd_TimerSet       , // GUI_CMD_TIMER_SET         0x05
    GuiCmd_GoToTextLine   , // GUI_CMD_GOTO_TEXT_LINE    0x06
    GuiCmd_LoadStyle      , // GUI_CMD_LOAD_STYLE        0x07
    GuiCmd_BGColor        , // GUI_CMD_BG_COLOR          0x08
    GuiCmd_FGColor        , // GUI_CMD_FG_COLOR          0x09
    GuiCmd_LineEnd        , // GUI_CMD_LINE_END          0x0A
    GuiCmd_FontSize       , // GUI_CMD_FONT_SIZE         0x0B
    GuiCmd_Align          , // GUI_CMD_ALIGN             0x0C
    GuiCmd_RunScript      , // GUI_CMD_RUN_SCRIPT        0x0D
    GuiCmd_List           , // GUI_CMD_LIST              0x0E
    GuiCmd_ListElement    , // GUI_CMD_LIST_ELEMENT      0x0F
    GuiCmd_Link           , // GUI_CMD_LINK              0x10
    GuiCmd_Title          , // GUI_CMD_TITLE             0x11
    GuiCmd_Text           , // GUI_CMD_TEXT              0x12
    GuiCmd_ValueDisplay   , // GUI_CMD_VALUE_DSP         0x13
    GuiCmd_ValueInput     , // GUI_CMD_VALUE_INP         0x14
    GuiCmd_GoToPosX       , // GUI_CMD_GOTO_POS_X        0x15
    GuiCmd_GoToPosY       , // GUI_CMD_GOTO_POS_Y        0x16
    GuiCmd_AddToPosX      , // GUI_CMD_ADD_POS_X         0x17
    GuiCmd_AddToPosY      , // GUI_CMD_ADD_POS_Y         0x18
    GuiCmd_DrawGraphic    , // GUI_CMD_DRAW_GRAPHIC      0x19
    GuiCmd_Operation      , // GUI_CMD_OPERATION         0x1A
    GuiCmd_Stubbed        , // 0x1B
    GuiCmd_Stubbed        , // 0x1C
#ifdef ENABLE_DEBUG_MSG
  #ifndef DISPLAY_DEBUG_MSG
    GuiCmd_DispDgbVal     , // GUI_CMD_DBG_VAL           0x1D
  #else // DISPLAY_DEBUG_MSG
    GuiCmd_SetDbgMsg      , // GUI_CMD_SET_DBG_MSG       0x1D
  #endif // DISPLAY_DEBUG_MSG
#else // ENABLE_DEBUG_MSG
    GuiCmd_Stubbed        , // GUI_CMD_STUBBED           0x1D
#endif // ENABLE_DEBUG_MSG
    GuiCmd_Stubbed        , // 0x1E
    GuiCmd_PageEnd          // GUI_CMD_PAGE_END          0x1F
};

static const ui_GuiCmd_fn ui_GuiGraphic_Table[ GUI_CMD_GRAPHIC_NUM_OF ] =
{
    GuiCmd_DrawLine       , // GUI_CMD_GRAPHIC_LINE         0x00
    GuiCmd_DrawRect       , // GUI_CMD_GRAPHIC_RECT         0x01
    GuiCmd_DrawRectFilled , // GUI_CMD_GRAPHIC_RECT_FILLED  0x02
    GuiCmd_DrawCircle       // GUI_CMD_GRAPHIC_CIRCLE       0x03
};

void ui_Draw_Page( GuiState_st* guiState )
{
    static uint8_t prevPageNum = ~0 ;
           bool    drawPage    = false ;

    do
    {
        guiState->update = false ;

        if( guiState->page.num != prevPageNum )
        {
            prevPageNum = guiState->page.num ;
            drawPage    = true ;
        }
        else
        {
            switch( guiState->event.type )
            {
                case EVENT_TYPE_KBD :
                {
                    guiState->event.payload = guiState->msg.keys ;
                    drawPage = true ;
                    break ;
                }
                case EVENT_TYPE_STATUS :
                {
                    if( guiState->event.payload & EVENT_STATUS_DEVICE )
                    {
                        drawPage = true ;
                    }
                    break ;
                }
                case EVENT_TYPE_RTX :
                {
                    drawPage = true ;
                    break ;
                }
            }
            if( drawPage )
            {
                guiState->update = true ;
            }
        }

        if( drawPage )
        {
            ui_DisplayPage( guiState );

            if( guiState->page.renderPage )
            {
                guiState->page.renderPage = false ;
                redraw_needed             = true ;
            }
            else
            {
                redraw_needed             = false ;
            }

        }
        else
        {
            redraw_needed = false ;
        }

    }while( guiState->page.num != prevPageNum );

}

static void ui_DisplayPage( GuiState_st* guiState )
{
    bool    displayPage = false ;
    uint8_t cmd ;
    bool    exit ;

    if( !guiState->update       ||
        guiState->pageHasEvents    )
    {
        displayPage = true ;
    }

    if( displayPage )
    {
        if( !guiState->update )
        {
            ui_InitGuiStateLayoutList( &guiState->layout );
            ui_InitGuiStateLayoutLink( &guiState->layout );
            ui_InitGuiStateLayoutVars( &guiState->layout );
            gfx_clearScreen();

            if( guiState->page.level == 0 )
            {
                guiState->page.levelList[ guiState->page.level ] = guiState->page.num ;
                guiState->page.level++ ;
            }

        }

        guiState->timeStamp              = getTick();

        guiState->page.ptr               = (uint8_t*)uiPageTable[ guiState->page.num ] ;

        guiState->layout.numOfEntries    = 0 ;

        guiState->layout.lineIndex       = 0 ;

        guiState->layout.varIndex        = 0 ;

        guiState->layout.scrollOffset    = 0 ;

        guiState->layout.varInputDisplay = false ;

        guiState->layout.line            = guiState->layout.lines[ GUI_LINE_INITIAL ] ;
        guiState->layout.style           = guiState->layout.styles[ GUI_STYLE_INITIAL ] ;

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

        guiState->page.renderPage        = false ;

        guiState->page.index             = 0 ;

        LineStart( guiState );

        exit = false ;
        while( !exit && ( guiState->page.index < MAX_NUM_PAGE_BYTES ) )
        {
            cmd = guiState->page.ptr[ guiState->page.index ] ;
            guiState->page.index++ ;

            if( cmd < GUI_CMD_DATA_AREA )
            {
                exit = ui_GuiCmd_Table[ cmd ]( guiState );
            }
        }

    }

}

static void RunScript( GuiState_st* guiState , uint8_t scriptPageNum )
{
    uint8_t* parentPtr   = guiState->page.ptr ;
    uint16_t parentIndex = guiState->page.index ;
    uint8_t  cmd ;
    bool     exit ;

    guiState->page.ptr   = (uint8_t*)uiPageTable[ scriptPageNum ] ;
    guiState->page.index = 0 ;

    exit = false ;
    while( !exit && ( guiState->page.index < MAX_NUM_PAGE_BYTES ) )
    {
        cmd = guiState->page.ptr[ guiState->page.index ] ;
        guiState->page.index++ ;

        if( cmd < GUI_CMD_DATA_AREA )
        {
            exit = ui_GuiCmd_Table[ cmd ]( guiState );
        }
    }

    guiState->page.ptr   = parentPtr ;
    guiState->page.index = parentIndex ;

}

// Please Note :- Although NULL is a string end it is also handled as a 'command'
static bool GuiCmd_Null( GuiState_st* guiState )
{
    (void)guiState ;
    bool pageEnd = false ;

    return pageEnd ;

}

static bool GuiCmd_EventStart( GuiState_st* guiState )
{
    uint32_t scriptEventValue ;
    uint32_t scriptEventType ;
    uint32_t scriptEventPayload ;
    bool     pageEnd = false ;

    scriptEventValue   = GuiCmd_LdValUILong( guiState );
    scriptEventType    = scriptEventValue >> EVENT_TYPE_SHIFT ;
    scriptEventPayload = scriptEventValue & EVENT_PAYLOAD_MASK ;

    guiState->displayEnabledInitial = guiState->displayEnabled ;

    if( !guiState->update )
    {
        guiState->pageHasEvents = true ;
        guiState->inEventArea   = true ;
    }
    else
    {
        if( ( guiState->event.type    == scriptEventType    ) &&
            ( guiState->event.payload  & scriptEventPayload )    )
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

    guiState->inEventArea    = false ;
    guiState->displayEnabled = guiState->displayEnabledInitial ;

    return pageEnd ;

}

static bool GuiCmd_OnEvent( GuiState_st* guiState )
{
    uint32_t scriptEventValue ;
    uint32_t scriptEventType ;
    uint32_t scriptEventPayload ;
    uint8_t  action ;
    uint8_t  parameter ;
    bool     pageEnd = false ;

    scriptEventValue   = GuiCmd_LdValUILong( guiState );
    scriptEventType    = scriptEventValue >> EVENT_TYPE_SHIFT ;
    scriptEventPayload = scriptEventValue & EVENT_PAYLOAD_MASK ;

    action             = GuiCmd_LdValUI( guiState );
    parameter          = GuiCmd_LdValUI( guiState );

    if( !guiState->update )
    {
        guiState->pageHasEvents = true ;

        switch( action )
        {
            case ON_EVENT_ACTION_GO_BACK :
            {
                if( guiState->page.level )
                {
                    if( guiState->page.levelList[ guiState->page.level - 1 ] == guiState->page.num )
                    {
                        guiState->page.level-- ;
                    }
                }
                if( guiState->page.level < MAX_PAGE_DEPTH )
                {
                    guiState->page.levelList[ guiState->page.level ] = guiState->page.num ;
                    guiState->page.level++ ;
                }
                break ;
            }
        }
    }
    else
    {
        if( ( guiState->event.type    == scriptEventType    ) &&
            ( guiState->event.payload == scriptEventPayload )    )
        {
            switch( action )
            {
                case ON_EVENT_ACTION_GOTO_PAGE :
                {
                    guiState->page.num = parameter ;
                    break ;
                }
                case ON_EVENT_ACTION_GO_BACK :
                {
                    if( guiState->page.level )
                    {
                        guiState->page.level-- ;
                    }
                    if( guiState->page.level )
                    {
                        guiState->page.level-- ;
                    }
                    guiState->page.num = guiState->page.levelList[ guiState->page.level ] ;
                    break ;
                }
            }
        }
    }

    return pageEnd ;

}

static bool GuiCmd_TimerCheck( GuiState_st* guiState )
{
    uint8_t scriptPageNum ;
    bool    pageEnd       = false ;

    scriptPageNum    = GuiCmd_LdValUI( guiState );

    if( !guiState->update )
    {
        guiState->pageHasEvents       = true ;
        guiState->inEventArea         = true ;
        guiState->displayEnabled      = true ;
        guiState->timer.timeOut       = 0 ;
        guiState->timer.scriptPageNum = scriptPageNum ;
        RunScript( guiState , guiState->timer.scriptPageNum );
    }
    else
    {
        if( guiState->timeStamp >= guiState->timer.timeOut )
        {
            guiState->update                = false ;
            guiState->inEventArea           = true ;
            guiState->displayEnabledInitial = guiState->displayEnabled ;
            guiState->displayEnabled        = true ;
            guiState->timer.timeOut         = 0 ;
            RunScript( guiState , guiState->timer.scriptPageNum );
            guiState->inEventArea           = false ;
            guiState->displayEnabled        = guiState->displayEnabledInitial ;
            guiState->update                = true ;
        }
    }

    return pageEnd ;

}

static bool GuiCmd_TimerSet( GuiState_st* guiState )
{
    uint16_t period ;
    uint8_t  scriptPageNum ;
    bool     pageEnd       = false ;

    scriptPageNum = GuiCmd_LdValUI( guiState );
    period        = GuiCmd_LdValUIWord( guiState );

    if( guiState->timer.timeOut == 0 )
    {
        guiState->timer.timeOut       = guiState->timeStamp + (long long int)period ;
        guiState->timer.scriptPageNum = scriptPageNum ;
    }

    return pageEnd ;

}

static bool GuiCmd_GoToTextLine( GuiState_st* guiState )
{
    uint8_t select ;
    bool    pageEnd = false ;

    select                = GuiCmd_LdValUI( guiState );

    if( select >= GUI_LINE_NUM_OF )
    {
        select = GUI_LINE_DEFAULT ;
    }

    guiState->layout.lineIndex = select ;

    guiState->layout.line  = guiState->layout.lines[ guiState->layout.lineIndex ] ;
    guiState->layout.style = guiState->layout.styles[ guiState->layout.lineIndex ] ;

    return pageEnd ;

}

static bool GuiCmd_LoadStyle( GuiState_st* guiState )
{
    uint8_t select ;
    bool    pageEnd = false ;

    select                 = GuiCmd_LdValUI( guiState );

    if( select >= GUI_STYLE_NUM_OF )
    {
        select = GUI_STYLE_DEFAULT ;
    }

    guiState->layout.style = guiState->layout.styles[ select ] ;

    return pageEnd ;

}

static bool GuiCmd_BGColor( GuiState_st* guiState )
{
    uint8_t color ;
    bool    pageEnd = false ;

    color = GuiCmd_LdValUI( guiState );

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

    color = GuiCmd_LdValUI( guiState );

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

    LineEnd( guiState );

    LineStart( guiState );

    return pageEnd ;

}

static void LineStart( GuiState_st* guiState )
{
    uint8_t colorBG ;
    uint8_t colorFG ;

    guiState->layout.line  = guiState->layout.lines[ guiState->layout.lineIndex ] ;
    guiState->layout.style = guiState->layout.styles[ guiState->layout.lineIndex ] ;

    if( guiState->layout.list.displayingList )
    {
        if( guiState->inEventArea )
        {
            if( guiState->layout.list.index == guiState->layout.list.selection )
            {
                colorBG = guiState->layout.style.colorBG ;
                colorFG = guiState->layout.style.colorFG ;
                guiState->layout.style.colorBG = colorFG ;
                guiState->layout.style.colorFG = colorBG ;
            }
            if( !guiState->update )
            {
                BlankLine( guiState );
            }
        }
    }

    if( guiState->layout.line.pos.y >= SCREEN_HEIGHT )
    {
        guiState->displayEnabled = false ;
    }

}

static void LineEnd( GuiState_st* guiState )
{
    uint8_t colorBG ;
    uint8_t colorFG ;

    if( guiState->layout.list.displayingList )
    {
        if( guiState->inEventArea )
        {
            if( guiState->layout.list.index == guiState->layout.list.selection )
            {
                colorBG = guiState->layout.style.colorBG ;
                colorFG = guiState->layout.style.colorFG ;
                guiState->layout.style.colorBG = colorFG ;
                guiState->layout.style.colorFG = colorBG ;
            }
        }
        guiState->layout.list.index++ ;
    }

    if( ( guiState->layout.lineIndex + 1 ) < GUI_LINE_NUM_OF )
    {
        guiState->layout.lineIndex++ ;
    }

}

static bool GuiCmd_FontSize( GuiState_st* guiState )
{
    uint8_t fontSize ;
    bool    pageEnd  = false ;

    fontSize = GuiCmd_LdValUI( guiState );

    if( fontSize >= FONT_SIZE_NUM_OF )
    {
        fontSize = FONT_SIZE_5PT ;
    }

    guiState->layout.style.font.size = fontSize ;

    return pageEnd ;

}

static bool GuiCmd_Align( GuiState_st* guiState )
{
    Style_st style   = guiState->layout.style ;
    uint8_t  select ;
    bool     pageEnd = false ;

    select = GuiCmd_LdValUI( guiState );

    style.align = 0 ;

    switch( select & GUI_CMD_ALIGN_PARA_MASK_X )
    {
        case GUI_CMD_ALIGN_PARA_LEFT :
        {
            style.align |= GFX_ALIGN_LEFT ;
            break ;
        }
        case GUI_CMD_ALIGN_PARA_CENTER :
        {
            style.align |= GFX_ALIGN_CENTER ;
            break ;
        }
        case GUI_CMD_ALIGN_PARA_RIGHT :
        {
            style.align |= GFX_ALIGN_RIGHT ;
            break ;
        }
    }
#ifdef ENABLE_ALIGN_VERTICAL
    switch( select & GUI_CMD_ALIGN_PARA_MASK_Y )
    {
        case GUI_CMD_ALIGN_PARA_TOP :
        {
            style.align |= ALIGN_TOP ;
            break ;
        }
        case GUI_CMD_ALIGN_PARA_MIDDLE :
        {
            style.align |= ALIGN_MIDDLE ;
            break ;
        }
        case GUI_CMD_ALIGN_PARA_BOTTOM :
        {
            style.align |= ALIGN_BOTTOM ;
            break ;
        }
    }
#endif // ENABLE_ALIGN_VERTICAL
    guiState->layout.style = style ;

    return pageEnd ;

}

static bool GuiCmd_RunScript( GuiState_st* guiState )
{
    uint8_t pageNum ;
    bool    pageEnd = false ;

    pageNum = GuiCmd_LdValUI( guiState );

    RunScript( guiState , pageNum );

    return pageEnd ;
}

static bool GuiCmd_List( GuiState_st* guiState )
{
    bool displayList = false ;
    bool pageEnd     = false ;

    guiState->layout.list.pageNum             = GuiCmd_LdValUI( guiState );
    guiState->layout.list.dataSource          = GuiCmd_LdValUI( guiState );
    if( guiState->layout.list.dataSource >= LIST_DATA_SOURCE_NUM_OF )
    {
        guiState->layout.list.dataSource = LIST_DATA_SOURCE_STUBBED ;
    }
    guiState->layout.list.numOfDisplayedLines = GuiCmd_LdValUI( guiState );

    if( !guiState->update )
    {
        guiState->layout.list.pos          = guiState->layout.line.pos ;
        guiState->layout.list.numOfEntries = 0 ;
        guiState->layout.list.selection    = 0 ;
        guiState->layout.list.offset       = 0 ;
        displayList                        = true ;

        // determine the number of list lines
        List_GetNumOfEntries( guiState );

    }
    else
    {
        if( guiState->event.type == EVENT_TYPE_KBD )
        {
            switch( guiState->event.payload )
            {
                case KEY_UP :
//                case KEY_VOLUP :
                {
                    if( guiState->layout.list.selection )
                    {
                        guiState->layout.list.selection-- ;
                        displayList = true ;
                    }
                    else
                    {
                        if( guiState->layout.list.offset )
                        {
                            guiState->layout.list.offset-- ;
                            displayList = true ;
                        }
                    }
                    break ;
                }
                case KEY_DOWN :
//                case KEY_VOLDOWN :
                {
                    if( ( guiState->layout.list.selection + 1  ) <
                        ( guiState->layout.list.numOfEntries -
                          guiState->layout.list.offset         )    )
                    {
                        if( ( guiState->layout.list.selection + 1     ) <
                            guiState->layout.list.numOfDisplayedLines     )
                        {
                            guiState->layout.list.selection++ ;
                            displayList = true ;
                        }
                        else
                        {
                            if( ( guiState->layout.list.offset +
                                  guiState->layout.list.numOfDisplayedLines ) <
                                guiState->layout.list.numOfEntries              )
                            {
                                guiState->layout.list.offset++ ;
                                displayList = true ;
                            }
                        }
                    }
                    break ;
                }
                case KEY_ENTER :
                {
                    List_EntrySelect( guiState );
                    break ;
                }
            }
        }
    }

    if( displayList )
    {
        DisplayList( guiState );
    }

    return pageEnd ;
}

void GuiCmd_List_GetNumOfEntries_Script( GuiState_st* guiState )
{
    uint8_t* ptr ;
    uint16_t index ;
    uint8_t  cmd ;

    for( ptr = (uint8_t*)uiPageTable[ guiState->layout.list.pageNum ] , index = 0 ;
         index < MAX_NUM_PAGE_BYTES ;
         index++ )
    {
        cmd = ptr[ index ] ;

        if( cmd == GUI_CMD_LINE_END )
        {
            guiState->layout.list.numOfEntries++ ;
        }
        else
        {
            if( cmd == GUI_CMD_PAGE_END )
            {
                guiState->layout.list.numOfEntries++ ;
                break ;
            }
        }
    }

}

static void DisplayList( GuiState_st* guiState )
{
    Page_st  parentPage = guiState->page ;
    uint8_t  offset ;
    bool     update ;
    uint8_t  cmd ;
    bool     exit ;

    guiState->page.num   = guiState->layout.list.pageNum ;
    guiState->page.ptr   = (uint8_t*)uiPageTable[ guiState->page.num ] ;

    guiState->page.index = 0 ;

    guiState->layout.list.displayingList = true ;
    guiState->layout.list.index          = 0 ;

    update           = guiState->update ;
    guiState->update = false ;

    if( guiState->layout.list.dataSource == LIST_DATA_SOURCE_SCRIPT )
    {
        // go to the start of the list - entry at offset position
        offset = 0 ;
        while( offset < guiState->layout.list.offset )
        {
            cmd = guiState->page.ptr[ guiState->page.index ] ;

            if( cmd == GUI_CMD_LINE_END )
            {
                offset++ ;
            }
            else
            {
                if( cmd == GUI_CMD_PAGE_END )
                {
                    offset++ ;
                    break ;
                }
            }

            guiState->page.index++ ;

        }
    }

    LineStart( guiState );

    exit = false ;
    while( !exit && ( guiState->page.index < MAX_NUM_PAGE_BYTES ) )
    {
        cmd = guiState->page.ptr[ guiState->page.index ] ;
        guiState->page.index++ ;

        if( cmd < GUI_CMD_DATA_AREA )
        {
            exit = ui_GuiCmd_Table[ cmd ]( guiState );
        }

        if( guiState->layout.list.index == guiState->layout.list.numOfDisplayedLines )
        {
            break ;
        }

    }

    guiState->update = update ;

    guiState->layout.list.displayingList = false ;

    guiState->page.renderPage  = true ;
    parentPage.renderPage     |= guiState->page.renderPage ;
    guiState->page             = parentPage ;

}

static bool GuiCmd_ListElement( GuiState_st* guiState )
{
    bool pageEnd = false ;

    List_EntryDisplay( guiState );

    return pageEnd ;
}

bool GuiCmd_List_EntryDisplay_Script( GuiState_st* guiState )
{
    bool pageEnd = false ;

    return pageEnd ;
}

void GuiCmd_List_EntrySelect_Script( GuiState_st* guiState )
{
    guiState->page.num = guiState->layout.link.pageNum ;
}

bool GuiCmd_List_EntryDisplay_Stubbed( GuiState_st* guiState )
{
    bool pageEnd = GuiCmd_Text( guiState );

    return pageEnd ;
}

void GuiCmd_List_EntryDisplay_TextString( GuiState_st* guiState , char* str )
{
    Color_st color_text ;

    ui_ColorLoad( &color_text , guiState->layout.style.colorFG );

    if( guiState->displayEnabled )
    {
        GuiCmd_Print( guiState , color_text , str );
        guiState->page.renderPage = true ;
    }
}

static bool GuiCmd_Link( GuiState_st* guiState )
{
    uint8_t linkNum ;
    bool    pageEnd = false ;

    linkNum = GuiCmd_LdValUI( guiState );

    if( guiState->layout.list.displayingList )
    {
        if( guiState->inEventArea )
        {
            if( guiState->layout.list.index == guiState->layout.list.selection )
            {
                guiState->layout.link.pageNum = linkNum ;
            }
        }
    }

    return pageEnd ;
}

static bool GuiCmd_Title( GuiState_st* guiState )
{
    uint8_t* scriptPtr ;
    Color_st color_fg ;
    bool     pageEnd   = false ;

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
    Color_st color_text ;
    bool     pageEnd    = false ;

    ui_ColorLoad( &color_text , guiState->layout.style.colorFG );

    scriptPtr = &guiState->page.ptr[ guiState->page.index ] ;

    if( guiState->displayEnabled )
    {
        GuiCmd_Print( guiState , color_text , (char*)scriptPtr );
        guiState->page.renderPage = true ;
    }

    GuiCmd_AdvToNextCmd( guiState );

    return pageEnd ;

}

static bool GuiCmd_ValueDisplay( GuiState_st* guiState )
{
    uint8_t varNum ;
    bool    pageEnd  = false ;

    varNum = GuiCmd_LdValUI( guiState );

    if( varNum >= GUI_VAL_NUM_OF )
    {
        varNum = GUI_VAL_STUBBED ;
    }

    if( !guiState->update )
    {
        guiState->layout.vars[ guiState->layout.varIndex ].varNum = varNum ;
        guiState->layout.varNumOf++ ;
    }

    if( guiState->displayEnabled )
    {
        GuiValDsp_DisplayValue( guiState );
        guiState->page.renderPage = true ;
    }

    guiState->layout.varIndex++ ;

    return pageEnd ;

}

static bool GuiCmd_ValueInput( GuiState_st* guiState )
{
    uint8_t varNum ;
    bool    pageEnd  = false ;

    varNum = GuiCmd_LdValUI( guiState );

    if( varNum >= GUI_VAL_NUM_OF )
    {
        varNum = GUI_VAL_STUBBED ;
    }

    if( !guiState->update )
    {
        guiState->pageHasEvents = true ;
        guiState->layout.vars[ guiState->layout.varIndex ].varNum = varNum ;
        guiState->layout.varNumOf++ ;
    }

    guiState->layout.varInputDisplay = true ;

    if( !guiState->update )
    {
        guiState->layout.varInputSelect = 0 ;
        GuiValDsp_DisplayValue( guiState );
        guiState->page.renderPage       = true ;
    }
    else
    {
        if( GuiValInp_InputValue( guiState ) )
        {
            GuiValDsp_DisplayValue( guiState );
            guiState->page.renderPage = true ;
        }
    }

    guiState->layout.varInputDisplay = false ;

    guiState->layout.varIndex++ ;

    return pageEnd ;

}

static bool GuiCmd_GoToPosX( GuiState_st* guiState )
{
    Pos_st pos     = guiState->layout.line.pos ;
    bool   pageEnd = false ;

    pos.x = (Pos_t)GuiCmd_LdValUI( guiState );

    guiState->layout.line.pos = pos ;

    return pageEnd ;

}

static bool GuiCmd_GoToPosY( GuiState_st* guiState )
{
    Pos_st pos     = guiState->layout.line.pos ;
    bool   pageEnd = false ;

    pos.y = (Pos_t)GuiCmd_LdValUI( guiState );

    guiState->layout.line.pos = pos ;

    return pageEnd ;

}

static bool GuiCmd_AddToPosX( GuiState_st* guiState )
{
    Pos_st pos     = guiState->layout.line.pos ;
    bool   pageEnd = false ;

    pos.x += (Pos_t)GuiCmd_LdValI( guiState );

    guiState->layout.line.pos = pos ;

    return pageEnd ;

}

static bool GuiCmd_AddToPosY( GuiState_st* guiState )
{
    Pos_st pos     = guiState->layout.line.pos ;
    bool   pageEnd = false ;

    pos.y += (Pos_t)GuiCmd_LdValI( guiState );

    guiState->layout.line.pos = pos ;

    return pageEnd ;

}

static bool GuiCmd_DrawGraphic( GuiState_st* guiState )
{
    uint8_t graphic = GuiCmd_LdValUI( guiState );
    bool    pageEnd = false ;

    if( graphic < GUI_CMD_GRAPHIC_NUM_OF )
    {
        if( guiState->displayEnabled )
        {
            pageEnd = ui_GuiGraphic_Table[ graphic ]( guiState );
            guiState->page.renderPage = true ;
        }
    }

    return pageEnd ;

}

static bool GuiCmd_DrawLine( GuiState_st* guiState )
{
    Pos_st   pos     = guiState->layout.line.pos ;
    Color_st color ;
    bool     pageEnd = false ;

    ui_ColorLoad( &color , guiState->layout.style.colorFG );

    pos.w = (Pos_t)GuiCmd_LdValI( guiState );
    pos.h = (Pos_t)GuiCmd_LdValI( guiState );

    guiState->layout.line.pos = gfx_drawLine( &pos , &color );

    return pageEnd ;

}

static bool GuiCmd_DrawRect( GuiState_st* guiState )
{
    Pos_st   pos      = guiState->layout.line.pos ;
    Color_st color ;
    bool     pageEnd  = false ;

    ui_ColorLoad( &color , guiState->layout.style.colorFG );

    pos.w = (Pos_t)GuiCmd_LdValI( guiState );
    pos.h = (Pos_t)GuiCmd_LdValI( guiState );

    gfx_drawRect( &pos , &color , false );

    return pageEnd ;

}

static bool GuiCmd_DrawRectFilled( GuiState_st* guiState )
{
    Pos_st   pos      = guiState->layout.line.pos ;
    Color_st color ;
    bool     pageEnd  = false ;

    ui_ColorLoad( &color , guiState->layout.style.colorFG );

    pos.w = (Pos_t)GuiCmd_LdValI( guiState );
    pos.h = (Pos_t)GuiCmd_LdValI( guiState );

    gfx_drawRect( &pos , &color , true );

    return pageEnd ;

}

static bool GuiCmd_DrawCircle( GuiState_st* guiState )
{
    Pos_st   pos     = guiState->layout.line.pos ;
    Color_st color ;
    bool     pageEnd = false ;

    ui_ColorLoad( &color , guiState->layout.style.colorFG );

    pos.w = 1 ;
    pos.h = (Pos_t)GuiCmd_LdValUI( guiState );

    gfx_drawCircle( &pos , &color );

    return pageEnd ;

}

static bool GuiCmd_Operation( GuiState_st* guiState )
{
    bool    pageEnd  = false ;
    uint8_t opr ;

    opr = (uint8_t)GuiCmd_LdValUI( guiState );

    switch( opr )
    {
        case GUI_CMD_OPR_PAGE_TREE_TOP :
        {
            guiState->page.level = 0 ;
            guiState->page.levelList[ guiState->page.level ] = guiState->page.num ;
            guiState->page.level++ ;
            break ;
        }
        case GUI_CMD_OPR_GOTO_SCREEN_LEFT :
        {
            guiState->layout.line.pos.x = 0 ;
            break ;
        }
        case GUI_CMD_OPR_GOTO_SCREEN_RIGHT :
        {
            guiState->layout.line.pos.x = SCREEN_WIDTH - 1 ;
            break ;
        }
        case GUI_CMD_OPR_GOTO_SCREEN_TOP :
        {
            guiState->layout.line.pos.y = 0 ;
            break ;
        }
        case GUI_CMD_OPR_GOTO_SCREEN_BASE :
        {
            guiState->layout.line.pos.y = SCREEN_HEIGHT - 1 ;
            break ;
        }
        case GUI_CMD_OPR_GOTO_LINE_TOP :
        {
            guiState->layout.line.pos.y  = guiState->layout.lines[ guiState->layout.lineIndex ].pos.y ;
            guiState->layout.line.pos.y -= guiState->layout.line.height ;
            guiState->layout.line.pos.y += 1 ;
            break ;
        }
        case GUI_CMD_OPR_GOTO_LINE_BASE :
        {
            guiState->layout.line.pos.y  = guiState->layout.lines[ guiState->layout.lineIndex ].pos.y ;
            break ;
        }
    }

    return pageEnd ;

}

#ifdef ENABLE_DEBUG_MSG
  #ifndef DISPLAY_DEBUG_MSG
static bool GuiCmd_DispDgbVal( GuiState_st* guiState )
{
static uint8_t  val = 0 ;
    char     valueBuffer[ MAX_ENTRY_LEN + 1 ] = "" ;
//    Line_st  line = guiState->layout.line ;
    Line_st  line = guiState->layout.lines[ GUI_LINE_3 ] ;
    Pos_st   pos  = line.pos ;
    Color_st color ;
    bool     pageEnd = false ;

//    val = GuiCmd_LdValUI( guiState );

    snprintf( valueBuffer , MAX_ENTRY_LEN , "[%X\n:%8.8llX\n:%8.8llX]" , val , guiState->timer.timeOut , guiState->timeStamp );
    val++ ;

    ui_ColorLoad( &color , guiState->layout.style.colorFG );

    line.pos.y = line.textY ;

    gfx_print( &line.pos                        ,
               guiState->layout.style.font.size ,
               guiState->layout.style.align     ,
               &color , valueBuffer               );

    return pageEnd ;

}

  #else // DISPLAY_DEBUG_MSG
static bool GuiCmd_SetDbgMsg( GuiState_st* guiState )
{
    (void)guiState ;
    uint8_t* scriptPtr ;
    bool     pageEnd   = false ;

    scriptPtr = &guiState->page.ptr[ guiState->page.index ] ;

    GuiVal_SetDebugMessage( (char*)scriptPtr );

    GuiCmd_AdvToNextCmd( guiState );

    return pageEnd ;

}
  #endif // DISPLAY_DEBUG_MSG
#endif // ENABLE_DEBUG_MSG

static bool GuiCmd_PageEnd( GuiState_st* guiState )
{
    (void)guiState ;
    bool pageEnd = true ;

    LineEnd( guiState );

    return pageEnd ;

}

static bool GuiCmd_Stubbed( GuiState_st* guiState )
{
    (void)guiState ;
    bool pageEnd = false ;

    printf( "Cmd Stubbed" );

    return pageEnd ;

}

static uint8_t GuiCmd_LdValUI( GuiState_st* guiState )
{
    uint8_t value ;

    value = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->page.index++ ;

    return value ;
}

static int8_t GuiCmd_LdValI( GuiState_st* guiState )
{
    uint8_t value ;

    value = LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    guiState->page.index++ ;

    if( value & GUI_CMD_PARA_SIGNED_FLAG )
    {
        value |= GUI_CMD_PARA_SIGNED_BIT ;
    }

    return (int8_t)value ;
}

static uint16_t GuiCmd_LdValUIWord( GuiState_st* guiState )
{
    uint16_t val ;
    uint16_t value ;

    val    = (uint16_t)LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    value  = val << GUI_CMD_PARA_VALUE_SHIFT_0 ;
    guiState->page.index++ ;
    val    = (uint16_t)LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    value |= val << GUI_CMD_PARA_VALUE_SHIFT_1 ;
    guiState->page.index++ ;
    val    = (uint16_t)LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    value |= val << GUI_CMD_PARA_VALUE_SHIFT_2 ;
    guiState->page.index++ ;

    return value ;
}

static uint32_t GuiCmd_LdValUILong( GuiState_st* guiState )
{
    uint32_t val ;
    uint32_t value ;

    val    = (uint32_t)LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    value  = val << GUI_CMD_PARA_VALUE_SHIFT_0 ;
    guiState->page.index++ ;
    val    = (uint32_t)LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    value |= val << GUI_CMD_PARA_VALUE_SHIFT_1 ;
    guiState->page.index++ ;
    val    = (uint32_t)LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    value |= val << GUI_CMD_PARA_VALUE_SHIFT_2 ;
    guiState->page.index++ ;
    val    = (uint32_t)LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    value |= val << GUI_CMD_PARA_VALUE_SHIFT_3 ;
    guiState->page.index++ ;
    val    = (uint32_t)LD_VAL( guiState->page.ptr[ guiState->page.index ] );
    value |= val << GUI_CMD_PARA_VALUE_SHIFT_4 ;
    guiState->page.index++ ;

    return value ;
}

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
    Pos_st pos = guiState->layout.line.pos ;

    pos.y = guiState->layout.line.textY ;

    guiState->layout.itemPos  = gfx_print( &pos                             ,
                                           guiState->layout.style.font.size ,
                                           guiState->layout.style.align     ,
                                           &color , scriptPtr                 );
    guiState->page.renderPage = true ;

}
//@@@Kl conditionally compile out
void DebugMsg_PrintStr( GuiState_st* guiState , char* scriptPtr )
{
    Pos_st   pos ;
    Color_st color_fg ;

    pos                   = guiState->layout.line.pos ;

    ui_ColorLoad( &color_fg , guiState->layout.style.colorFG );

    pos.y = guiState->layout.line.textY ;

    guiState->layout.itemPos = gfx_print( &pos                             ,
                                          guiState->layout.style.font.size ,
                                          guiState->layout.style.align     ,
                                          &color_fg , scriptPtr );

    guiState->layout.line.pos.x += guiState->layout.itemPos.w ;
    guiState->page.renderPage    = true ;

}

void DebugMsg_PrintLine( GuiState_st* guiState , char* scriptPtr , uint8_t lineNum )
{
    Line_st  line     = guiState->layout.line ;
    Pos_st   pos      = guiState->layout.line.pos ;
    Color_st color_fg ;

    ui_ColorLoad( &color_fg , guiState->layout.style.colorFG );

    guiState->layout.line = guiState->layout.lines[ lineNum ] ;

    BlankLine( guiState );

    pos.y = guiState->layout.line.textY ;

    guiState->layout.itemPos  = gfx_print( &pos                             ,
                                           guiState->layout.style.font.size ,
                                           GFX_ALIGN_LEFT                   ,
//                                           guiState->layout.style.align     ,
                                           &color_fg , scriptPtr );
    guiState->layout.line     = line ;
    guiState->page.renderPage = true ;

}

static void BlankLine( GuiState_st* guiState )
{
    Pos_st   pos      = guiState->layout.line.pos ;
    Color_st color_bg ;

    pos.w = SCREEN_WIDTH ;
    pos.h = guiState->layout.line.height ;

    ui_ColorLoad( &color_bg , guiState->layout.style.colorBG );

    gfx_drawRect( &pos , &color_bg , true );

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

void ui_RenderDisplay( GuiState_st* guiState )
{
    guiState->page.renderPage = true ;
}
