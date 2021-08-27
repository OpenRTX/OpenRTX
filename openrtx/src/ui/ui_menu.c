/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ui.h>
#include <interfaces/nvmem.h>
#include <interfaces/platform.h>

#include <maidenhead.h>
extern void _ui_drawMainBackground();
extern void _ui_drawMainTop();
extern void _ui_drawBottom();

#include <satellite.h>
#include <math.h>

#include <stdlib.h>

void _ui_drawMenuList(uint8_t selected, int (*getCurrentEntry)(char *buf, uint8_t max_len, uint8_t index))
{
    point_t pos = layout.line1_pos;
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen = (SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    color_t text_color = color_white;
    for(int item=0, result=0; (result == 0) && (pos.y < SCREEN_HEIGHT); item++)
    {
        // If selection is off the screen, scroll screen
        if(selected >= entries_in_screen)
            scroll = selected - entries_in_screen + 1;
        // Call function pointer to get current menu entry string
        result = (*getCurrentEntry)(entry_buf, sizeof(entry_buf), item+scroll);
        if(result != -1)
        {
            text_color = color_white;
            if(item + scroll == selected)
            {
                text_color = color_black;
                // Draw rectangle under selected item, compensating for text height
                point_t rect_pos = {0, pos.y - layout.menu_h + 3};
                gfx_drawRect(rect_pos, SCREEN_WIDTH, layout.menu_h, color_white, true);
            }
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_LEFT, text_color, entry_buf);
            pos.y += layout.menu_h;
        }
    }
}

void _ui_drawMenuListValue(ui_state_t* ui_state, uint8_t selected,
                           int (*getCurrentEntry)(char *buf, uint8_t max_len, uint8_t index),
                           int (*getCurrentValue)(char *buf, uint8_t max_len, uint8_t index))
{
    point_t pos = layout.line1_pos;
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen = (SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    char value_buf[MAX_ENTRY_LEN] = "";
    color_t text_color = color_white;
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
            text_color = color_white;
            if(item + scroll == selected)
            {
                // Draw rectangle under selected item, compensating for text height
                // If we are in edit mode, draw a hollow rectangle
                text_color = color_black;
                bool full_rect = true;
                if(ui_state->edit_mode)
                {
                    text_color = color_white;
                    full_rect = false;
                }
                point_t rect_pos = {0, pos.y - layout.menu_h + 3};
                gfx_drawRect(rect_pos, SCREEN_WIDTH, layout.menu_h, color_white, full_rect);
            }
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_LEFT, text_color, entry_buf);
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_RIGHT, text_color, value_buf);
            pos.y += layout.menu_h;
        }
    }
}

int _ui_getMenuTopEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= menu_num) return -1;
    snprintf(buf, max_len, "%s", menu_items[index]);
    return 0;
}

int _ui_getSettingsEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_num) return -1;
    snprintf(buf, max_len, "%s", settings_items[index]);
    return 0;
}

int _ui_getDisplayEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= display_num) return -1;
    snprintf(buf, max_len, "%s", display_items[index]);
    return 0;
}

int _ui_getDisplayValueName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= display_num) return -1;
    uint8_t value = 0;
    switch(index)
    {
        case D_BRIGHTNESS:
            value = last_state.settings.brightness;
            break;
#ifdef SCREEN_CONTRAST
        case D_CONTRAST:
            value = last_state.settings.contrast;
            break;
#endif
    }
    snprintf(buf, max_len, "%d", value);
    return 0;
}

#ifdef HAS_GPS
int _ui_getSettingsGPSEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= settings_gps_num) return -1;
    snprintf(buf, max_len, "%s", settings_gps_items[index]);
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
            snprintf(buf, max_len, "%s", (last_state.settings.gps_set_time) ? "ON" : "OFF");
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
    snprintf(buf, max_len, "%s", info_items[index]);
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
        case 1: // Battery voltage
        {
            // Compute integer part and mantissa of voltage value, adding 50mV
            // to mantissa for rounding to nearest integer
            uint16_t volt  = last_state.v_bat / 1000;
            uint16_t mvolt = ((last_state.v_bat - volt * 1000) + 50) / 100;
            snprintf(buf, max_len, "%d.%dV", volt, mvolt);
        }
            break;
        case 2: // Battery charge
            snprintf(buf, max_len, "%d%%", last_state.charge);
            break;
        case 3: // RSSI
            snprintf(buf, max_len, "%.1fdBm", last_state.rssi);
            break;
        case 4: // Model
            snprintf(buf, max_len, "%s", hwinfo->name);
            break;
        case 5: // Band
            snprintf(buf, max_len, "%s %s", hwinfo->vhf_band ? "VHF" : "", hwinfo->uhf_band ? "UHF" : "");
            break;
        case 6: // VHF
            snprintf(buf, max_len, "%d - %d", hwinfo->vhf_minFreq, hwinfo->vhf_maxFreq);
            break;
        case 7: // UHF
            snprintf(buf, max_len, "%d - %d", hwinfo->uhf_minFreq, hwinfo->uhf_maxFreq);
            break;
        case 8: // LCD Type
            snprintf(buf, max_len, "%d", hwinfo->lcd_type);
            break;
    }
    return 0;
}

int _ui_getZoneName(char *buf, uint8_t max_len, uint8_t index)
{
    int result = 0;
    // First zone "All channels" is not read from flash
    if(index == 0)
    {
        snprintf(buf, max_len, "All channels");
    }
    else
    {
        zone_t zone;
        result = nvm_readZoneData(&zone, index);
        if(result != -1)
            snprintf(buf, max_len, "%s", zone.name);
    }
    return result;
}

int _ui_getChannelName(char *buf, uint8_t max_len, uint8_t index)
{
    channel_t channel;
    int result = nvm_readChannelData(&channel, index + 1);
    if(result != -1)
        snprintf(buf, max_len, "%s", channel.name);
    return result;
}

int _ui_getContactName(char *buf, uint8_t max_len, uint8_t index)
{
    contact_t contact;
    int result = nvm_readContactData(&contact, index + 1);
    if(result != -1)
        snprintf(buf, max_len, "%s", contact.name);
    return result;
}

void _ui_drawMenuTop(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Menu" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Menu");
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getMenuTopEntryName);
}

void _ui_drawMenuZone(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Zone" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Zone");
    // Print zone entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getZoneName);
}

void _ui_drawMenuChannel(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Channel" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Channels");
    // Print channel entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getChannelName);
}

void _ui_drawMenuContacts(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Contacts" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Contacts");
    // Print contact entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getContactName);
}

#ifdef HAS_GPS
void _ui_drawMenuGPS()
{
    char *fix_buf, *type_buf;
    gfx_clearScreen();
    // Print "GPS" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "GPS");
    point_t fix_pos = {layout.line2_pos.x, SCREEN_HEIGHT * 2 / 5};
    // Print GPS status, if no fix, hide details
    if(!last_state.settings.gps_enabled)
        gfx_print(fix_pos, layout.line3_font, TEXT_ALIGN_CENTER,
                  color_white, "GPS OFF");
    else if (last_state.gps_data.fix_quality == 0)
        gfx_print(fix_pos, layout.line3_font, TEXT_ALIGN_CENTER,
                  color_white, "No Fix");
    else if (last_state.gps_data.fix_quality == 6)
        gfx_print(fix_pos, layout.line3_font, TEXT_ALIGN_CENTER,
                  color_white, "Fix Lost");
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
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, fix_buf);
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, "N     ");
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "%8.6f", last_state.gps_data.latitude);
        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, type_buf);
        // Convert from signed longitude, to unsigned + direction
        float longitude = last_state.gps_data.longitude;
        char *direction = (longitude < 0) ? "W     " : "E     ";
        longitude = (longitude < 0) ? -longitude : longitude;
        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, direction);
        gfx_print(layout.line2_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "%8.6f", longitude);
        gfx_print(layout.bottom_pos, layout.bottom_font, TEXT_ALIGN_CENTER,
                  color_white, "S %4.1fkm/h  A %4.1fm",
                  last_state.gps_data.speed,
                  last_state.gps_data.altitude);
    }
    // Draw compass
    point_t compass_pos = {layout.horizontal_pad * 2, SCREEN_HEIGHT / 2};
    gfx_drawGPScompass(compass_pos,
                       SCREEN_WIDTH / 9 + 2,
                       last_state.gps_data.tmg_true,
                       last_state.gps_data.fix_quality != 0 &&
                       last_state.gps_data.fix_quality != 6);
    // Draw satellites bar graph
    point_t bar_pos = {layout.line3_pos.x + SCREEN_WIDTH * 1 / 3, SCREEN_HEIGHT / 2};
    gfx_drawGPSgraph(bar_pos,
                     (SCREEN_WIDTH * 2 / 3) - layout.horizontal_pad,
                     SCREEN_HEIGHT / 3,
                     last_state.gps_data.satellites,
                     last_state.gps_data.active_sats);
}
int _ui_getSatelliteName(char *buf, uint8_t max_len, uint8_t index)
{
    int result = 0;

    if(index == 0)
    {
        snprintf(buf, max_len, "Current Sky");
    }
    else
    {
        if( index -1 < num_satellites ){ 
            //index 0 is actually a special value
            //index 1 is actually the first sat in the array
            sat_mem_t selected = satellites[index-1];
            sat_pos_t sat = calcSatNow( selected.tle, last_state );
            snprintf(buf, max_len, "% 6.1f   %s", sat.elev, selected.name);
        } else {
            result = -1;
        }
    }
    return result;
}
void _ui_drawMenuSatChoose(ui_state_t * ui_state){
    gfx_clearScreen();
    gfx_print(layout.top_pos, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white, "Satellite Tracking");
    // Print zone entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getSatelliteName);
}

unsigned long long getTick(void);
void _ui_drawMenuSatPredict(ui_state_t* ui_state){
    //actually, it's "asteroids"
    //this is a single game frame
    static unsigned long long last_t = 0;
    static game_obj_2d_t me = {0};
    static game_obj_2d_t asteroids[3] = {0};
    static int frame = 0;
    unsigned long long t = getTick();
    unsigned long long td = t - last_t;
    point_t center = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2};
    /*printf("TD: %lu \n", td);*/
    if( last_t == 0 ){
        //init
        //everything should already be init'd to zero
        /*printf("init\n");*/
        me.x = center.x;
        me.y = center.y;
        me.rot = RAD(-45);
        game_obj_init( &asteroids[0] );
        game_obj_init( &asteroids[1] );
        game_obj_init( &asteroids[2] );
        
    } else {
        //regular call
        game_move(&me, td);
        if( ui_state->keys & KEY_4 ){
            me.rot -= RAD(2);
        }
        if( ui_state->keys & KEY_6 ){
            me.rot += RAD(2);
        }
        if( ui_state->keys & KEY_5 ){
            game_addvel(&me, 1, me.rot);
        }
        if( ui_state->keys & KEY_ENTER ){
            //fire
        }
        game_obj_screenwrap(&me);
        for( int i = 0; i < 3; i++ ){
            game_move(&asteroids[i], td);
            game_obj_screenwrap(&asteroids[i]);
        }
    }
    point_t mepos = {me.x, me.y};
    fflush(stdout);

    gfx_clearScreen();
    gfx_drawDeltaArrow(mepos, 8, DEG(me.rot)+90, yellow_fab413);
    if( ui_state->keys & KEY_5 ){
        //draw rocket exhaust
    }
    for( int i = 0; i < 3; i++ ){
        point_t apos = {asteroids[i].x, asteroids[i].y};
        gfx_drawCircle(apos, 4, color_white);
    }

    frame++;

    last_t = t;
}
void _ui_drawMenuSatSkyView(ui_state_t * ui_state, int whichsat, int passidx, int drawstars ){
    //whichsat == 0 means all sats (so you can pass in menu_selected directly)
    //whichpass == 0 means first pass
    //(this is partly because pass selection is not menu based, but based around custom inputs)
    topo_pos_t obs = getObserverPosition();
    double jd = curTime_to_julian_day(last_state.time);
    
   
    //handle zoom and pan events here
    //+- radius for zoom, +- plot_center for pan
    //0 means reset
    //
    //normalize this away from pixels 
    //and store the zoom coords not as a modification of plot center
    //but as an azel to center! that way we can zoom in on whats under our viewport
    //without things sliding away on us
    static point_t viewport_center = {0, 0};
    int screen_scale = SCREEN_HEIGHT/2;
    static int radius = SCREEN_WIDTH/2/1.5;
    static int snap_item = -1;
    point_t plot_center = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2+5};


    //TODO draw scale in degrees/arcseconds, etc
    //90/radius is degrees per pixel
    //debug prints
    //gfx_print(layout.bottom_pos, FONT_SIZE_5PT, TEXT_ALIGN_RIGHT, color_grey, "r=%dpx",radius );
    float deg_per_pix = 90.0 / radius;
    float pix_per_deg = radius/90.0;
    gfx_print(layout.bottom_pos, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, color_grey, "1px=%f deg", deg_per_pix);
    //gfx_print(layout.bottom_pos, FONT_SIZE_5PT, TEXT_ALIGN_RIGHT, color_grey, "trk%d", snap_item);

    if( ui_state->keys & KEY_0 ){
        radius = SCREEN_WIDTH/2/1.5;
        viewport_center.x = 0;
        viewport_center.y = 0;
    }
    //the more zoomed in we are, the more things i want to print for each satellite or star
    //az, el, speed, next pass, etc
    //as we zoom in a LOT and get more space
    //want to make the font larger too
    //
    if( ui_state->keys & KEY_UP ){
        radius *= 1.2;
        viewport_center.x *= 1.2;
        viewport_center.y *= 1.2;
    }
    if( ui_state->keys & KEY_DOWN ){
        radius /= 1.2;
        viewport_center.x /= 1.2;
        viewport_center.y /= 1.2;
    }
    if( ui_state->keys & KEY_2 ){
        viewport_center.y += fmax(screen_scale * 10/ radius, 1);
    }
    if( ui_state->keys & KEY_8 ){
        viewport_center.y -= fmax(screen_scale * 10/ radius, 1);
    }
    if( ui_state->keys & KEY_4 ){
        viewport_center.x += fmax(screen_scale * 10/ radius, 1);
    }
    if( ui_state->keys & KEY_6 ){
        viewport_center.x -= fmax(screen_scale * 10/ radius, 1);
    }
    if( ui_state->keys & KEY_1 ){
        snap_item++; //continued in whichsat == 0 section below!
    }
    plot_center = offset_point( plot_center, 1, viewport_center);
    
    gfx_drawPolarAzElPlot( plot_center, radius, color_grey );
    
    /*int retrograde = degrees(tle->xincl) > 90;*/
    //if( retrograde ) satellite passes east to west
    //else it passes west to east
    
    if( whichsat == 0 ){
        //draw current positions of all known sats
        for( int i = 0; i < num_satellites; i++){
            point_t temppos = {0,0};
            sat_mem_t selected = satellites[ i ]; 
            double jd_1s = 1.0 / 86400; //1 second in decimal days
            sat_pos_t sat = calcSat( selected.tle, jd, obs);
            if( sat.elev < -15 ){
                continue;
            }
            gfx_drawPolar( plot_center, radius, sat.az, sat.elev, '+', yellow_fab413 );


            //draws a little trail of where it was two times in the past
            //hoping that shows motion well enough, with the fab413 -> white -> grey indicating age of data
            //ideally we'd save a number of track histories (with decreasing granularity as data ages)
            //and just draw those, rather than recalculating old stuff
            //...but that's a TODO later maybe, we'll see how this goes
            sat_pos_t sat_p2 = calcSat( selected.tle, jd-60*jd_1s, obs);
            sat_pos_t sat_p1 = calcSat( selected.tle, jd-30*jd_1s, obs);
            gfx_drawPolar( plot_center, radius, sat_p2.az, sat_p2.elev, 0, color_grey );
            gfx_drawPolar( plot_center, radius, sat_p1.az, sat_p1.elev, 0, color_white );


            point_t pos = azel_deg_to_xy( sat.az, sat.elev, radius);
            if( snap_item == i ){
                viewport_center.x = -1*pos.x;
                viewport_center.y = -1*pos.y;
                //mult -1 because the viewport gets shifted away in the opposite direction
                //(if you want the (1,1) corner of a piece of paper to
                //be centered on your desk when the center of the desk and
                //center of the paper are in the same place, you have to
                //move the paper in the (-1,-1) direction
                gfx_print(layout.bottom_pos, FONT_SIZE_5PT, TEXT_ALIGN_RIGHT, color_white, "TRK: %s", selected.name);
            }
            point_t text_offset = {6,6};

            temppos = offset_point( plot_center, 2, pos, text_offset);
            gfx_print(temppos, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, color_white,
                    "%s", selected.name);
        }
        if( snap_item >= num_satellites ){ //if snap_item is 1 and num_satellites is 1, zero indexing means we're past it and should reset already
            snap_item = -1; //reset
        }
    } else {
        (void) passidx;
        //draw a pass and current position for only a specific satellite
        sat_azel_t pass_azel[] = {
            {2459315.240168, 287.3, 1.2,2430.8},
            {2459315.240368, 288.7, 2.3,2327.1},{2459315.240568, 290.2, 3.4,2224.8},{2459315.240768,
            291.9, 4.6,2124.1},{2459315.240968, 293.7,
            5.9,2025.5},{2459315.241168, 295.8, 7.1,1929.2},{2459315.241368,
            298.0, 8.5,1835.6},{2459315.241568, 300.6,
            9.9,1745.2},{2459315.241768, 303.4, 11.3,1658.6},{2459315.241968,
            306.6, 12.8,1576.4},{2459315.242168, 310.1,
            14.3,1499.3},{2459315.242368, 314.1, 15.9,1428.2},{2459315.242568,
            318.5, 17.4,1364.1},{2459315.242768, 323.4,
            18.8,1308.1},{2459315.242968, 328.8, 20.1,1261.1},{2459315.243168,
            334.7, 21.3,1224.4},{2459315.243368, 341.0,
            22.1,1198.8},{2459315.243568, 347.5, 22.7,1185.1},{2459315.243768,
            354.2, 22.9,1183.6},{2459315.243968, 0.8,
            22.7,1194.5},{2459315.244168, 7.2, 22.1,1217.4},{2459315.244368,
            13.3, 21.3,1251.6},{2459315.244568, 18.9,
            20.2,1296.2},{2459315.244768, 24.0, 19.0,1350.2},{2459315.244968,
            28.6, 17.6,1412.5},{2459315.245168, 32.8,
            16.2,1482.0},{2459315.245368, 36.5, 14.8,1557.8},{2459315.245568,
            39.8, 13.4,1638.8},{2459315.245768, 42.8,
            12.0,1724.5},{2459315.245968, 45.4, 10.7,1814.0},{2459315.246168,
            47.8, 9.4,1906.8},{2459315.246368, 49.9,
            8.1,2002.5},{2459315.246568, 51.9, 6.9,2100.5},{2459315.246768,
            53.6, 5.8,2200.7},{2459315.246968, 55.2,
            4.7,2302.5},{2459315.247168, 56.6, 3.6,2405.9},{2459315.247368,
            58.0, 2.5,2510.5},{2459315.247568, 59.2,
            1.5,2616.3},{2459315.247768, 60.3, 0.6,2722.9},{2459315.247968,
            61.3, -0.4,2830.4},
        };
        int num_points_pass = sizeof(pass_azel) / sizeof(sat_azel_t);


        for( int i = 0; i < num_points_pass; i+=2 ){
            gfx_drawPolar( plot_center, radius, pass_azel[i].az, pass_azel[i].elev, 
                    0, //which means set a pixel, don't draw a character
                    color_white );
        }
        point_t rise_rel = azel_deg_to_xy( pass_azel[0].az, pass_azel[0].elev, radius);
        point_t left_text_offset = {-26, 3};
        point_t set_rel = azel_deg_to_xy( pass_azel[num_points_pass-1].az, pass_azel[num_points_pass-1].elev, radius);
        point_t right_text_offset = {9, 3};
        point_t rise = offset_point( plot_center, 2, rise_rel, left_text_offset );
        point_t set = offset_point( plot_center, 2, set_rel, right_text_offset );

        //at start and end points, print rise and set time
        gfx_print(rise, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, color_white,
                "%02d:%02d", 4, 53);

        gfx_print(set, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, color_white,
                "%02d:%02d", 4, 59);
                

        point_t line_offset_5pt = {0,8};
        point_t temppos = {0,0};

        temppos = offset_point( rise, 1, line_offset_5pt );
        gfx_print(temppos, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, color_white,
                "%.0f AZ", pass_azel[0].az);
                

        temppos = offset_point( set, 1, line_offset_5pt );
        gfx_print(temppos, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, color_white,
                "%.0f AZ", pass_azel[num_points_pass-1].az);
                
        
        int i = 0;
        double az = pass_azel[i].az;
        double elev = pass_azel[i].elev;
        gfx_drawPolar( plot_center, radius, az, elev, '+', yellow_fab413 );
    }


    if( drawstars ){
        
        for( int i = 0; i < num_stars; i+=1 ){
            double az, alt;
            double ra, dec;
            ra  = stars[i].ra*15; //in decimal hours, so *15->deg
            dec = stars[i].dec;
            ra_dec_to_az_alt(jd, RAD(obs.lat), RAD(obs.lon), RAD(ra), RAD(dec), &az, &alt);
            az = DEG(az);
            alt = DEG(alt);
            /*printf("%.1f %.1f %.0f %.0f\n", ra, dec, az, alt);*/
            if( alt < -2.5 ){ 
                //draw stars that are juuust below the horizon
                continue;
            }
            int brt_scale = 0xff;
            if( stars[i].mag > 5 && radius < screen_scale*3 ){
                //too dim, not zoomed in enough to see it
                continue; 
            }
            if( radius > screen_scale * 3){
                brt_scale <<= 2;
            }
            if( radius > screen_scale * 8){
                brt_scale <<= 2;
            }
            int brt = pow(2.51,-1*stars[i].mag)*brt_scale;
            if( brt > 0xff ) brt = 0xff;
            if( brt < 0x10 ) brt = 0x10;

            color_t clr = {0xff, 0xff, 0x00, brt};
            color_t clr2 = {0xff, 0xff, 0x00, 0x30};
            if( i < pow(log2(fmax(radius,58)-55),2) || radius > screen_scale * 12){ //just an eyeball magic number
                //the more zoomed in we are, the more names of stars we print!
                point_t relpos = azel_deg_to_xy( az, alt, radius);
                point_t offset = {3,3};
                fontSize_t ft= FONT_SIZE_5PT;
                point_t pos = offset_point( plot_center, 2, relpos, offset );
                gfx_print(pos, ft, TEXT_ALIGN_LEFT, clr2, stars[i].name);
            }
            gfx_drawPolar( plot_center, radius, az, alt, 0, clr );
        }
    }


}
void _ui_drawMenuSatPass(ui_state_t* ui_state){
    gfx_clearScreen();
    _ui_drawMainTop();
    int whichsat = ui_state->menu_selected;
    //TODO handle inputs to select pass
    _ui_drawMenuSatSkyView(ui_state, whichsat, 0, 1);
    return;
}


void _ui_drawMenuSatTrack(ui_state_t * ui_state)
{
    
    char gridsquare[7] = {0}; //we want to use this as a c-style string so that extra byte stays zero

    gfx_clearScreen();
    _ui_drawMainTop();
    topo_pos_t obs = getObserverPosition();
    double jd = curTime_to_julian_day(last_state.time);

    if( ui_state->menu_selected == 0 ){ 
        //first menu entry (0) is to show all satellites in sky view
        _ui_drawMenuSatSkyView(ui_state, 0, 0, 1);
        return;
    }
    _ui_drawMainBackground(); 
    //_ui_drawBottom();
    int idx = ui_state->menu_selected - 1; //because 0 is "all sats"
    sat_mem_t selected = satellites[ idx ]; 
    sat_pos_t sat = calcSat( selected.tle, jd, obs);
    //TODO recalculate when we re-enter from a different satellite entry
    static double tdiff = 0;
    static double nextpass = 0;
    if( tdiff <= 0 ){
        nextpass = sat_nextpass( selected.tle, 2459452.83, 0.5, obs);
    }
    tdiff = (nextpass - jd) * 24 * 60 * 60; //seconds
    gfx_print(layout.bottom_pos, FONT_SIZE_8PT, TEXT_ALIGN_LEFT, color_white,
            "AOS %ds", (int)tdiff);
    /*printf( "next pass %s JD: %lf\n", selected.name, nextpass);*/

    // left side
    // relative coordinates to satellite
    gfx_print(layout.line2_pos, FONT_SIZE_8PT, TEXT_ALIGN_LEFT, color_white,
            "AZ %.1f", sat.az);
            
    gfx_print(layout.line2_pos, FONT_SIZE_8PT, TEXT_ALIGN_RIGHT, color_white,
            "EL %.1f", sat.elev);
            
    gfx_print(layout.line1_pos, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white, selected.name);

    //right side
    //doppler correction readout
    /*snprintf(sbuf, 25, "%.1fk DOP", ((float)doppler_offset)/1000);*/
    /*gfx_print(layout.line1_pos, sbuf, FONT_SIZE_8PT, TEXT_ALIGN_RIGHT, color_white);*/
    //draw gridsquare text
    lat_lon_to_maidenhead(obs.lat, obs.lon, gridsquare, 3); //precision=3 here means 6 characters like FN41uq
    gfx_print(layout.line3_pos, FONT_SIZE_8PT, TEXT_ALIGN_LEFT, color_white, gridsquare);

    //center bottom - show 
    //satellite and AOS/LOS countdown 

    //draw Az/El
    int radius = SCREEN_WIDTH/2/4;
    point_t plot_center = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2+5};
    gfx_drawPolarAzElPlot( plot_center, radius, color_grey );
    /*for( int i = 0; i < num_points_pass; i+=2 ){*/
        /*gfx_drawPolar( plot_center, radius, pass_azel[i].az,*/
            /*pass_azel[i].elev, 0, color_white );*/
    /*}*/
    gfx_drawPolar( plot_center, radius, sat.az, sat.elev, '+', yellow_fab413 );

    /*
    char * pass_state;
    double mark;
    double pass_start_jd = 0; //pass_azel[0].jd; 
    double pass_end_jd = 0; //pass_azel[num_points_pass-1].jd; 
    //mark is the time we care about most - e.g. before a pass, it's the time the pass starts
    //or during a pass, it's the LOS time (when the sat will go below the horizon)
    if( jd < pass_start_jd ){
        //before this pass comes over the horizon
        pass_state = "AOS";
        mark = pass_start_jd;
    } else if ( jd > pass_start_jd && jd < pass_end_jd ){
        //during the pass
        pass_state = "LOS";
        mark = pass_end_jd;
    } else {
        //now it's gone over the horizon, so same as the elif above (just will be a
        //negative number to show it was in the past)
        //left here for clarity to show the actual LOS condition
        pass_state = "LOS"; 
        mark = pass_end_jd;
    }
    float diff = (mark - jd)*86400; //diff is seconds until (+) or since (-) the mark timestamp
    snprintf(sbuf, 25, "%s %s %.0fs", sat_name, pass_state, diff);
    gfx_print(layout.line3_pos, sbuf, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white);
    */
}
#endif

void _ui_drawMenuSettings(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Settings");
    // Print menu entries
    _ui_drawMenuList(ui_state->menu_selected, _ui_getSettingsEntryName);
}

void _ui_drawMenuInfo(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Info" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Info");
    // Print menu entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getInfoEntryName,
                           _ui_getInfoValueName);
}

void _ui_drawMenuAbout()
{
    gfx_clearScreen();
    point_t openrtx_pos = {layout.horizontal_pad, layout.line3_h};
    if(SCREEN_HEIGHT >= 100)
        ui_drawSplashScreen(false);
    else
        gfx_print(openrtx_pos, layout.line3_font, TEXT_ALIGN_CENTER,
                  color_white, "OpenRTX");
    uint8_t line_h = layout.menu_h;
    point_t pos = {SCREEN_WIDTH / 7, SCREEN_HEIGHT - (line_h * (author_num - 1)) - 5};
    for(int author = 0; author < author_num; author++)
    {
        gfx_print(pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, "%s", authors[author]);
        pos.y += line_h;
    }
}

void _ui_drawSettingsDisplay(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "Display" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Display");
    // Print display settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getDisplayEntryName,
                           _ui_getDisplayValueName);
}

#ifdef HAS_GPS
void _ui_drawSettingsGPS(ui_state_t* ui_state)
{
    gfx_clearScreen();
    // Print "GPS Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "GPS Settings");
    // Print display settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected,
                          _ui_getSettingsGPSEntryName,
                          _ui_getSettingsGPSValueName);
}
#endif

#ifdef HAS_RTC
void _ui_drawSettingsTimeDate()
{
    gfx_clearScreen();
    curTime_t local_time = state_getLocalTime(last_state.time);
    // Print "Time&Date" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Time&Date");
    // Print current time and date
    gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, "%02d/%02d/%02d",
              local_time.date, local_time.month, local_time.year);
    gfx_print(layout.line3_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, "%02d:%02d:%02d",
              local_time.hour, local_time.minute, local_time.second);
}

void _ui_drawSettingsTimeDateSet(ui_state_t* ui_state)
{
    (void) last_state;

    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Time&Date");
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
    gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, ui_state->new_date_buf);
    gfx_print(layout.line3_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, ui_state->new_time_buf);
}
#endif

bool _ui_drawMacroMenu() {
        // Header
        gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, "Macro Menu");
        // First row
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413, "1");
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, "  %6.1f",
                  ctcss_tone[last_state.channel.fm.txTone]/10.0f);
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, "2       ");
        char encdec_str[9] = { 0 };
        bool tone_tx_enable = last_state.channel.fm.txToneEn;
        bool tone_rx_enable = last_state.channel.fm.rxToneEn;
        if (tone_tx_enable && tone_rx_enable)
            snprintf(encdec_str, 9, "     E+D");
        else if (tone_tx_enable && !tone_rx_enable)
            snprintf(encdec_str, 9, "      E ");
        else if (!tone_tx_enable && tone_rx_enable)
            snprintf(encdec_str, 9, "      D ");
        else
            snprintf(encdec_str, 9, "        ");
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, encdec_str);
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413, "3        ");
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "%.1gW", last_state.channel.power);
        // Second row
        // Calculate symmetric second row position, line2_pos is asymmetric like main screen
        point_t pos_2 = {layout.line1_pos.x, layout.line1_pos.y +
                        (layout.line3_pos.y - layout.line1_pos.y)/2};
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413, "4");
        char bw_str[8] = { 0 };
        switch (last_state.channel.bandwidth)
        {
            case BW_12_5:
                snprintf(bw_str, 8, "   12.5");
                break;
            case BW_20:
                snprintf(bw_str, 8, "     20");
                break;
            case BW_25:
                snprintf(bw_str, 8, "     25");
                break;
        }
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, bw_str);
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, "5       ");
        char mode_str[9] = "";
        switch(last_state.channel.mode)
        {
            case FM:
            snprintf(mode_str, 9,"      FM");
            break;
            case DMR:
            snprintf(mode_str, 9,"     DMR");
            break;
        }
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, mode_str);
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413, "6        ");
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "Lck");
        // Third row
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413, "7");
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, "    B+");
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, "8       ");
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, "     B-");
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413, "9        ");
        gfx_print(layout.line3_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "Sav");
        // Smeter bar
        float rssi = last_state.rssi;
        float squelch = last_state.sqlLevel / 16.0f;
        point_t smeter_pos = { layout.horizontal_pad,
                               layout.bottom_pos.y +
                               layout.status_v_pad +
                               layout.text_v_offset -
                               layout.bottom_h };
        gfx_drawSmeter(smeter_pos,
                       SCREEN_WIDTH - 2 * layout.horizontal_pad,
                       layout.bottom_h - 1,
                       rssi,
                       squelch,
                       yellow_fab413);

        return true;
}
