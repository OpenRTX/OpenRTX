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
 *        160x128 display (MD380)            Recommended font size
 *      ┌─────────────────────────┐
 *      │  top_status_bar (16px)  │  8 pt (11 px) font with 2 px vertical padding
 *      │      top_pad (4px)      │  4 px padding
 *      │      Line 1 (20px)      │  8 pt (11 px) font with 4 px vertical padding
 *      │      Line 2 (20px)      │  8 pt (11 px) font with 4 px vertical padding
 *      │                         │
 *      │      Line 3 (40px)      │  16 pt (xx px) font with 6 px vertical padding
 *      │ RSSI+squelch bar (20px) │  20 px
 *      │      bottom_pad (4px)   │  4 px padding
 *      └─────────────────────────┘
 *
 *         128x64 display (GD-77)
 *      ┌─────────────────────────┐
 *      │  top_status_bar (11 px) │  6 pt (9 px) font with 1 px vertical padding
 *      │      top_pad (1px)      │  1 px padding
 *      │      Line 1 (10px)      │  6 pt (9 px) font without vertical padding
 *      │      Line 2 (10px)      │  6 pt (9 px) font with 2 px vertical padding
 *      │      Line 3 (18px)      │  12 pt (xx px) font with 0 px vertical padding
 *      │ RSSI+squelch bar (11px) │  11 px
 *      │      bottom_pad (1px)   │  1 px padding
 *      └─────────────────────────┘
 *
 *         128x48 display (RD-5R)
 *      ┌─────────────────────────┐
 *      │  top_status_bar (11 px) │  6 pt (9 px) font with 1 px vertical padding
 *      ├─────────────────────────┤  1 px line
 *      │      Line 2 (10px)      │  8 pt (11 px) font with 4 px vertical padding
 *      │      Line 3 (18px)      │  8 pt (11 px) font with 4 px vertical padding
 *      └─────────────────────────┘
 */

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

extern long long getTick();

static void ui_InitUIState( UI_State_st* uiState );
static void ui_InitGuiState( GuiState_st* guiState );
static void ui_InitGuiStateEvent( Event_st* event );
static void ui_InitGuiStatePage( Page_st* page );
static void ui_InitGuiStateLayout( Layout_st* layout );
/* UI main screen functions, their implementation is in "ui_main.c" */
extern bool _ui_Draw_MacroMenu( GuiState_st* guiState );

extern const char* display_timer_values[];

//@@@KL change all strings over to use englishStrings in EnglishStrings.h

GuiState_st GuiState ;

enum
{
    // Horizontal line height
    SCREEN_HLINE_H       = 1 ,
    // Compensate for fonts printing below the start position
    SCREEN_TEXT_V_OFFSET = 1
};

// Calculate UI layout depending on vertical resolution
// Tytera MD380, MD-UV380
#if SCREEN_HEIGHT > 127
enum
{
    SCREEN_INITIAL_ALIGN       	  =  GFX_ALIGN_LEFT ,
    // Height and padding shown in diagram at beginning of file
    SCREEN_INITIAL_X              =  0 ,
    SCREEN_INITIAL_Y              =  0 ,
    SCREEN_INITIAL_HEIGHT         = 20 ,
    SCREEN_TOP_ALIGN              = GFX_ALIGN_CENTER ,
    SCREEN_TOP_HEIGHT             = 16 ,
    SCREEN_TOP_PAD                =  4 ,
    SCREEN_LINE_ALIGN             =  GFX_ALIGN_LEFT ,
    SCREEN_LINE_1_HEIGHT          = 20 ,
    SCREEN_LINE_2_HEIGHT          = 20 ,
    SCREEN_LINE_3_HEIGHT          = 20 ,
    SCREEN_LINE_3_LARGE_HEIGHT    = 40 ,
    SCREEN_LINE_4_HEIGHT          = 20 ,
    SCREEN_LINE_5_HEIGHT          = 20 ,
    SCREEN_MENU_HEIGHT            = 16 ,
    SCREEN_BOTTOM_ALIGN           =  GFX_ALIGN_LEFT ,
    SCREEN_BOTTOM_HEIGHT          = 23 ,
    SCREEN_BOTTOM_PAD             = SCREEN_TOP_PAD ,
    SCREEN_STATUS_V_PAD           =  2 ,
    SCREEN_SMALL_LINE_V_PAD       =  2 ,
    SCREEN_BIG_LINE_V_PAD         =  6 ,
    SCREEN_HORIZONTAL_PAD         =  4 ,
    SCREEN_INITIAL_FONT_SIZE      = FONT_SIZE_8PT     ,
    SCREEN_INITIAL_SYMBOL_SIZE    = SYMBOLS_SIZE_8PT  ,
    SCREEN_INITIAL_COLOR_BG       = COLOR_BG ,
    SCREEN_INITIAL_COLOR_FG       = COLOR_FG ,

    // Top bar font: 8 pt
    SCREEN_TOP_FONT_SIZE          = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_TOP_SYMBOL_SIZE        = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    // Text line font: 8 pt
    SCREEN_LINE_1_FONT_SIZE       = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_LINE_1_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    SCREEN_LINE_2_FONT_SIZE       = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_LINE_2_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    SCREEN_LINE_3_FONT_SIZE       = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_LINE_3_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    SCREEN_LINE_4_FONT_SIZE       = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_LINE_4_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    SCREEN_LINE_5_FONT_SIZE       = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_LINE_5_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    // Frequency line font: 16 pt
    SCREEN_LINE_3_LARGE_FONT_SIZE = FONT_SIZE_16PT    , // FontSize_t
    // Bottom bar font: 8 pt
    SCREEN_BOTTOM_FONT_SIZE       = FONT_SIZE_8PT     , // FontSize_t
    SCREEN_BOTTOM_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    // TimeDate/Frequency input font
    SCREEN_INPUT_FONT_SIZE        = FONT_SIZE_12PT    , // FontSize_t
    // Menu font
    SCREEN_MENU_FONT_SIZE         = FONT_SIZE_8PT     , // FontSize_t
    // Mode screen frequency font: 12 pt
    SCREEN_MODE_FONT_SIZE_BIG     = FONT_SIZE_12PT    , // FontSize_t
    // Mode screen details font: 9 pt
    SCREEN_MODE_FONT_SIZE_SMALL   = FONT_SIZE_9PT     , // FontSize_t

    COLOR_GG_BG                   = COLOR_GG                ,
    COLOR_GG_FG                   = SCREEN_INITIAL_COLOR_FG ,

    COLOR_AGG_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_AGG_FG                  = COLOR_AGG               ,

    COLOR_HL_BG                   = COLOR_HL                ,
    COLOR_HL_FG                   = SCREEN_INITIAL_COLOR_FG ,

    COLOR_OP0_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP0_FG                  = COLOR_OP0               ,

    COLOR_OP1_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP1_FG                  = COLOR_OP1               ,

    COLOR_OP2_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP2_FG                  = COLOR_OP2               ,

    COLOR_OP3_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP3_FG                  = COLOR_OP3               ,

    COLOR_AL_BG                   = SCREEN_INITIAL_COLOR_BG ,
    COLOR_AL_FG                   = COLOR_AL

};

// Radioddity GD-77
#elif SCREEN_HEIGHT > 63
enum
{
    SCREEN_INITIAL_ALIGN          = GFX_ALIGN_LEFT ,
    // Height and padding shown in diagram at beginning of file
    SCREEN_INITIAL_X              =  0 ,
    SCREEN_INITIAL_Y              =  0 ,
    SCREEN_INITIAL_HEIGHT         = 10 ,
    SCREEN_TOP_ALIGN              = GFX_ALIGN_CENTER ,
    SCREEN_TOP_HEIGHT             = 11 ,
    SCREEN_TOP_PAD                =  1 ,
    SCREEN_LINE_ALIGN             = GFX_ALIGN_LEFT ,
    SCREEN_LINE_1_HEIGHT          = 10 ,
    SCREEN_LINE_2_HEIGHT          = 10 ,
    SCREEN_LINE_3_HEIGHT          = 10 ,
    SCREEN_LINE_3_LARGE_HEIGHT    = 16 ,
    SCREEN_LINE_4_HEIGHT          = 10 ,
    SCREEN_MENU_HEIGHT            = 10 ,
    SCREEN_BOTTOM_ALIGN           = GFX_ALIGN_LEFT ,
    SCREEN_BOTTOM_HEIGHT          = 15 ,
    SCREEN_BOTTOM_PAD             =  0 ,
    SCREEN_STATUS_V_PAD           =  1 ,
    SCREEN_SMALL_LINE_V_PAD       =  1 ,
    SCREEN_BIG_LINE_V_PAD         =  0 ,
    SCREEN_HORIZONTAL_PAD         =  4 ,
    SCREEN_INITIAL_FONT_SIZE      = FONT_SIZE_6PT     ,
    SCREEN_INITIAL_SYMBOL_SIZE    = SYMBOLS_SIZE_6PT  ,
    // Top bar font: 6 pt
    SCREEN_TOP_FONT_SIZE          = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_TOP_SYMBOL_SIZE        = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    // Middle line fonts: 5, 8, 8 pt
    SCREEN_LINE_1_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_1_SYMBOL_SIZE     = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    SCREEN_LINE_2_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_2_SYMBOL_SIZE     = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    SCREEN_LINE_3_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_3_SYMBOL_SIZE     = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    SCREEN_LINE_3_LARGE_FONT_SIZE = FONT_SIZE_10PT    , // FontSize_t
    SCREEN_LINE_4_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_4_SYMBOL_SIZE     = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    // Bottom bar font: 6 pt
    SCREEN_BOTTOM_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_BOTTOM_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t
    // TimeDate/Frequency input font
    SCREEN_INPUT_FONT_SIZE        = FONT_SIZE_8PT     , // FontSize_t
    // Menu font
    SCREEN_MENU_FONT_SIZE         = FONT_SIZE_6PT     , // FontSize_t
    // Mode screen frequency font: 9 pt
    SCREEN_MODE_FONT_SIZE_BIG     = FONT_SIZE_9PT     , // FontSize_t
    // Mode screen details font: 6 pt
    SCREEN_MODE_FONT_SIZE_SMALL   = FONT_SIZE_6PT     , // FontSize_t

    COLOR_GG_BG                   = COLOR_GG                ,
    COLOR_GG_FG                   = SCREEN_INITIAL_COLOR_FG ,

    COLOR_AGG_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_AGG_FG                  = COLOR_AGG               ,

    COLOR_HL_BG                   = COLOR_HL                ,
    COLOR_HL_FG                   = SCREEN_INITIAL_COLOR_FG ,

    COLOR_OP0_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP0_FG                  = COLOR_OP0               ,

    COLOR_OP1_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP1_FG                  = COLOR_OP1               ,

    COLOR_OP2_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP2_FG                  = COLOR_OP2               ,

    COLOR_OP3_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP3_FG                  = COLOR_OP3               ,

    COLOR_AL_BG                   = SCREEN_INITIAL_COLOR_BG ,
    COLOR_AL_FG                   = COLOR_AL

};

// Radioddity RD-5R
#elif SCREEN_HEIGHT > 47
enum
{
    SCREEN_INITIAL_ALIGN          = GFX_ALIGN_LEFT ,
    // Height and padding shown in diagram at beginning of file
    SCREEN_INITIAL_HEIGHT         = 10 ,
    // Height and padding shown in diagram at beginning of file
    SCREEN_TOP_ALIGN              = GFX_ALIGN_CENTER ,
    SCREEN_TOP_HEIGHT             = 11 ,
    SCREEN_TOP_PAD                =  1 ,
    SCREEN_LINE_ALIGN             = GFX_ALIGN_LEFT ,
    SCREEN_LINE_1_HEIGHT          =  0 ,
    SCREEN_LINE_2_HEIGHT          = 10 ,
    SCREEN_LINE_3_HEIGHT          = 10 ,
    SCREEN_LINE_3_LARGE_HEIGHT    = 18 ,
    SCREEN_LINE_4_HEIGHT          = 10 ,
    SCREEN_MENU_HEIGHT            = 10 ,
    SCREEN_BOTTOM_ALIGN           = GFX_ALIGN_LEFT ,
    SCREEN_BOTTOM_HEIGHT          =  0 ,
    SCREEN_BOTTOM_PAD             =  0 ,
    SCREEN_STATUS_V_PAD           =  1 ,
    SCREEN_SMALL_LINE_V_PAD       =  1 ,
    SCREEN_BIG_LINE_V_PAD         =  0 ,
    SCREEN_HORIZONTAL_PAD         =  4 ,
    SCREEN_INITIAL_FONT_SIZE      = FONT_SIZE_6PT     ,
    SCREEN_INITIAL_SYMBOL_SIZE    = SYMBOLS_SIZE_6PT  ,
    // Top bar font: 6 pt
    SCREEN_TOP_FONT_SIZE          = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_TOP_SYMBOL_SIZE        = SYMBOLS_SIZE_6PT  , // SymbolSize_t
    // Middle line fonts: 16, 16
    SCREEN_LINE_2_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_3_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_4_FONT_SIZE       = FONT_SIZE_6PT     , // FontSize_t
    SCREEN_LINE_3_LARGE_FONT_SIZE = FONT_SIZE_12PT    , // FontSize_t
    // TimeDate/Frequency input font
    SCREEN_INPUT_FONT_SIZE        = FONT_SIZE_8PT     , // FontSize_t
    // Menu font
    SCREEN_MENU_FONT_SIZE         = FONT_SIZE_6PT     , // FontSize_t
    // Mode screen frequency font: 9 pt
    SCREEN_MODE_FONT_SIZE_BIG     = FONT_SIZE_9PT     , // FontSize_t
    // Mode screen details font: 6 pt
    SCREEN_MODE_FONT_SIZE_SMALL   = FONT_SIZE_6PT     , // FontSize_t
    // Not present on this resolution
    SCREEN_LINE_1_FONT_SIZE       =  0                , // FontSize_t
    SCREEN_BOTTOM_FONT_SIZE       =  0                , // FontSize_t
    SCREEN_BOTTOM_SYMBOL_SIZE     = SYMBOLS_SIZE_8PT  , // SymbolSize_t

    COLOR_GG_BG                   = COLOR_GG                ,
    COLOR_GG_FG                   = SCREEN_INITIAL_COLOR_FG ,

    COLOR_AGG_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_AGG_FG                  = COLOR_AGG               ,

    COLOR_HL_BG                   = COLOR_HL                ,
    COLOR_HL_FG                   = SCREEN_INITIAL_COLOR_FG ,

    COLOR_OP0_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP0_FG                  = COLOR_OP0               ,

    COLOR_OP1_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP1_FG                  = COLOR_OP1               ,

    COLOR_OP2_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP2_FG                  = COLOR_OP2               ,

    COLOR_OP3_BG                  = SCREEN_INITIAL_COLOR_BG ,
    COLOR_OP3_FG                  = COLOR_OP3               ,

    COLOR_AL_BG                   = SCREEN_INITIAL_COLOR_BG ,
    COLOR_AL_FG                   = COLOR_AL

};
#else
    #error Unsupported vertical resolution!
#endif
void ui_init( void )
{
    GuiState_st* guiState = &GuiState ;

    ui_InitUIState( &guiState->uiState );
    ui_InitGuiState( guiState );

    last_event_tick = getTick();
    redraw_needed   = true ;
}

static void ui_InitUIState( UI_State_st* uiState )
{
    uint16_t size ;
    uint16_t index ;

    size = sizeof( UI_State_st );

    for( index = 0 ; index < size ; index++ )
    {
        ((uint8_t*)uiState)[ index ] = 0 ;
    }

}

static void ui_InitGuiState( GuiState_st* guiState )
{
    ui_InitGuiStateEvent( &guiState->event );
    guiState->update                = false ;
    guiState->pageHasEvents         = false ;
    guiState->inEventArea           = false ;
    guiState->displayEnabledInitial = false ;
    guiState->displayEnabled        = false ;
    guiState->timeStamp             = 0 ;
    guiState->timer.timeOut         = 0 ;
    guiState->timer.scriptPageNum   = PAGE_STUBBED ;
    guiState->sync_rtx              = false ;
    guiState->handled               = false ;
    ui_InitGuiStatePage( &guiState->page );
    ui_InitGuiStateLayout( &guiState->layout );
}

static void ui_InitGuiStateEvent( Event_st* event )
{
    event->type    = EVENT_TYPE_NONE ;
    event->payload = 0 ;
}

static void ui_InitGuiStatePage( Page_st* page )
{
    uint16_t index ;

    page->num        = PAGE_INITIAL ;

    for( index = 0 ; index < MAX_PAGE_DEPTH ; index++ )
    {
        page->levelList[ index ] = PAGE_INITIAL ;
    }

    page->level      = 0 ;
    page->ptr        = (uint8_t*)uiPageTable[ 0 ] ;
    page->index      = 0 ;
    page->cmdIndex   = 0 ;
    page->renderPage = false ;
}

static void ui_InitGuiStateLayout( Layout_st* layout )
{
    layout->hline_h                                = SCREEN_HLINE_H ;
    layout->menu_h                                 = SCREEN_MENU_HEIGHT ;
    layout->bottom_pad                             = SCREEN_BOTTOM_PAD ;
    layout->status_v_pad                           = SCREEN_STATUS_V_PAD ;
    layout->horizontal_pad                         = SCREEN_HORIZONTAL_PAD ;
    layout->text_v_offset                          = SCREEN_TEXT_V_OFFSET ;

    layout->line.pos.x                             = SCREEN_INITIAL_X ;
    layout->line.pos.y                             = SCREEN_INITIAL_Y ;
    layout->line.height                            = SCREEN_INITIAL_HEIGHT ;
    layout->style.align                            = SCREEN_INITIAL_ALIGN ;
    layout->style.font.size                        = SCREEN_INITIAL_FONT_SIZE ;
    layout->style.symbolSize                       = SCREEN_INITIAL_SYMBOL_SIZE ;
    layout->style.colorBG                          = SCREEN_INITIAL_COLOR_BG ;
    layout->style.colorFG                          = SCREEN_INITIAL_COLOR_FG ;

    layout->lines[ GUI_LINE_TOP ].pos.x            = SCREEN_HORIZONTAL_PAD ;
    layout->lines[ GUI_LINE_TOP ].pos.y            = SCREEN_TOP_HEIGHT - SCREEN_STATUS_V_PAD - SCREEN_TEXT_V_OFFSET ;
    layout->lines[ GUI_LINE_TOP ].height           = SCREEN_TOP_HEIGHT ;
    layout->styles[ GUI_STYLE_TOP ].align          = SCREEN_TOP_ALIGN ;
    layout->styles[ GUI_STYLE_TOP ].font.size      = SCREEN_TOP_FONT_SIZE ;
    layout->styles[ GUI_STYLE_TOP ].symbolSize     = SCREEN_TOP_SYMBOL_SIZE ;
    layout->styles[ GUI_STYLE_TOP ].colorBG        = SCREEN_INITIAL_COLOR_BG ;
    layout->styles[ GUI_STYLE_TOP ].colorFG        = SCREEN_INITIAL_COLOR_FG ;

    layout->lines[ GUI_LINE_1 ].pos.x              = SCREEN_HORIZONTAL_PAD ;
    layout->lines[ GUI_LINE_1 ].pos.y              = layout->lines[ GUI_LINE_TOP ].pos.y + SCREEN_TOP_PAD + SCREEN_LINE_1_HEIGHT ;
    layout->lines[ GUI_LINE_1 ].height             = SCREEN_LINE_1_HEIGHT ;
    layout->styles[ GUI_STYLE_1 ].align            = SCREEN_LINE_ALIGN ;
    layout->styles[ GUI_STYLE_1 ].font.size        = SCREEN_LINE_1_FONT_SIZE ;
    layout->styles[ GUI_STYLE_1 ].symbolSize       = SCREEN_LINE_1_SYMBOL_SIZE ;
    layout->styles[ GUI_STYLE_1 ].colorBG          = SCREEN_INITIAL_COLOR_BG ;
    layout->styles[ GUI_STYLE_1 ].colorFG          = SCREEN_INITIAL_COLOR_FG ;

    layout->lines[ GUI_LINE_2 ].pos.x              = SCREEN_HORIZONTAL_PAD ;
    layout->lines[ GUI_LINE_2 ].pos.y              = layout->lines[ GUI_LINE_1 ].pos.y + SCREEN_LINE_2_HEIGHT ;
    layout->lines[ GUI_LINE_2 ].height             = SCREEN_LINE_2_HEIGHT ;
    layout->styles[ GUI_STYLE_2 ].align            = SCREEN_LINE_ALIGN ;
    layout->styles[ GUI_STYLE_2 ].font.size        = SCREEN_LINE_2_FONT_SIZE ;
    layout->styles[ GUI_STYLE_2 ].symbolSize       = SCREEN_LINE_2_SYMBOL_SIZE ;
    layout->styles[ GUI_STYLE_2 ].colorBG          = SCREEN_INITIAL_COLOR_BG ;
    layout->styles[ GUI_STYLE_2 ].colorFG          = SCREEN_INITIAL_COLOR_FG ;

    layout->lines[ GUI_LINE_3 ].pos.x              = SCREEN_HORIZONTAL_PAD ;
    layout->lines[ GUI_LINE_3 ].pos.y              = layout->lines[ GUI_LINE_2 ].pos.y + SCREEN_LINE_3_HEIGHT ;
    layout->lines[ GUI_LINE_3 ].height             = SCREEN_LINE_3_HEIGHT ;
    layout->styles[ GUI_STYLE_3 ].align            = SCREEN_LINE_ALIGN ;
    layout->styles[ GUI_STYLE_3 ].font.size        = SCREEN_LINE_3_FONT_SIZE ;
    layout->styles[ GUI_STYLE_3 ].symbolSize       = SCREEN_LINE_3_SYMBOL_SIZE ;
    layout->styles[ GUI_STYLE_3 ].colorBG          = SCREEN_INITIAL_COLOR_BG ;
    layout->styles[ GUI_STYLE_3 ].colorFG          = SCREEN_INITIAL_COLOR_FG ;

    layout->lines[ GUI_LINE_3_LARGE ].pos.x        = SCREEN_HORIZONTAL_PAD ;
    layout->lines[ GUI_LINE_3_LARGE ].pos.y        = layout->lines[ GUI_LINE_2 ].pos.y + SCREEN_LINE_3_LARGE_HEIGHT ;
    layout->lines[ GUI_LINE_3_LARGE ].height       = SCREEN_LINE_3_LARGE_HEIGHT ;
    layout->styles[ GUI_STYLE_3_LARGE ].align      = SCREEN_LINE_ALIGN ;
    layout->styles[ GUI_STYLE_3_LARGE ].font.size  = SCREEN_LINE_3_LARGE_FONT_SIZE ;
    layout->styles[ GUI_STYLE_3_LARGE ].symbolSize = SCREEN_LINE_3_SYMBOL_SIZE ;
    layout->styles[ GUI_STYLE_3_LARGE ].colorBG    = SCREEN_INITIAL_COLOR_BG ;
    layout->styles[ GUI_STYLE_3_LARGE ].colorFG    = SCREEN_INITIAL_COLOR_FG ;

    layout->lines[ GUI_LINE_4 ].pos.x              = SCREEN_HORIZONTAL_PAD ;
    layout->lines[ GUI_LINE_4 ].pos.y              = layout->lines[ GUI_LINE_3 ].pos.y + SCREEN_LINE_4_HEIGHT ;
    layout->lines[ GUI_LINE_4 ].height             = SCREEN_LINE_4_HEIGHT ;
    layout->styles[ GUI_STYLE_4 ].align            = SCREEN_LINE_ALIGN ;
    layout->styles[ GUI_STYLE_4 ].font.size        = SCREEN_LINE_4_FONT_SIZE ;
    layout->styles[ GUI_STYLE_4 ].symbolSize       = SCREEN_LINE_4_SYMBOL_SIZE ;
    layout->styles[ GUI_STYLE_4 ].colorBG          = SCREEN_INITIAL_COLOR_BG ;
    layout->styles[ GUI_STYLE_4 ].colorFG          = SCREEN_INITIAL_COLOR_FG ;

    layout->lines[ GUI_LINE_5 ].pos.x              = SCREEN_HORIZONTAL_PAD ;
    layout->lines[ GUI_LINE_5 ].pos.y              = layout->lines[ GUI_LINE_4 ].pos.y + SCREEN_LINE_5_HEIGHT ;
    layout->lines[ GUI_LINE_5 ].height             = SCREEN_LINE_5_HEIGHT ;
    layout->styles[ GUI_STYLE_5 ].align            = SCREEN_LINE_ALIGN ;
    layout->styles[ GUI_STYLE_5 ].font.size        = SCREEN_LINE_5_FONT_SIZE ;
    layout->styles[ GUI_STYLE_5 ].symbolSize       = SCREEN_LINE_5_SYMBOL_SIZE ;
    layout->styles[ GUI_STYLE_5 ].colorBG          = SCREEN_INITIAL_COLOR_BG ;
    layout->styles[ GUI_STYLE_5 ].colorFG          = SCREEN_INITIAL_COLOR_FG ;

    layout->lines[ GUI_LINE_BOTTOM ].pos.x         = SCREEN_HORIZONTAL_PAD ;
    layout->lines[ GUI_LINE_BOTTOM ].pos.y         = SCREEN_HEIGHT - SCREEN_BOTTOM_PAD - SCREEN_STATUS_V_PAD - SCREEN_TEXT_V_OFFSET ;
    layout->lines[ GUI_LINE_BOTTOM ].height        = SCREEN_BOTTOM_HEIGHT ;
    layout->styles[ GUI_STYLE_BOTTOM ].align       = SCREEN_LINE_ALIGN ;
    layout->styles[ GUI_STYLE_BOTTOM ].font.size   = SCREEN_BOTTOM_FONT_SIZE ;
    layout->styles[ GUI_STYLE_BOTTOM ].symbolSize  = SCREEN_BOTTOM_SYMBOL_SIZE ;
    layout->styles[ GUI_STYLE_BOTTOM ].colorBG     = SCREEN_INITIAL_COLOR_BG ;
    layout->styles[ GUI_STYLE_BOTTOM ].colorFG     = SCREEN_INITIAL_COLOR_FG ;

    layout->styles[ GUI_STYLE_GG ].colorBG         = COLOR_GG_BG ;
    layout->styles[ GUI_STYLE_GG ].colorFG         = COLOR_GG_FG ;

    layout->styles[ GUI_STYLE_AGG ].colorBG        = COLOR_AGG_BG ;
    layout->styles[ GUI_STYLE_AGG ].colorFG        = COLOR_AGG_FG ;

    layout->styles[ GUI_STYLE_HL ].colorBG         = COLOR_HL_BG ;
    layout->styles[ GUI_STYLE_HL ].colorFG         = COLOR_HL_FG ;

    layout->styles[ GUI_STYLE_OP0 ].colorBG        = COLOR_OP0_BG ;
    layout->styles[ GUI_STYLE_OP0 ].colorFG        = COLOR_OP0_FG ;

    layout->styles[ GUI_STYLE_OP1 ].colorBG        = COLOR_OP1_BG ;
    layout->styles[ GUI_STYLE_OP1 ].colorFG        = COLOR_OP1_FG ;

    layout->styles[ GUI_STYLE_OP2 ].colorBG        = COLOR_OP2_BG ;
    layout->styles[ GUI_STYLE_OP2 ].colorFG        = COLOR_OP2_FG ;

    layout->styles[ GUI_STYLE_OP3 ].colorBG        = COLOR_OP3_BG ;
    layout->styles[ GUI_STYLE_OP3 ].colorFG        = COLOR_OP3_FG ;

    layout->styles[ GUI_STYLE_AL ].colorBG         = COLOR_AL_BG ;
    layout->styles[ GUI_STYLE_AL ].colorFG         = COLOR_AL_FG ;

	layout->lineIndex							   = 0 ;

	layout->itemPos.x       					   = 0 ;
	layout->itemPos.y       					   = 0 ;
	layout->itemPos.w       					   = 0 ;
	layout->itemPos.h       					   = 0 ;

    layout->input_font.size                        = SCREEN_INPUT_FONT_SIZE ;
    layout->menu_font.size                         = SCREEN_MENU_FONT_SIZE ;
    layout->mode_font_big.size                     = SCREEN_MODE_FONT_SIZE_BIG ;
    layout->mode_font_small.size                   = SCREEN_MODE_FONT_SIZE_SMALL ;

    layout->inSelect                               = false ;

    ui_InitGuiStateLayoutLinks( layout );
    ui_InitGuiStateLayoutVars( layout );

}

void ui_InitGuiStateLayoutLinks( Layout_st* layout )
{
    uint8_t index ;

    for( index = 0 ; index < LINK_MAX_NUM_OF ; index++ )
    {
        layout->links[ index ].type = LINK_TYPE_NONE ;
        layout->links[ index ].num  = 0 ;
        layout->links[ index ].amt  = 0 ;
    }
    layout->linkNumOf = 0 ;
    layout->linkIndex = 0 ;

}

void ui_InitGuiStateLayoutVars( Layout_st* layout )
{
    uint8_t index ;

    for( index = 0 ; index < VAR_MAX_NUM_OF ; index++ )
    {
        layout->vars[ index ].varNum = GUI_VAL_DSP_STUBBED ;
        layout->vars[ index ].pos.y  =  0 ;
        layout->vars[ index ].pos.x  =  0 ;
        layout->vars[ index ].pos.h  =  0 ;
        layout->vars[ index ].pos.w  =  0 ;
        layout->vars[ index ].value  = ~0 ;
    }
    layout->varNumOf = 0 ;
    layout->varIndex = 0 ;
}

void ui_Draw_SplashScreen( void )
{
    Pos_st   logo_pos ;
    Pos_st   call_pos ;
    Font_st  logo_font ;
    Font_st  call_font ;
    Color_st color_fg ;
    Color_st color_op3 ;

    ui_ColorLoad( &color_fg , COLOR_FG );
    ui_ColorLoad( &color_op3 , COLOR_OP3 );

    gfx_clearScreen();

    #if SCREEN_HEIGHT > 64
    logo_pos.x     = 0 ;
    logo_pos.y     = ( SCREEN_HEIGHT / 2 ) ;
    call_pos.x     = 0 ;
    call_pos.y     = SCREEN_HEIGHT - 8 ;
    logo_font.size = FONT_SIZE_12PT ;
    call_font.size = FONT_SIZE_8PT ;
    #else
    logo_pos.x     = 0 ;
    logo_pos.y     = 19 ;
    call_pos.x     = 0 ;
    call_pos.y     = SCREEN_HEIGHT - 8 ;
    logo_font.size = FONT_SIZE_8PT ;
    call_font.size = FONT_SIZE_6PT ;
    #endif

    gfx_print( &logo_pos , logo_font.size , GFX_ALIGN_CENTER , &color_op3 , "O P N\nR T X" );
    gfx_print( &call_pos , call_font.size , GFX_ALIGN_CENTER , &color_fg  , state.settings.callsign );

    vp_announceSplashScreen();
}

void ui_saveState( void )
{
    last_state = state ;
}
//@@@KL redraw_needed will need to be redacted \ name changed \ put into GuiState
bool ui_updateGUI( Event_st* event )
{
    ui_Draw_Page( &GuiState , event );

    return redraw_needed ;
}

bool ui_pushEvent( const uint8_t type , const uint32_t data )
{
    uint8_t  newHead = ( evQueue_wrPos + 1 ) % MAX_NUM_EVENTS ;
    Event_st event ;
    bool     result  = false ;

    // Queue is not full
    if( newHead != evQueue_rdPos )
    {
        // Preserve atomicity when writing the new element into the queue.
        event.type               = type ;
        event.payload            = data ;
        evQueue[ evQueue_wrPos ] = event ;
        evQueue_wrPos            = newHead ;
        result                   = true ;
    }

    return result ;
}

bool ui_popEvent( Event_st* event )
{
    bool eventPresent = false ;

    event->type    = EVENT_TYPE_NONE ;
    event->payload = 0 ;

    // Check for events
    if( evQueue_rdPos != evQueue_wrPos )
    {
        // Pop an event from the queue
        *event        = evQueue[ evQueue_rdPos ] ;
        evQueue_rdPos = ( evQueue_rdPos + 1 ) % MAX_NUM_EVENTS ;
        eventPresent  = true ;
    }

    return eventPresent ;

}

void ui_terminate( void )
{
}
