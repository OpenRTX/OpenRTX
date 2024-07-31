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

#ifndef UI_COMMANDS_H
#define UI_COMMANDS_H

#include <ui/ui_default.h>
#include "ui_scripts.h"

// System Parameter Values
enum
{
    GUI_CMD_PARA_VALUE_SHIFT   = 7 ,
    // Short
    GUI_CMD_PARA_SIGNED_BIT    = 0x01 << GUI_CMD_PARA_VALUE_SHIFT ,
    GUI_CMD_PARA_SIGNED_FLAG   = GUI_CMD_PARA_SIGNED_BIT >> 1 ,
    GUI_CMD_PARA_VALUE_MASK    = ~GUI_CMD_PARA_SIGNED_BIT ,
    // Long
    GUI_CMD_PARA_VALUE_SHIFT_0 = GUI_CMD_PARA_VALUE_SHIFT * 0 , //  0
    GUI_CMD_PARA_VALUE_SHIFT_1 = GUI_CMD_PARA_VALUE_SHIFT * 1 , //  7
    GUI_CMD_PARA_VALUE_SHIFT_2 = GUI_CMD_PARA_VALUE_SHIFT * 2 , // 14
    GUI_CMD_PARA_VALUE_SHIFT_3 = GUI_CMD_PARA_VALUE_SHIFT * 3 , // 21
    GUI_CMD_PARA_VALUE_SHIFT_4 = GUI_CMD_PARA_VALUE_SHIFT * 4   // 28
};

#define ST_VAL( val )       ( (uint8_t)( val & GUI_CMD_PARA_VALUE_MASK ) + \
                              (uint8_t)GUI_CMD_DATA_AREA )

#define LD_VAL( val )       ( (uint8_t)val - (uint8_t)GUI_CMD_DATA_AREA )

#define ST_VAL_WORD( val )  ST_VAL( (uint8_t)( (uint32_t)val >> GUI_CMD_PARA_VALUE_SHIFT_0 ) ) , \
                            ST_VAL( (uint8_t)( (uint32_t)val >> GUI_CMD_PARA_VALUE_SHIFT_1 ) ) , \
                            ST_VAL( (uint8_t)( (uint32_t)val >> GUI_CMD_PARA_VALUE_SHIFT_2 ) )

#define ST_VAL_LONG( val )  ST_VAL( (uint8_t)( (uint32_t)val >> GUI_CMD_PARA_VALUE_SHIFT_0 ) ) , \
                            ST_VAL( (uint8_t)( (uint32_t)val >> GUI_CMD_PARA_VALUE_SHIFT_1 ) ) , \
                            ST_VAL( (uint8_t)( (uint32_t)val >> GUI_CMD_PARA_VALUE_SHIFT_2 ) ) , \
                            ST_VAL( (uint8_t)( (uint32_t)val >> GUI_CMD_PARA_VALUE_SHIFT_3 ) ) , \
                            ST_VAL( (uint8_t)( (uint32_t)val >> GUI_CMD_PARA_VALUE_SHIFT_4 ) )

// GUI Commands
enum
{
    GUI_CMD_NULL             = 0x00 ,
    GUI_CMD_EVENT_START      = 0x01 , // dynamic display - variables updated
    GUI_CMD_EVENT_END        = 0x02 , //  used with page sections that respond to events
    GUI_CMD_ON_EVENT         = 0x03 , // undertakes action on event occurance (not on initial page display)
    GUI_CMD_TIMER_CHECK      = 0x04 ,
    GUI_CMD_TIMER_SET        = 0x05 ,
    GUI_CMD_GOTO_TEXT_LINE   = 0x06 ,
    GUI_CMD_LOAD_STYLE       = 0x07 ,
    GUI_CMD_BG_COLOR         = 0x08 ,
    GUI_CMD_FG_COLOR         = 0x09 ,
    GUI_CMD_LINE_END         = 0x0A ,
    GUI_CMD_FONT_SIZE        = 0x0B ,
    GUI_CMD_ALIGN            = 0x0C ,
    GUI_CMD_RUN_SCRIPT       = 0x0D ,
    GUI_CMD_LIST             = 0x0E ,
    GUI_CMD_LIST_ELEMENT     = 0x0F ,
    GUI_CMD_LINK             = 0x10 ,
    GUI_CMD_TITLE            = 0x11 ,
    GUI_CMD_TEXT             = 0x12 ,
    GUI_CMD_VALUE_DSP        = 0x13 ,
    GUI_CMD_VALUE_INP        = 0x14 ,
    GUI_CMD_GOTO_POS_X       = 0x15 ,
    GUI_CMD_GOTO_POS_Y       = 0x16 ,
    GUI_CMD_ADD_POS_X        = 0x17 ,
    GUI_CMD_ADD_POS_Y        = 0x18 ,
    GUI_CMD_DRAW_GRAPHIC     = 0x19 ,
    GUI_CMD_OPERATION        = 0x1A ,
#ifndef DISPLAY_DEBUG_MSG
    GUI_CMD_DBG_VAL          = 0x1D ,
#else // DISPLAY_DEBUG_MSG
    GUI_CMD_SET_DBG_MSG      = 0x1D ,
#endif // DISPLAY_DEBUG_MSG
    GUI_CMD_STUBBED          = 0x1E ,
    GUI_CMD_PAGE_END         = 0x1F ,
    GUI_CMD_NUM_OF                  ,
    GUI_CMD_DATA_AREA        = 0x20
};

/*
// Color Variable Field Unshifted Masks and Shifts
enum
{
    COLOR_ENC_RED_SHIFT   =   24 ,
    COLOR_ENC_RED_MASK    = 0xFF ,
    COLOR_ENC_GREEN_SHIFT =   16 ,
    COLOR_ENC_GREEN_MASK  = 0xFF ,
    COLOR_ENC_BLUE_SHIFT  =    8 ,
    COLOR_ENC_BLUE_MASK   = 0xFF ,
    COLOR_ENC_ALPHA_SHIFT =    0 ,
    COLOR_ENC_ALPHA_MASK  = 0xFF
};

// Color Variable Field Encoder
#define COLOR_DEF_ENC( r , g , b , a ) (const uint32_t)(                        \
    ( ( (const uint32_t)r & COLOR_ENC_RED_MASK   ) << COLOR_ENC_RED_SHIFT   ) | \
    ( ( (const uint32_t)g & COLOR_ENC_GREEN_MASK ) << COLOR_ENC_GREEN_SHIFT ) | \
    ( ( (const uint32_t)b & COLOR_ENC_BLUE_MASK  ) << COLOR_ENC_BLUE_SHIFT  ) | \
    ( ( (const uint32_t)a & COLOR_ENC_ALPHA_MASK ) << COLOR_ENC_ALPHA_SHIFT )   )

// Color Definitions
#define COLOR_DEF_WHITE         COLOR_DEF_ENC( 255 , 255 , 255 , 255 )
#define COLOR_DEF_BLACK         COLOR_DEF_ENC(   0 ,   0 ,  0  , 255 )
#define COLOR_DEF_GREY          COLOR_DEF_ENC(  60 ,  60 ,  60 , 255 )
#define COLOR_DEF_ALPHA_GREY    COLOR_DEF_ENC(   0 ,   0 ,   0 , 255 )
#define COLOR_DEF_YELLOW_FAB413 COLOR_DEF_ENC( 250 , 180 ,  19 , 255 )
#define COLOR_DEF_RED           COLOR_DEF_ENC( 255 ,   0 ,   0 , 255 )
#define COLOR_DEF_GREEN         COLOR_DEF_ENC(   0 , 255 ,   0 , 255 )
#define COLOR_DEF_BLUE          COLOR_DEF_ENC(   0 ,   0 , 255 , 255 )

// Color Table Definition
#define COLOR_TABLE             \
    COLOR_DEF_BLACK         ,   \
    COLOR_DEF_WHITE         ,   \
    COLOR_DEF_GREY          ,   \
    COLOR_DEF_ALPHA_GREY    ,   \
    COLOR_DEF_YELLOW_FAB413 ,   \
    COLOR_DEF_RED           ,   \
    COLOR_DEF_GREEN         ,   \
    COLOR_DEF_BLUE

// Color Selectors
typedef enum
{
    COLOR_BLACK         ,
    COLOR_WHITE         ,
    COLOR_GREY          ,
    COLOR_ALPHA_GREY    ,
    COLOR_YELLOW_FAB413 ,
    COLOR_RED           ,
    COLOR_GREEN         ,
    COLOR_BLUE          ,
    COLOR_NUM_OF        ,

    COLOR_BG  = COLOR_BLACK         , // background
    COLOR_FG  = COLOR_WHITE         , // foreground
    COLOR_GG  = COLOR_GREY          , // grey background
    COLOR_AGG = COLOR_ALPHA_GREY    , // alpha grey overlay foreground
    COLOR_HL  = COLOR_YELLOW_FAB413 , // highlight background
    COLOR_OP0 = COLOR_RED           , // option 0 foreground
    COLOR_OP1 = COLOR_GREEN         , // option 1 foreground
    COLOR_OP2 = COLOR_BLUE          , // option 2 foreground
    COLOR_OP3 = COLOR_YELLOW_FAB413 , // option 3 foreground
    COLOR_AL  = COLOR_RED             // alarm foreground

}ColorSelector_en;

// Load the Color_st structure with the color variable fields
#define COLOR_LD( c , cv )                                                       \
c->red   = (uint8_t)( ( cv >> COLOR_ENC_RED_SHIFT   ) & COLOR_ENC_RED_MASK   ) ; \
c->green = (uint8_t)( ( cv >> COLOR_ENC_GREEN_SHIFT ) & COLOR_ENC_GREEN_MASK ) ; \
c->blue  = (uint8_t)( ( cv >> COLOR_ENC_BLUE_SHIFT  ) & COLOR_ENC_BLUE_MASK  ) ; \
c->alpha = (uint8_t)( ( cv >> COLOR_ENC_ALPHA_SHIFT ) & COLOR_ENC_ALPHA_MASK ) ;
*/

/*******************************************************************************
*   Script Command Macros
*******************************************************************************/

#define EVENT_ENC( t , p )      ( ( (uint32_t)t << EVENT_TYPE_SHIFT ) | (uint32_t)p )
#define ST_EVENT( t , p )       ST_VAL_LONG( EVENT_ENC( t , p ) )

#define EVENT_START( t , p )    GUI_CMD_EVENT_START , ST_EVENT( t , p )
#define EVENT_END               GUI_CMD_EVENT_END

// GUI_CMD_ON_EVENT Actions
enum
{
    ON_EVENT_ACTION_GOTO_PAGE ,
    ON_EVENT_ACTION_GO_BACK
};

#define ON_EVENT( t , p , a , v )           GUI_CMD_ON_EVENT , ST_EVENT( t , p ) , ST_VAL( a ) , ST_VAL( v )

#define ON_EVENT_KEY_ENTER_GOTO_PAGE( v )   ON_EVENT( EVENT_TYPE_KBD , KEY_ENTER , ON_EVENT_ACTION_GOTO_PAGE , v )

#define ON_EVENT_KEY_ENTER_GO_BACK          ON_EVENT( EVENT_TYPE_KBD , KEY_ENTER , ON_EVENT_ACTION_GO_BACK , 0 )
#define ON_EVENT_KEY_ESC_GO_BACK            ON_EVENT( EVENT_TYPE_KBD , KEY_ESC   , ON_EVENT_ACTION_GO_BACK , 0 )

#define TIMER_CHECK( p )                    GUI_CMD_TIMER_CHECK , ST_VAL( p )
#define TIMER_SET( pg , period )            GUI_CMD_TIMER_SET , ST_VAL( pg ) , ST_VAL_WORD( period )

#define GOTO_TEXT_LINE( l ) GUI_CMD_GOTO_TEXT_LINE , ST_VAL( l )
#define LOAD_STYLE( l )     GUI_CMD_LOAD_STYLE , ST_VAL( l )

#define BG_COLOR( c )       GUI_CMD_BG_COLOR   , ST_VAL( COLOR_##c )
#define FG_COLOR( c )       GUI_CMD_FG_COLOR   , ST_VAL( COLOR_##c )

#define FONT_SIZE( s )      GUI_CMD_FONT_SIZE  , ST_VAL( s )

//#define ENABLE_ALIGN_VERTICAL

enum
{
    GUI_CMD_ALIGN_PARA_LEFT   = 0x01 ,
    GUI_CMD_ALIGN_PARA_CENTER = 0x02 ,
    GUI_CMD_ALIGN_PARA_RIGHT  = 0x03 ,
    GUI_CMD_ALIGN_PARA_MASK_X = 0x07
#ifdef ENABLE_ALIGN_VERTICAL
    ,
    GUI_CMD_ALIGN_PARA_TOP    = 0x01 << 3 ,
    GUI_CMD_ALIGN_PARA_MIDDLE = 0x02 << 3 ,
    GUI_CMD_ALIGN_PARA_BOTTOM = 0x03 << 3 ,
    GUI_CMD_ALIGN_PARA_MASK_Y = 0x07 << 3
#endif // ENABLE_ALIGN_VERTICAL
};

enum
{
    GUI_CMD_GRAPHIC_LINE        ,
    GUI_CMD_GRAPHIC_RECT        ,
    GUI_CMD_GRAPHIC_RECT_FILLED ,
    GUI_CMD_GRAPHIC_CIRCLE      ,
    GUI_CMD_GRAPHIC_NUM_OF
};

#define ALIGN_LEFT              GUI_CMD_ALIGN , ST_VAL( GUI_CMD_ALIGN_PARA_LEFT   )
#define ALIGN_CENTER            GUI_CMD_ALIGN , ST_VAL( GUI_CMD_ALIGN_PARA_CENTER )
#define ALIGN_RIGHT             GUI_CMD_ALIGN , ST_VAL( GUI_CMD_ALIGN_PARA_RIGHT  )
#ifdef ENABLE_ALIGN_VERTICAL
  #define ALIGN_TOP               GUI_CMD_ALIGN , ST_VAL( GUI_CMD_ALIGN_PARA_TOP    )
  #define ALIGN_MIDDLE            GUI_CMD_ALIGN , ST_VAL( GUI_CMD_ALIGN_PARA_MIDDLE )
  #define ALIGN_BOTTOM            GUI_CMD_ALIGN , ST_VAL( GUI_CMD_ALIGN_PARA_BOTTOM )
#endif // ENABLE_ALIGN_VERTICAL

#define RUN_SCRIPT( p )     GUI_CMD_RUN_SCRIPT , ST_VAL( p )

#define LINE_END            GUI_CMD_LINE_END

#define LIST( p , s , l )   GUI_CMD_LIST , ST_VAL( p ) , ST_VAL( s ) , ST_VAL( l )
#define LIST_ELEMENT        GUI_CMD_LIST_ELEMENT
#define LINK( p )           GUI_CMD_LINK , ST_VAL( p )

#define TITLE               GUI_CMD_TITLE
#define TEXT                GUI_CMD_TEXT
#define NULL_CH             GUI_CMD_NULL
#define VALUE_DSP( n )      GUI_CMD_VALUE_DSP , ST_VAL( GUI_VAL_##n )
#define VALUE_INP( n )      GUI_CMD_VALUE_INP , ST_VAL( GUI_VAL_##n )

#define GOTO_X( x )         GUI_CMD_GOTO_POS_X   , ST_VAL( x )
#define GOTO_Y( y )         GUI_CMD_GOTO_POS_Y   , ST_VAL( y )
#define GOTO_XY( x , y )    GOTO_X( x ) , GOTO_Y( y )
#define ADD_X( x )          GUI_CMD_ADD_POS_X , ST_VAL( x )
#define ADD_Y( y )          GUI_CMD_ADD_POS_Y , ST_VAL( y )
#define ADD_XY( x , y )     ADD_X( x ) , ADD_Y( y )

#define DRAW_GRAPHIC( g )   GUI_CMD_DRAW_GRAPHIC , ST_VAL( g )
#define LINE( w , h )       DRAW_GRAPHIC( GUI_CMD_GRAPHIC_LINE ) , ST_VAL( w ) , ST_VAL( h )
#define RECT( w , h )       DRAW_GRAPHIC( GUI_CMD_GRAPHIC_RECT ) , ST_VAL( w ) , ST_VAL( h )
#define RECT_FILL( w , h )  DRAW_GRAPHIC( GUI_CMD_GRAPHIC_RECT_FILLED ) , ST_VAL( w ) , ST_VAL( h )
#define CIRCLE( r )         DRAW_GRAPHIC( GUI_CMD_GRAPHIC_CIRCLE ) , ST_VAL( r )

enum
{
    GUI_CMD_OPR_PAGE_TREE_TOP     ,
    GUI_CMD_OPR_GOTO_SCREEN_LEFT  ,
    GUI_CMD_OPR_GOTO_SCREEN_RIGHT ,
    GUI_CMD_OPR_GOTO_SCREEN_TOP   ,
    GUI_CMD_OPR_GOTO_SCREEN_BASE  ,
    GUI_CMD_OPR_GOTO_LINE_TOP     ,
    GUI_CMD_OPR_GOTO_LINE_BASE
};

#define PAGE_TREE_TOP       GUI_CMD_OPERATION , ST_VAL( GUI_CMD_OPR_PAGE_TREE_TOP )
#define GOTO_SCREEN_LEFT    GUI_CMD_OPERATION , ST_VAL( GUI_CMD_OPR_GOTO_SCREEN_LEFT )
#define GOTO_SCREEN_RIGHT   GUI_CMD_OPERATION , ST_VAL( GUI_CMD_OPR_GOTO_SCREEN_RIGHT )
#define GOTO_SCREEN_TOP     GUI_CMD_OPERATION , ST_VAL( GUI_CMD_OPR_GOTO_SCREEN_TOP )
#define GOTO_SCREEN_BASE    GUI_CMD_OPERATION , ST_VAL( GUI_CMD_OPR_GOTO_SCREEN_BASE )
#define GOTO_LINE_TOP       GUI_CMD_OPERATION , ST_VAL( GUI_CMD_OPR_GOTO_LINE_TOP )
#define GOTO_LINE_BASE      GUI_CMD_OPERATION , ST_VAL( GUI_CMD_OPR_GOTO_LINE_BASE )

#ifndef DISPLAY_DEBUG_MSG
  #define DBG_VAL( v )      GUI_CMD_DBG_VAL   , ST_VAL( v )
#endif // DISPLAY_DEBUG_MSG

#define PAGE_END            GUI_CMD_PAGE_END

// List Handling
extern void GuiCmd_List_GetNumOfEntries_Script( GuiState_st* guiState );
extern bool GuiCmd_List_EntryDisplay_Script( GuiState_st* guiState );
extern void GuiCmd_List_EntrySelect_Script( GuiState_st* guiState );
extern bool GuiCmd_List_EntryDisplay_Stubbed( GuiState_st* guiState );
extern void GuiCmd_List_EntryDisplay_TextString( GuiState_st* guiState , char* str );

// Color load fn.
extern void ui_ColorLoad( Color_st* color , ColorSelector_en colorSelector );

extern void ui_Draw_Page( GuiState_st* guiState );
extern void ui_States_SetPageNum( GuiState_st* guiState , uint8_t pageNum );
extern void ui_RenderDisplay( GuiState_st* guiState );

extern void DebugMsg_PrintStr( GuiState_st* guiState , char* scriptPtr );
extern void DebugMsg_PrintLine( GuiState_st* guiState , char* scriptPtr , uint8_t lineNum );

#endif // UI_COMMANDS_H
