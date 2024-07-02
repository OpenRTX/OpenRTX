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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <utils.h>
#include <ui.h>
#include <ui/ui_default.h>
#include <interfaces/nvmem.h>
#include <interfaces/cps_io.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <memory_profiling.h>
#include <ui/ui_strings.h>
#include <core/voicePromptUtils.h>
#include "ui_value_display.h"
#include "ui_value_arrays.h"

#include "ui_main.h"
#include "ui_states.h"
#include "ui_commands.h"
#include "ui_list_display.h"

#ifdef PLATFORM_TTWRPLUS
#include <SA8x8.h>
#endif

void _ui_Draw_MenuTop( GuiState_st* guiState )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Menu" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->menu );
    // Print menu entries
    _ui_Draw_MenuList( guiState , PAGE_MENU_TOP );
}

void _ui_Draw_MenuBank( GuiState_st* guiState )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Bank" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->banks );
    // Print bank entries
    _ui_Draw_MenuList( guiState , PAGE_MENU_BANK );
}

void _ui_Draw_MenuChannel( GuiState_st* guiState )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Channel" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->channels );
    // Print channel entries
    _ui_Draw_MenuList( guiState , PAGE_MENU_CHANNEL );
}

void _ui_Draw_MenuContacts( GuiState_st* guiState )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Contacts" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->contacts );
    // Print contact entries
    _ui_Draw_MenuList( guiState , PAGE_MENU_CONTACTS );
}

#ifdef GPS_PRESENT
void _ui_Draw_MenuGPS( GuiState_st* guiState )
{
    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line1       = &guiState->layout.lines[ GUI_LINE_1 ] ;
//    Style_st* style1      = &guiState->layout.styles[ GUI_STYLE_1 ] ;
    Line_st*  line2       = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2      = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    Line_st*  lineBottom  = &guiState->layout.lines[ GUI_LINE_BOTTOM ] ;
    Style_st* styleBottom = &guiState->layout.styles[ GUI_STYLE_BOTTOM ] ;
    char*     fix_buf ;
    char*     type_buf ;
    Color_st  color_fg ;
    Pos_st    fix_pos     = { line2->pos.x , ( SCREEN_HEIGHT * 2 ) / 5 , 0 , 0 };

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "GPS" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->gps );
    // Print GPS status, if no fix, hide details
    if( !last_state.settings.gps_enabled )
    {
        gfx_print( &fix_pos , style3Large->font.size , GFX_ALIGN_CENTER ,
                   &color_fg , currentLanguage->gpsOff );
    }
    else if( last_state.gps_data.fix_quality == 0 )
    {
        gfx_print( &fix_pos , style3Large->font.size , GFX_ALIGN_CENTER ,
                   &color_fg , currentLanguage->noFix );
    }
    else if( last_state.gps_data.fix_quality == 6 )
    {
        gfx_print( &fix_pos , style3Large->font.size , GFX_ALIGN_CENTER ,
                   &color_fg , currentLanguage->fixLost );
    }
    else
    {
        switch(last_state.gps_data.fix_quality)
        {
            case 1 :
            {
                fix_buf = "SPS" ;
                break ;
            }
            case 2 :
            {
                fix_buf = "DGPS" ;
                break ;
            }
            case 3:
            {
                fix_buf = "PPS" ;
                break ;
            }
            default:
            {
                fix_buf = (char*)currentLanguage->error ;
                break ;
            }
        }

        switch(last_state.gps_data.fix_type)
        {
            case 1:
            {
                type_buf = "" ;
                break ;
            }
            case 2:
            {
                type_buf = "2D" ;
                break ;
            }
            case 3:
            {
                type_buf = "3D" ;
                break ;
            }
            default:
            {
                type_buf = (char*)currentLanguage->error ;
                break ;
            }
        }
        gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
                   &color_fg , fix_buf );
        gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
                   &color_fg , "N     " );
        gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_RIGHT ,
                   &color_fg , "%8.6f" , last_state.gps_data.latitude );
        gfx_print( &line2->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
                   &color_fg , type_buf);
        // Convert from signed longitude, to unsigned + direction
        float longitude = last_state.gps_data.longitude ;
        char* direction = (longitude < 0) ? "W     " : "E     " ;
        longitude = (longitude < 0) ? -longitude : longitude ;
        gfx_print( &line2->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
                   &color_fg , direction );
        gfx_print( &line2->pos , styleTop->font.size , GFX_ALIGN_RIGHT ,
                   &color_fg , "%8.6f" , longitude );
        gfx_print( &lineBottom->pos , styleBottom->font.size , GFX_ALIGN_CENTER ,
                   &color_fg , "S %4.1fkm/h  A %4.1fm" ,
                   last_state.gps_data.speed , last_state.gps_data.altitude );
    }
    // Draw compass
    Pos_st compass_pos = { guiState->layout.horizontal_pad * 2 ,
                           SCREEN_HEIGHT / 2 , 0 , 0 };
    gfx_drawGPScompass( &compass_pos , SCREEN_WIDTH / 9 + 2 ,
                        last_state.gps_data.tmg_true ,
                        last_state.gps_data.fix_quality != 0 &&
                        last_state.gps_data.fix_quality != 6 );
    // Draw satellites bar graph
    Pos_st bar_pos = { line3Large->pos.x + SCREEN_WIDTH * 1 / 3 ,
                       SCREEN_HEIGHT / 2 ,
                      ( ( SCREEN_WIDTH * 2 ) / 3) - guiState->layout.horizontal_pad ,
                      SCREEN_HEIGHT / 3 };
    gfx_drawGPSgraph( &bar_pos ,
                      last_state.gps_data.satellites ,
                      last_state.gps_data.active_sats );
}
#endif

void _ui_Draw_MenuSettings( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Settings" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->settings );
    // Print menu entries
    _ui_Draw_MenuList( guiState , PAGE_MENU_SETTINGS );
}

void _ui_Draw_MenuBackupRestore( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Backup & Restore" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->backupAndRestore );
    // Print menu entries
    _ui_Draw_MenuList( guiState , PAGE_MENU_BACKUP_RESTORE );
}

void _ui_Draw_MenuBackup( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line2    = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2   = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Flash Backup" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->flashBackup );
    // Print backup message
    Pos_st line = line2->pos ;
    gfx_print( &line , FONT_SIZE_8PT , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->connectToRTXTool );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( &line , FONT_SIZE_8PT , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->toBackupFlashAnd );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( &line , FONT_SIZE_8PT , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->pressPTTToStart );

    if( !platform_getPttStatus() )
    {
        return ;
    }

    state.devStatus     = DATATRANSFER ;
    state.backup_eflash = true ;
}

void _ui_Draw_MenuRestore( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line2    = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2   = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Flash Restore" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->flashRestore );
    // Print backup message
    Pos_st line = line2->pos ;
    gfx_print( &line , FONT_SIZE_8PT , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->connectToRTXTool );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( &line , FONT_SIZE_8PT , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->toRestoreFlashAnd );
    line.y += 18 ; // @@@KL hard coding - could produce portability problems
    gfx_print( &line , FONT_SIZE_8PT , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->pressPTTToStart );

    if( !platform_getPttStatus() )
    {
        return ;
    }

    state.devStatus      = DATATRANSFER ;
    state.restore_eflash = true ;
}

void _ui_Draw_MenuInfo( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Info" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->info );
    // Print menu entries
    _ui_Draw_MenuListValue( guiState ,
                           PAGE_MENU_INFO , PAGE_MENU_INFO );
}

void _ui_Draw_SettingsDisplay( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Display" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->display );
    // Print display settings entries
    _ui_Draw_MenuListValue( guiState ,
                           PAGE_SETTINGS_DISPLAY , PAGE_SETTINGS_DISPLAY );
}

#ifdef GPS_PRESENT
void _ui_Draw_SettingsGPS( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "GPS Settings" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->gpsSettings );
    // Print display settings entries
    _ui_Draw_MenuListValue( guiState ,
                           PAGE_SETTINGS_GPS , PAGE_SETTINGS_GPS );
}
#endif

#ifdef RTC_PRESENT
void _ui_Draw_SettingsTimeDate( GuiState_st* guiState )
{
    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line2       = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2      = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
//    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    datetime_t local_time = utcToLocalTime( last_state.time ,
                                            last_state.settings.utc_timezone );
    // Print "Time&Date" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->timeAndDate );
    // Print current time and date
    gfx_print( &line2->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
               &color_fg , "%02d/%02d/%02d" ,
               local_time.date , local_time.month , local_time.year );
    gfx_print( &line3Large->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
               &color_fg , "%02d:%02d:%02d" ,
               local_time.hour , local_time.minute , local_time.second );
}

void _ui_Draw_SettingsTimeDateSet( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line2       = &guiState->layout.lines[ GUI_LINE_2 ] ;
//    Style_st* style2      = &guiState->layout.styles[ GUI_STYLE_2 ] ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
//    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->timeAndDate );

    if( guiState->uiState.input_position <= 0 )
    {
        strcpy( guiState->uiState.new_date_buf , "__/__/__" );
        strcpy( guiState->uiState.new_time_buf , "__:__:00" );
    }
    else
    {
        char input_char = guiState->uiState.input_number + '0' ;
        // Insert date digit
        if( guiState->uiState.input_position <= 6 )
        {
            uint8_t pos = guiState->uiState.input_position -1 ;
            // Skip "/"
            if( guiState->uiState.input_position > 2 ) pos += 1 ;
            if( guiState->uiState.input_position > 4 ) pos += 1 ;
            guiState->uiState.new_date_buf[ pos ] = input_char ;
        }
        // Insert time digit
        else
        {
            uint8_t pos = guiState->uiState.input_position -7 ;
            // Skip ":"
            if( guiState->uiState.input_position > 8 ) pos += 1 ;
            guiState->uiState.new_time_buf[ pos ] = input_char ;
        }
    }
    gfx_print( &line2->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
               &color_fg , guiState->uiState.new_date_buf );
    gfx_print( &line3Large->pos , guiState->layout.input_font.size , GFX_ALIGN_CENTER ,
               &color_fg , guiState->uiState.new_time_buf );
}
#endif

void _ui_Draw_SettingsM17( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  lineBottom  = &guiState->layout.lines[ GUI_LINE_BOTTOM ] ;
//    Style_st* styleBottom = &guiState->layout.styles[ GUI_STYLE_BOTTOM ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "M17 Settings" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->m17settings );
    gfx_printLine( 1 , 4 , lineTop->height , SCREEN_HEIGHT - lineBottom->height ,
                   guiState->layout.horizontal_pad , guiState->layout.menu_font.size ,
                   GFX_ALIGN_LEFT , &color_fg , currentLanguage->callsign );
    if( guiState->uiState.edit_mode && ( guiState->uiState.entrySelected == M17_CALLSIGN ) )
    {
        Pos_st rect_origin ;

        rect_origin.w =   SCREEN_WIDTH  - ( guiState->layout.horizontal_pad * 2 ) ;
        rect_origin.h = ( SCREEN_HEIGHT - ( lineTop->height + lineBottom->height ) ) / 2 ;
        rect_origin.x = ( SCREEN_WIDTH  - rect_origin.w ) / 2 ;
        rect_origin.y = ( SCREEN_HEIGHT - rect_origin.h ) / 2 ;

        gfx_drawRect( &rect_origin , &color_fg , false );

        // Print M17 callsign being typed
        gfx_printLine( 1 , 1 , lineTop->height , SCREEN_HEIGHT - lineBottom->height ,
                       guiState->layout.horizontal_pad , guiState->layout.input_font.size ,
                       GFX_ALIGN_CENTER , &color_fg , guiState->uiState.new_callsign );
    }
    else
    {
        _ui_Draw_MenuListValue( guiState , PAGE_SETTINGS_M17 , PAGE_SETTINGS_M17 );
    }
}

void _ui_Draw_SettingsVoicePrompts( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Voice" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->voice );
    // Print voice settings entries
    _ui_Draw_MenuListValue( guiState ,
                           PAGE_SETTINGS_VOICE , PAGE_SETTINGS_VOICE );
}

void _ui_Draw_SettingsReset2Defaults( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*         lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st*        styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*         lineBottom  = &guiState->layout.lines[ GUI_LINE_BOTTOM ] ;
//    Style_st*        styleBottom = &guiState->layout.styles[ GUI_STYLE_BOTTOM ] ;
    static int       drawcnt     = 0 ;
    static long long lastDraw    = 0 ;
    Color_st         text_color ;
    Color_st         color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->resetToDefaults );

    // Make text flash yellow once every 1s
    if( drawcnt % 2 == 0 )
    {
        ui_ColorLoad( &text_color , COLOR_FG );
    }
    else
    {
        ui_ColorLoad( &text_color , COLOR_OP3 );
    }
    gfx_printLine( 1 , 4 ,
                   lineTop->height ,
                   SCREEN_HEIGHT - lineBottom->height ,
                   guiState->layout.horizontal_pad ,
                   styleTop->font.size ,
                   GFX_ALIGN_CENTER , &text_color , currentLanguage->toReset );
    gfx_printLine( 2 , 4 , lineTop->height ,
                    SCREEN_HEIGHT - lineBottom->height ,
                   guiState->layout.horizontal_pad ,
                   styleTop->font.size ,
                   GFX_ALIGN_CENTER , &text_color , currentLanguage->pressEnterTwice );

    if( ( getTick() - lastDraw ) > 1000 )
    {
        drawcnt++ ;
        lastDraw = getTick();
    }

    drawcnt++ ;
}

void _ui_Draw_SettingsRadio( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  lineBottom  = &guiState->layout.lines[ GUI_LINE_BOTTOM ] ;
//    Style_st* styleBottom = &guiState->layout.styles[ GUI_STYLE_BOTTOM ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();

    // Print "Radio Settings" on top bar
    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->radioSettings );

    // Handle the special case where a frequency is being input
    if( ( guiState->uiState.entrySelected == R_OFFSET ) && ( guiState->uiState.edit_mode ) )
    {
        char   buf[ 17 ] = { 0 };
        Pos_st rect_origin ;

        rect_origin.w =   SCREEN_WIDTH  - ( guiState->layout.horizontal_pad * 2 ) ;
        rect_origin.h = ( SCREEN_HEIGHT - ( lineTop->height + lineBottom->height ) ) / 2 ;
        rect_origin.x = ( SCREEN_WIDTH  - rect_origin.w ) / 2 ;
        rect_origin.y = ( SCREEN_HEIGHT - rect_origin.h ) / 2 ;

        gfx_drawRect( &rect_origin, &color_fg, false);

        // Print frequency with the most sensible unit
        if( guiState->uiState.new_offset < 1000 )
        {
            snprintf( buf , 17 , "%dHz" , (int)guiState->uiState.new_offset );
        }
        else if( guiState->uiState.new_offset < 1000000 )
        {
            snprintf( buf , 17 , "%gkHz" , (float)guiState->uiState.new_offset / 1000.0f );
        }
        else
        {
            snprintf( buf , 17 , "%gMHz" , (float)guiState->uiState.new_offset / 1000000.0f );
        }
        gfx_printLine( 1 , 1 , lineTop->height , SCREEN_HEIGHT - lineBottom->height ,
                       guiState->layout.horizontal_pad , guiState->layout.input_font.size ,
                       GFX_ALIGN_CENTER , &color_fg, buf );
    }
    else
    {
        // Print radio settings entries
        _ui_Draw_MenuListValue( guiState ,
                               PAGE_SETTINGS_RADIO , PAGE_SETTINGS_RADIO );
    }
}

void _ui_Draw_MacroTop( GuiState_st* guiState )
{
    Line_st*  lineTop  = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Color_st  color_fg ;

    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_print( &lineTop->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , currentLanguage->macroMenu );

    if( macro_latched )
    {
        gfx_drawSymbol( &lineTop->pos ,
                        styleTop->symbolSize , GFX_ALIGN_LEFT ,
                        &color_fg , SYMBOL_ALPHA_M_BOX_OUTLINE );
    }
    if( last_state.settings.gps_enabled )
    {
        if( last_state.gps_data.fix_quality > 0 )
        {
            gfx_drawSymbol( &lineTop->pos , styleTop->symbolSize , GFX_ALIGN_RIGHT ,
                            &color_fg , SYMBOL_CROSSHAIRS_GPS );
        }
        else
        {
            gfx_drawSymbol( &lineTop->pos , styleTop->symbolSize , GFX_ALIGN_RIGHT ,
                            &color_fg , SYMBOL_CROSSHAIRS );
        }
    }
}

bool _ui_Draw_MacroMenu( GuiState_st* guiState , Event_st* event )
{
    (void)event ;
//    Line_st*  lineTop     = &guiState->layout.lines[ GUI_LINE_TOP ] ;
    Style_st* styleTop    = &guiState->layout.styles[ GUI_STYLE_TOP ] ;
    Line_st*  line1       = &guiState->layout.lines[ GUI_LINE_1 ] ;
//    Style_st* style1      = &guiState->layout.styles[ GUI_STYLE_1 ] ;
    Line_st*  line3Large  = &guiState->layout.lines[ GUI_LINE_3_LARGE ] ;
//    Style_st* style3Large = &guiState->layout.styles[ GUI_STYLE_3_LARGE ] ;
    Color_st  color_fg ;
    Color_st  color_op3 ;

    ui_ColorLoad( &color_fg , COLOR_FG );
    ui_ColorLoad( &color_op3 , COLOR_OP3 );

    // Header
    _ui_Draw_MacroTop( guiState );
    // First row
    if( last_state.channel.mode == OPMODE_FM )
    {
/*
 * If we have a keyboard installed draw all numbers, otherwise draw only the
 * currently selected number.
 */
#ifdef UI_NO_KEYBOARD
        if( guiState->uiState.macro_entrySelected == 0 )
        {
#endif // UI_NO_KEYBOARD
            gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
                       &color_op3 , "1" );
            gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
                       &color_fg , "   T-" );
            gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
                       &color_fg , "     %7.1f" ,
                       ctcss_tone[ last_state.channel.fm.txTone ] / 10.0f );
#ifdef UI_NO_KEYBOARD
        }
#endif // UI_NO_KEYBOARD
#ifdef UI_NO_KEYBOARD
        if( guiState->uiState.macro_entrySelected == 1 )
        {
#endif // UI_NO_KEYBOARD
            gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
                       &color_op3 , "2" );
            gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
                       &color_fg ,   "       T+" );
#ifdef UI_NO_KEYBOARD
        }
#endif // UI_NO_KEYBOARD
    }
    else
    {
        if( last_state.channel.mode == OPMODE_M17 )
        {
            gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
                       &color_op3 , "1" );
            gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
                       &color_fg , "          " );
            gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
                       &color_op3 , "2" );
        }
    }
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 2 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_RIGHT ,
                   &color_op3 , "3        " );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD

    if( last_state.channel.mode == OPMODE_FM )
    {
        char encdec_str[ 9 ] = { 0 };
        bool tone_tx_enable = last_state.channel.fm.txToneEn ;
        bool tone_rx_enable = last_state.channel.fm.rxToneEn ;

        if( tone_tx_enable && tone_rx_enable )
        {
            snprintf( encdec_str , 9 , "     E+D" );
        }
        else
        {
            if( tone_tx_enable && !tone_rx_enable )
            {
                snprintf( encdec_str , 9 , "      E " );
            }
            else
            {
                if( !tone_tx_enable && tone_rx_enable )
                {
                    snprintf( encdec_str , 9 , "      D " );
                }
                else
                {
                    snprintf( encdec_str , 9 , "        " );
                }
            }
        }
        gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_RIGHT ,
                   &color_fg , encdec_str );
    }
    else
    {
        if ( last_state.channel.mode == OPMODE_M17 )
        {
            char encdec_str[ 9 ] = "        " ;
            gfx_print( &line1->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
                       &color_fg , encdec_str );
        }
    }
    // Second row
    // Calculate symmetric second row position, line2_pos is asymmetric like main screen
    Pos_st pos_2 = { line1->pos.x ,
                      line1->pos.y + ( line3Large->pos.y - line3Large->pos.y ) / 2 , 0 , 0 };
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 3 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( &pos_2 , styleTop->font.size , GFX_ALIGN_LEFT ,
                   &color_op3 , "4" );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
    if( last_state.channel.mode == OPMODE_FM )
    {
        char bw_str[ 12 ] = { 0 };
        switch( last_state.channel.bandwidth )
        {
            case BW_12_5 :
            {
                snprintf( bw_str , 12 , "   BW 12.5" );
                break ;
            }
            case BW_20 :
            {
                snprintf( bw_str , 12 , "   BW  20 " );
                break ;
            }
            case BW_25 :
            {
                snprintf( bw_str , 12 , "   BW  25 " );
                break ;
            }
        }
        gfx_print( &pos_2 , styleTop->font.size , GFX_ALIGN_LEFT ,
                   &color_fg , bw_str );
    }
    else
    {
        if( last_state.channel.mode == OPMODE_M17 )
        {
            gfx_print( &pos_2 , styleTop->font.size , GFX_ALIGN_LEFT ,
                       &color_fg , "       " );

        }
    }
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 4 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( &pos_2 , styleTop->font.size , GFX_ALIGN_CENTER ,
                   &color_op3 , "5" );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
    char mode_str[12] = "";
    switch( last_state.channel.mode )
    {
        case OPMODE_FM :
        {
            snprintf( mode_str , 12 ,"         FM" );
            break ;
        }
        case OPMODE_DMR :
        {
            snprintf( mode_str , 12 ,"        DMR" );
            break ;
        }
        case OPMODE_M17 :
        {
            snprintf( mode_str , 12 ,"        M17" );
            break ;
        }
    }
    gfx_print( &pos_2 , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg , mode_str );
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 5 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( &pos_2 , styleTop->font.size , GFX_ALIGN_RIGHT ,
                   &color_op3 , "6        " );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
    gfx_print( &pos_2 , styleTop->font.size , GFX_ALIGN_RIGHT ,
               &color_fg , "%.1gW" , dBmToWatt( last_state.channel.power ) );
    // Third row
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 6 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( &line3Large->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
                   &color_op3 , "7" );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
#ifdef SCREEN_BRIGHTNESS
    gfx_print( &line3Large->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
               &color_fg , "   B-" );
    gfx_print( &line3Large->pos , styleTop->font.size , GFX_ALIGN_LEFT ,
               &color_fg , "       %5d" , state.settings.brightness );
#endif
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 7 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( &line3Large->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
                   &color_op3 , "8" );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
#ifdef SCREEN_BRIGHTNESS
    gfx_print( &line3Large->pos , styleTop->font.size , GFX_ALIGN_CENTER ,
               &color_fg ,   "       B+" );
#endif
#ifdef UI_NO_KEYBOARD
    if( guiState->uiState.macro_entrySelected == 8 )
    {
#endif // UI_NO_KEYBOARD
        gfx_print( &line3Large->pos , styleTop->font.size , GFX_ALIGN_RIGHT ,
                   &color_op3 , "9        " );
#ifdef UI_NO_KEYBOARD
    }
#endif // UI_NO_KEYBOARD
    if( guiState->uiState.input_locked == true )
    {
       gfx_print( &line3Large->pos , styleTop->font.size , GFX_ALIGN_RIGHT ,
                  &color_fg , "Unlk" );
    }
    else
    {
       gfx_print( &line3Large->pos , styleTop->font.size , GFX_ALIGN_RIGHT ,
                  &color_fg , "Lck" );
    }
    // Draw S-meter bar
    _ui_Draw_MainBottom( guiState , event );
    return true;
}
