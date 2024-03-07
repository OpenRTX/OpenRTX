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

// GUI Commands
enum
{
    GUI_CMD_NULL         = 0x00 ,
    GUI_CMD_STATIC_START = 0x01 , // static display - new page - text and variables
    GUI_CMD_STATIC_END   = 0x02 , //  used with pages that respond to events
    GUI_CMD_EVENT_START  = 0x03 , // dynamic display - variables updated
    GUI_CMD_EVENT_END    = 0x04 , //  used with pages that respond to events
    GUI_CMD_TIMER_CHECK  = 0x05 ,
    GUI_CMD_TIMER_SET    = 0x06 ,
    GUI_CMD_ALIGN_LEFT   = 0x07 ,
    GUI_CMD_ALIGN_CENTER = 0x08 ,
    GUI_CMD_ALIGN_RIGHT  = 0x09 ,
    GUI_CMD_LINE_END     = 0x0A ,
    GUI_CMD_LINE         = 0x0B ,
    GUI_CMD_STYLE        = 0x0C ,
    GUI_CMD_RUN_SCRIPT   = 0x0D ,
    GUI_CMD_LINK         = 0x0E ,
    GUI_CMD_LINK_END     = 0x0F ,
    GUI_CMD_PAGE         = 0x10 ,
    GUI_CMD_VALUE_DSP    = 0x11 ,
    GUI_CMD_VALUE_INP    = 0x12 ,
    GUI_CMD_TITLE        = 0x13 ,
    GUI_CMD_TEXT         = 0x14 ,
    GUI_CMD_STUBBED      = 0x1E ,
    GUI_CMD_PAGE_END     = 0x1F ,
    GUI_CMD_NUM_OF              ,
    GUI_CMD_DATA_AREA    = 0x20
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
// Color load fn.
extern void ui_ColorLoad( Color_st* color , ColorSelector_en colorSelector );

extern bool ui_DisplayPage( GuiState_st* guiState );
extern void ui_SetPageNum( GuiState_st* guiState , uint8_t pageNum );

#endif // UI_COMMANDS_H
