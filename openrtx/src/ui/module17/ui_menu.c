/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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
#include <utils.h>
#include <ui/ui_mod17.h>
#include <interfaces/nvmem.h>
#include <interfaces/cps_io.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <memory_profiling.h>

int _ui_getMenuTopEntryName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getBankName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getChannelName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getContactName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getSettingsEntryName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getBackupRestoreEntryName( char* buf , uint8_t max_len , uint8_t index );

int _ui_getInfoEntryName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getDisplayEntryName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getSettingsGPSEntryName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getM17EntryName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getVoiceEntryName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getRadioEntryName( char* buf , uint8_t max_len , uint8_t index );

int _ui_getInfoValueName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getDisplayValueName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getSettingsGPSValueName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getM17ValueName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getVoiceValueName( char* buf , uint8_t max_len , uint8_t index );
int _ui_getRadioValueName( char* buf , uint8_t max_len , uint8_t index );

static bool DidSelectedMenuItemChange( char* menuName , char* menuValue );
static bool ScreenContainsReadOnlyEntries( int menuScreen );
static void announceMenuItemIfNeeded( GuiState_st* guiState , char* name , char* value , bool editMode );

/* UI main screen helper functions, their implementation is in "ui_main.c" */
extern void _ui_Draw_MainBottom();

const char *display_timer_values[] =
{
    "Off",
    "5 s",
    "10 s",
    "15 s",
    "20 s",
    "25 s",
    "30 s",
    "1 min",
    "2 min",
    "3 min",
    "4 min",
    "5 min",
    "15 min",
    "30 min",
    "45 min",
    "1 hour"
};

const char *mic_gain_values[] =
{
    "40 dB",
    "50 dB",
    "60 dB"
};

const char *phase_values[] =
{
    "Normal",
    "Inverted"
};

const char *hwVersions[] =
{
    "0.1d",
    "0.1e"
};

static const GetMenuList_fn GetEntryName_table[ PAGE_NUM_OF ] =
{
    _ui_getMenuTopEntryName       ,
    _ui_getBankName               ,
    _ui_getChannelName            ,
    _ui_getContactName            ,
    _ui_getSettingsEntryName      ,
    _ui_getBackupRestoreEntryName ,
    _ui_getInfoEntryName          ,
    _ui_getDisplayEntryName       ,
    _ui_getSettingsGPSEntryName   ,
    _ui_getM17EntryName           ,
    _ui_getVoiceEntryName         ,
    _ui_getRadioEntryName
};

void _ui_Draw_MenuList(uint8_t selected, uiPageNum_en currentEntry )
{
    GetMenuList_fn getCurrentEntry = GetEntryName_table[ currentEntry ];

    Pos_st pos = layout.lines[ GUI_LINE_1 ].pos;
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen = (SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );
    Color_st color_bg ;
    ui_ColorLoad( &color_bg , COLOR_BG );
    Color_st text_color = color_fg ;

    for(int item=0, result=0; (result == 0) && (pos.y < SCREEN_HEIGHT); item++)
    {
        // If selection is off the screen, scroll screen
        if(selected >= entries_in_screen)
            scroll = selected - entries_in_screen + 1;
        // Call function pointer to get current menu entry string
        result = (*getCurrentEntry)(entry_buf, sizeof(entry_buf), item+scroll);
        if(result != -1)
        {
            text_color = color_fg;
            if(item + scroll == selected)
            {
                text_color = color_bg;
                // Draw rectangle under selected item, compensating for text height
                Pos_st rect_pos = {0, pos.y - layout.menu_h + 3};
                gfx_drawRect(rect_pos, SCREEN_WIDTH, layout.menu_h, color_fg, true);
            }
            gfx_print(pos, layout.menu_font.size, ALIGN_LEFT, text_color, entry_buf);
            pos.y += layout.menu_h;
        }
    }
}

static const GetMenuList_fn GetEntryValue_table[ ENTRY_VALUE_NUM_OF ] =
{
    _ui_getInfoValueName        ,
    _ui_getDisplayValueName     ,
    _ui_getSettingsGPSValueName ,
    _ui_getM17ValueName         ,
    _ui_getVoiceValueName       ,
    _ui_getRadioValueName
};

void _ui_Draw_MenuListValue( UI_State_st* ui_state , uint8_t selected ,
                            uiPageNum_en currentEntry , uiPageNum_en currentEntryValue )
{
    GetMenuList_fn getCurrentEntry = GetEntryName_table[ currentEntry ];
    GetMenuList_fn getCurrentValue = GetEntryValue_table[ currentEntryValue ];

    Pos_st pos = layout.lines[ GUI_LINE_1 ].pos;
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen = (SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    char value_buf[MAX_ENTRY_LEN] = "";
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );
    Color_st color_bg ;
    ui_ColorLoad( &color_bg , COLOR_BG );
    Color_st text_color = color_fg ;

    for(int item=0, result=0; (result == 0) && (pos.y < SCREEN_HEIGHT); item++)
    {
        // If selection is off the screen, scroll screen
        if(selected >= entries_in_screen)
            scroll = selected - entries_in_screen + 1;
        // Call function pointer to get current menu entry string
        result = (*getCurrentEntry)(entry_buf, sizeof(entry_buf), item+scroll);
        // Call function pointer to get current entry value string
        result = (*getCurrentValue)(value_buf, sizeof(value_buf), item+scroll);
        if(result != -1)
        {
            text_color = color_fg;
            if(item + scroll == selected)
            {
                // Draw rectangle under selected item, compensating for text height
                // If we are in edit mode, draw a hollow rectangle
                text_color = color_bg;
                bool full_rect = true;
                if(ui_state->edit_mode)
                {
                    text_color = color_fg;
                    full_rect = false;
                }
                Pos_st rect_pos = {0, pos.y - layout.menu_h + 3};
                gfx_drawRect(rect_pos, SCREEN_WIDTH, layout.menu_h, color_fg, full_rect);
            }
            gfx_print(pos, layout.menu_font.size, ALIGN_LEFT, text_color, entry_buf);
            gfx_print(pos, layout.menu_font.size, ALIGN_RIGHT, text_color, value_buf);
            pos.y += layout.menu_h;
        }
    }
}

int _ui_getMenuTopEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    uint8_t maxEntries = menu_num;
    if(platform_getHwInfo()->hw_version < 1)
        maxEntries -= 1;

    if(index >= maxEntries) return -1;
    snprintf(buf, max_len, "%s", Page_MenuItems[index]);
    return 0;
}

int _ui_getSettingsEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_num) return -1;
    snprintf(buf, max_len, "%s", Page_MenuSettings[index]);
    return 0;
}

int _ui_getDisplayEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= display_num) return -1;
    snprintf(buf, max_len, "%s", PAGE_MENU_SETTINGSDisplay[index]);
    return 0;
}

int _ui_getDisplayValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= display_num) return -1;
    uint8_t value = 0;
    switch(index)
    {
#ifdef SCREEN_CONTRAST
        case D_CONTRAST:
            value = last_state.settings.contrast;
            break;
#endif
        case D_TIMER:
            snprintf(buf, max_len, "%s",
                     display_timer_values[last_state.settings.display_timer]);
            return 0;
    }
    snprintf(buf, max_len, "%d", value);
    return 0;
}

int _ui_getM17EntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= m17_num) return -1;
    snprintf(buf, max_len, "%s", m17_items[index]);
    return 0;
}

int _ui_getM17ValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= m17_num) return -1;

    switch(index)
    {
        case M_CALLSIGN:
            snprintf(buf, max_len, "%s", last_state.settings.callsign);
            return 0;
        case M_CAN:
            snprintf(buf, max_len, "%d", last_state.settings.m17_can);
            break;
        case M_CAN_RX:
            snprintf(buf, max_len, "%s", (last_state.settings.m17_can_rx) ? "on" : "off");
            break;
    }

    return 0;
}

int _ui_getModule17EntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= module17_num) return -1;
    snprintf(buf, max_len, "%s", module17_items[index]);
    return 0;
}

int _ui_getModule17ValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= module17_num) return -1;

    switch(index)
    {
        case D_TXWIPER:
            snprintf(buf, max_len, "%d", mod17CalData.tx_wiper);
            break;
        case D_RXWIPER:
            snprintf(buf, max_len, "%d", mod17CalData.rx_wiper);
            break;
        case D_TXINVERT:
            snprintf(buf, max_len, "%s", phase_values[mod17CalData.tx_invert]);
            break;
        case D_RXINVERT:
            snprintf(buf, max_len, "%s", phase_values[mod17CalData.rx_invert]);
            break;
        case D_MICGAIN:
            snprintf(buf, max_len, "%s", mic_gain_values[mod17CalData.mic_gain]);
            break;
    }

    return 0;
}

#ifdef GPS_PRESENT
int _ui_getSettingsGPSEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_gps_num) return -1;
    snprintf(buf, max_len, "%s", PAGE_MENU_SETTINGSGPS[index]);
    return 0;
}

int _ui_getSettingsGPSValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_gps_num) return -1;
    switch(index)
    {
        case G_ENABLED:
            snprintf(buf, max_len, "%s", (last_state.settings.gps_enabled) ? "ON" : "OFF");
            break;
        case G_SET_TIME:
            snprintf(buf, max_len, "%s", (last_state.gps_set_time) ? "ON" : "OFF");
            break;
        case G_TIMEZONE:
            // Add + prefix to positive numbers
            if(last_state.settings.utc_timezone > 0)
                snprintf(buf, max_len, "+%d", last_state.settings.utc_timezone);
            else
                snprintf(buf, max_len, "%d", last_state.settings.utc_timezone);
            break;
    }
    return 0;
}
#endif

int _ui_getInfoEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= info_num) return -1;
    snprintf(buf, max_len, "%s", Page_MenuInfo[index]);
    return 0;
}

int _ui_getInfoValueName(char *buf, uint8_t max_len, uint8_t index)
{
    const hwInfo_t* hwinfo = platform_getHwInfo();
    if(index >= info_num) return -1;
    switch(index)
    {
        case 0: // Git Version
            snprintf(buf, max_len, "%s", GIT_VERSION);
            break;
        case 1: // Heap usage
            snprintf(buf, max_len, "%dB", getHeapSize() - getCurrentFreeHeap());
            break;
        case 2: // LCD Type
            snprintf(buf, max_len, "%s", hwVersions[hwinfo->hw_version]);
            break;
    }
    return 0;
}

void _ui_Draw_MenuTop(UI_State_st* ui_state)
{
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Menu" on top bar
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
              color_fg, "Menu");
    // Print menu entries
    _ui_Draw_MenuList(ui_state->entrySelected, PAGE_MENU_TOP );
}

#ifdef GPS_PRESENT
void _ui_Draw_MenuGPS()
{
    char *fix_buf, *type_buf;
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "GPS" on top bar
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
              color_fg, "GPS");
    Pos_st fix_pos = {layout.lines[ GUI_LINE_2 ].pos.x, SCREEN_HEIGHT * 2 / 5};
    // Print GPS status, if no fix, hide details
    if(!last_state.settings.gps_enabled)
        gfx_print(fix_pos, layout.lines[ GUI_LINE_3 ]_font, ALIGN_CENTER,
                  color_fg, "GPS OFF");
    else if (last_state.gps_data.fix_quality == 0)
        gfx_print(fix_pos, layout.lines[ GUI_LINE_3 ]_font, ALIGN_CENTER,
                  color_fg, "No Fix");
    else if (last_state.gps_data.fix_quality == 6)
        gfx_print(fix_pos, layout.lines[ GUI_LINE_3 ]_font, ALIGN_CENTER,
                  color_fg, "Fix Lost");
    else
    {
        switch(last_state.gps_data.fix_quality)
        {
            case 1:
                fix_buf = "SPS";
                break;
            case 2:
                fix_buf = "DGPS";
                break;
            case 3:
                fix_buf = "PPS";
                break;
            default:
                fix_buf = "ERROR";
                break;
        }

        switch(last_state.gps_data.fix_type)
        {
            case 1:
                type_buf = "";
                break;
            case 2:
                type_buf = "2D";
                break;
            case 3:
                type_buf = "3D";
                break;
            default:
                type_buf = "ERROR";
                break;
        }
        gfx_print(layout.lines[ GUI_LINE_1 ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_LEFT,
                  color_fg, fix_buf);
        gfx_print(layout.lines[ GUI_LINE_1 ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
                  color_fg, "N     ");
        gfx_print(layout.lines[ GUI_LINE_1 ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_RIGHT,
                  color_fg, "%8.6f", last_state.gps_data.latitude);
        gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_LEFT,
                  color_fg, type_buf);
        // Convert from signed longitude, to unsigned + direction
        float longitude = last_state.gps_data.longitude;
        char *direction = (longitude < 0) ? "W     " : "E     ";
        longitude = (longitude < 0) ? -longitude : longitude;
        gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
                  color_fg, direction);
        gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_RIGHT,
                  color_fg, "%8.6f", longitude);
        gfx_print(layout.lines[ GUI_LINE_BOTTOM ].pos, layout.lines[ GUI_LINE_BOTTOM ].font, ALIGN_CENTER,
                  color_fg, "S %4.1fkm/h  A %4.1fm",
                  last_state.gps_data.speed,
                  last_state.gps_data.altitude);
    }
    // Draw compass
    Pos_st compass_pos = {layout.horizontal_pad * 2, SCREEN_HEIGHT / 2};
    gfx_drawGPScompass(compass_pos,
                       SCREEN_WIDTH / 9 + 2,
                       last_state.gps_data.tmg_true,
                       last_state.gps_data.fix_quality != 0 &&
                       last_state.gps_data.fix_quality != 6);
    // Draw satellites bar graph
    Pos_st bar_pos = {layout.lines[ GUI_LINE_3 ].pos.x + SCREEN_WIDTH * 1 / 3, SCREEN_HEIGHT / 2};
    gfx_drawGPSgraph(bar_pos,
                     (SCREEN_WIDTH * 2 / 3) - layout.horizontal_pad,
                     SCREEN_HEIGHT / 3,
                     last_state.gps_data.satellites,
                     last_state.gps_data.active_sats);
}
#endif

void _ui_Draw_MenuSettings(UI_State_st* ui_state)
{
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Settings" on top bar
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
              color_fg, "Settings");
    // Print menu entries
    _ui_Draw_MenuList(ui_state->entrySelected, PAGE_MENU_SETTINGS );
}

void _ui_Draw_MenuInfo(UI_State_st* ui_state)
{
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Info" on top bar
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
              color_fg, "Info");
    // Print menu entries
    _ui_Draw_MenuListValue(ui_state, ui_state->entrySelected,
                          PAGE_MENU_INFO , PAGE_MENU_INFO);
}

void _ui_Draw_MenuAbout()
{
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();

    Pos_st openrtx_pos = {layout.horizontal_pad, layout.lines[ GUI_LINE_3 ].height};
    gfx_print(openrtx_pos, layout.lines[ GUI_LINE_3 ]_font, ALIGN_CENTER, color_fg,
              "OpenRTX");

    uint8_t line_h = layout.menu_h;
    Pos_st pos = {SCREEN_WIDTH / 7, SCREEN_HEIGHT - (line_h * (author_num - 1)) - 5};
    for(int author = 0; author < author_num; author++)
    {
        gfx_print(pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_LEFT,
                  color_fg, "%s", authors[author]);
        pos.y += line_h;
    }
}

void _ui_Draw_SettingsDisplay(UI_State_st* ui_state)
{
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Display" on top bar
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
              color_fg, "Display");
    // Print display settings entries
    _ui_Draw_MenuListValue(ui_state, ui_state->entrySelected,
                           PAGE_SETTINGS_DISPLAY , PAGE_SETTINGS_DISPLAY);
}

#ifdef GPS_PRESENT
void _ui_Draw_SettingsGPS(UI_State_st* ui_state)
{
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "GPS Settings" on top bar
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
              color_fg, "GPS Settings");
    // Print display settings entries
    _ui_Draw_MenuListValue(ui_state, ui_state->entrySelected,
                          PAGE_SETTINGS_GPS, PAGE_SETTINGS_GPS);
}
#endif

#ifdef RTC_PRESENT
void _ui_Draw_SettingsTimeDate()
{
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    datetime_t local_time = utcToLocalTime(last_state.time,
                                           last_state.settings.utc_timezone);
    // Print "Time&Date" on top bar
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
              color_fg, "Time&Date");
    // Print current time and date
    gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.input_font.size, ALIGN_CENTER,
              color_fg, "%02d/%02d/%02d",
              local_time.date, local_time.month, local_time.year);
    gfx_print(layout.lines[ GUI_LINE_3 ].pos, layout.input_font.size, ALIGN_CENTER,
              color_fg, "%02d:%02d:%02d",
              local_time.hour, local_time.minute, local_time.second);
}

void _ui_Draw_SettingsTimeDateSet(UI_State_st* ui_state)
{
    (void) last_state;
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
              color_fg, "Time&Date");
    if(ui_state->input_position <= 0)
    {
        strcpy(ui_state->new_date_buf, "__/__/__");
        strcpy(ui_state->new_time_buf, "__:__:00");
    }
    else
    {
        char input_char = ui_state->input_number + '0';
        // Insert date digit
        if(ui_state->input_position <= 6)
        {
            uint8_t pos = ui_state->input_position -1;
            // Skip "/"
            if(ui_state->input_position > 2) pos += 1;
            if(ui_state->input_position > 4) pos += 1;
            ui_state->new_date_buf[pos] = input_char;
        }
        // Insert time digit
        else
        {
            uint8_t pos = ui_state->input_position -7;
            // Skip ":"
            if(ui_state->input_position > 8) pos += 1;
            ui_state->new_time_buf[pos] = input_char;
        }
    }
    gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.input_font.size, ALIGN_CENTER,
              color_fg, ui_state->new_date_buf);
    gfx_print(layout.lines[ GUI_LINE_3 ].pos, layout.input_font.size, ALIGN_CENTER,
              color_fg, ui_state->new_time_buf);
}
#endif

void _ui_Draw_SettingsM17(UI_State_st* ui_state)
{
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
              color_fg, "M17 Settings");

    if(ui_state->edit_mode)
    {
        gfx_printLine(1, 4, layout.lines[ GUI_LINE_TOP ].height, SCREEN_HEIGHT - layout.lines[ GUI_LINE_BOTTOM ].height,
                    layout.horizontal_pad, layout.menu_font.size,
                    ALIGN_LEFT, color_fg, "Callsign:");

        // uint16_t rect_width = SCREEN_WIDTH - (layout.horizontal_pad * 2);
        // uint16_t rect_height = (SCREEN_HEIGHT - (layout.lines[ GUI_LINE_TOP ].height + layout.lines[ GUI_LINE_BOTTOM ].height))/2;
        // Pos_st rect_origin = {(SCREEN_WIDTH - rect_width) / 2,
        //                        (SCREEN_HEIGHT - rect_height) / 2};
        // gfx_drawRect(rect_origin, rect_width, rect_height, color_fg, false);
        // Print M17 callsign being typed
        gfx_printLine(1, 1, layout.lines[ GUI_LINE_TOP ].height, SCREEN_HEIGHT - layout.lines[ GUI_LINE_BOTTOM ].height,
                      layout.horizontal_pad, layout.input_font.size,
                      ALIGN_CENTER, color_fg, ui_state->new_callsign);
    }
    else
    {
        _ui_Draw_MenuListValue(ui_state, ui_state->entrySelected,
                              PAGE_SETTINGS_M17, PAGE_SETTINGS_M17);
    }
}

void _ui_Draw_SettingsModule17(UI_State_st* ui_state)
{
    gfx_clearScreen();
    // Print "Module17 Settings" on top bar
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
              color_fg, "Module17 Settings");
    // Print Module17 settings entries
    _ui_Draw_MenuListValue(ui_state, ui_state->entrySelected,
//                           _ui_getModule17EntryName , _ui_getModule17ValueName);
// not provided for as it's not provided for in the compiled version
                           PAGE_SETTINGS_M17 , PAGE_SETTINGS_M17);
}

void _ui_Draw_SettingsReset2Defaults(UI_State_st* ui_state)
{
    (void) ui_state;

    static int drawcnt = 0;
    static long long lastDraw = 0;
    Color_st color_fg ;
    ui_ColorLoad( &color_fg , COLOR_FG );

    gfx_clearScreen();
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, ALIGN_CENTER,
              color_fg, "Reset to Defaults");

    // Make text flash yellow once every 1s
    Color_st textcolor = drawcnt % 2 == 0 ? color_fg : yellow_fab413;
    gfx_printLine(1, 4, layout.lines[ GUI_LINE_TOP ].height, SCREEN_HEIGHT - layout.lines[ GUI_LINE_BOTTOM ].height,
                  layout.horizontal_pad, layout.lines[ GUI_LINE_TOP ].font,
                  ALIGN_CENTER, textcolor, "To reset:");
    gfx_printLine(2, 4, layout.lines[ GUI_LINE_TOP ].height, SCREEN_HEIGHT - layout.lines[ GUI_LINE_BOTTOM ].height,
                  layout.horizontal_pad, layout.lines[ GUI_LINE_TOP ].font,
                  ALIGN_CENTER, textcolor, "Press Enter twice");

    if((getTick() - lastDraw) > 1000)
    {
        drawcnt++;
        lastDraw = getTick();
    }
}
