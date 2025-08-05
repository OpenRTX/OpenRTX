/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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
 *                                                                         *
 *   (2025) Modified by KD0OSS for new modes on Module17                   *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/cps_io.h>
#include <interfaces/delays.h>
#include <stdio.h>
#include <stdint.h>
#include <ui/ui_mod17.h>
#include <string.h>

#include <unistd.h>
#include <stdlib.h>


static char message[54];
static bool scrollStarted = false;

// shift string to left by one char
// putting first char to the back
bool _ui_scrollString(char *string, bool reset)
{
    uint8_t stringLen = 0;
    char *in = NULL;

    if(reset)
    {
        // add extra space to allow for gap
        // between last char and first
        stringLen = strlen(string)+2;
        in = malloc(stringLen);
        if(in == NULL) return false;
        memset(in, 0, stringLen);
    }
    else
    {
        stringLen = strlen(string)+1;
        in = malloc(stringLen);
    }
    if(in == NULL) return false;
    strcpy(in, string);
    if(reset)
    {
        // add space to end
        in[strlen(string)] = 32;
    }

    char *data = malloc(stringLen);
    if(data == NULL)
    {
        free(in);
        return false;
    }
    memset(data, 0, stringLen);

    size_t i;
    for(i=1;i<strlen(in);i++)
        data[i-1] = in[i];
    data[i-1] = in[0];

    strcpy(string, data);
    free(data);
    free(in);

    return true;
}

void _ui_drawMainBackground()
{
    // Print top bar line of hline_h pixel height
    gfx_drawHLine(layout.top_h, layout.hline_h, color_grey);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(CONFIG_SCREEN_HEIGHT - layout.bottom_h - 1, layout.hline_h, color_grey);
}

void _ui_drawMainTop()
{
#ifdef CONFIG_RTC
    // Print clock on top bar
    datetime_t local_time = utcToLocalTime(last_state.time,
                                           last_state.settings.utc_timezone);
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "%02d:%02d:%02d", local_time.hour,
              local_time.minute, local_time.second);
#endif

    // Print the source callsign on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, state.settings.callsign);
}

static void _ui_drawModeInfo(ui_state_t* ui_state)
{
    switch(last_state.channel.mode)
    {
        case OPMODE_M17:
        {
            rtxStatus_t rtxStatus = rtx_getCurrentStatus();

            if(rtxStatus.lsfOk)
            {
                gfx_drawSymbol(layout.line2_pos, layout.line2_symbol_font, TEXT_ALIGN_LEFT,
                               color_white, SYMBOL_CALL_RECEIVED);
                gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, "%s", rtxStatus.M17_dst);
                gfx_drawSymbol(layout.line1_pos, layout.line1_symbol_font, TEXT_ALIGN_LEFT,
                               color_white, SYMBOL_CALL_MADE);
                gfx_print(layout.line1_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, "%s", rtxStatus.M17_src);

                // metatext available
                if(strlen(rtxStatus.M17_Meta_Text) > 20)
                {
                    if(!scrollStarted)
                    {
                        strcpy(message, rtxStatus.M17_Meta_Text);
                        _ui_scrollString(message, true);
                        scrollStarted = true;
                    }
                    _ui_scrollString(message, false);

                    char msg[21];
                    strncpy(msg, message, 20);
                    gfx_print(layout.line5_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                              color_white, "%s", msg);
                    sleepFor(0, 100);
                }
                else
                    if(strlen(rtxStatus.M17_Meta_Text) > 0)
                        gfx_print(layout.line5_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                                  color_white, "%s", rtxStatus.M17_Meta_Text);

                        if(rtxStatus.M17_link[0] != '\0')
                        {
                            gfx_drawSymbol(layout.line4_pos, layout.line4_symbol_font, TEXT_ALIGN_LEFT,
                                           color_white, SYMBOL_ACCESS_POINT);
                            gfx_print(layout.line4_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                                      color_white, "%s", rtxStatus.M17_link);
                        }

                        if(rtxStatus.M17_refl[0] != '\0')
                        {
                            gfx_drawSymbol(layout.line3_pos, layout.line3_symbol_font, TEXT_ALIGN_LEFT,
                                           color_white, SYMBOL_NETWORK);
                            gfx_print(layout.line3_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                                      color_white, "%s", rtxStatus.M17_refl);
                        }
            }
            else
            {
                char *dst = NULL;
                char *last = NULL;
                scrollStarted = false;

                if(ui_state->edit_mode)
                {
                    dst = ui_state->new_callsign;
                }
                else
                {
                    if(strnlen(rtxStatus.destination_address, 10) == 0)
                        dst = "--";
                    else
                        dst = rtxStatus.destination_address;
                }

                if(strnlen(rtxStatus.M17_src, 10) == 0)
                    last = "LAST";
                else
                    last = rtxStatus.M17_src;

                // Show envelope if SMS received
                if(state.totalSMSMessages > 0)
                    gfx_drawSymbol(layout.top_pos, layout.top_symbol_font, TEXT_ALIGN_CENTER,
                                   color_white, SYMBOL_MAIL);
                // Print CAN
                gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                          color_white, "CAN %02d", state.settings.m17_can);
                gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, last);
                // Print M17 Destination ID on line 2
                gfx_print(layout.line3_pos, layout.line3_font, TEXT_ALIGN_CENTER,
                          color_white, "%s", dst);
                if (ui_state->edit_mode)
                {
                    // Print Button Info
                    gfx_print(layout.line5_pos, layout.line5_font, TEXT_ALIGN_LEFT,
                              color_white, "Cancel");
                    gfx_print(layout.line5_pos, layout.line5_font, TEXT_ALIGN_RIGHT,
                              color_white, "Accept");
                }
                else
                {
                    // Menu
                    gfx_print(layout.line5_pos, layout.line5_font, TEXT_ALIGN_RIGHT,
                              color_white, "Menu");
                }
                break;
            }
        }
    }
}


void _ui_drawMainVFO(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop();
    _ui_drawModeInfo(ui_state);
}
