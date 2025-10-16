/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "interfaces/cps_io.h"
#include <stdio.h>
#include <stdint.h>
#include "ui/ui_mod17.h"
#include <string.h>

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

                if(rtxStatus.M17_link[0] != '\0')
                {
                    gfx_drawSymbol(layout.line4_pos, layout.line3_symbol_font, TEXT_ALIGN_LEFT,
                                color_white, SYMBOL_ACCESS_POINT);
                    gfx_print(layout.line4_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                            color_white, "%s", rtxStatus.M17_link);
                }

                if(rtxStatus.M17_refl[0] != '\0')
                {
                    gfx_drawSymbol(layout.line3_pos, layout.line4_symbol_font, TEXT_ALIGN_LEFT,
                                   color_white, SYMBOL_NETWORK);
                    gfx_print(layout.line3_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                              color_white, "%s", rtxStatus.M17_refl);
                }
            }
            else
            {
                char *dst = NULL;
                char *last = NULL;

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
