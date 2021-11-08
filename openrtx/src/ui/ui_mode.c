/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <ui.h>
#include <string.h>
#include <rtx.h>

/* UI main screen helper functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainTop();
extern void _ui_drawMainBottom();

void _ui_drawModeVFOFreq()
{
    // Print VFO RX Frequency on line 1 of 3
    gfx_printLine(1, 3, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, 
                  layout.horizontal_pad, layout.mode_font_big, 
                  TEXT_ALIGN_CENTER, color_white, "%03lu.%05lu",
                  (unsigned long)last_state.channel.rx_frequency/1000000,
                  (unsigned long)last_state.channel.rx_frequency%1000000/10);
}

void _ui_drawMEMChannel()
{
    // Print Channel name on line 1 of 3
    gfx_printLine(1, 3, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, 
                  layout.horizontal_pad, layout.mode_font_small, 
                  TEXT_ALIGN_CENTER, color_white, "%03d: %.12s", 
                  last_state.channel_index, last_state.channel.name);
}

void _ui_drawModeDetails(ui_state_t* ui_state)
{
    char bw_str[8] = { 0 };
    char encdec_str[9] = { 0 };
        
    rtxStatus_t cfg = rtx_getCurrentStatus();
    
    switch(last_state.channel.mode)
    {
        case OPMODE_FM:
        // Get Bandwith string
        if(last_state.channel.bandwidth == BW_12_5)
            snprintf(bw_str, 8, "12.5");
        else if(last_state.channel.bandwidth == BW_20)
            snprintf(bw_str, 8, "20");
        else if(last_state.channel.bandwidth == BW_25)
            snprintf(bw_str, 8, "25");
        // Get encdec string
        bool tone_tx_enable = last_state.channel.fm.txToneEn;
        bool tone_rx_enable = last_state.channel.fm.rxToneEn;
        if (tone_tx_enable && tone_rx_enable)
            snprintf(encdec_str, 9, "E+D");
        else if (tone_tx_enable && !tone_rx_enable)
            snprintf(encdec_str, 9, "E");
        else if (!tone_tx_enable && tone_rx_enable)
            snprintf(encdec_str, 9, "D");
        else
            snprintf(encdec_str, 9, " ");

        // Print Bandwidth info
        gfx_printLine(2, 3, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, 
                      layout.horizontal_pad, layout.mode_font_small,
                      TEXT_ALIGN_LEFT, color_white, "BW:%s", bw_str);
        // Print Tone and encdec info
        gfx_printLine(3, 3, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, 
                      layout.horizontal_pad, layout.mode_font_small, 
                      TEXT_ALIGN_LEFT, color_white, "T:%4.1f   S:%s",
                      ctcss_tone[last_state.channel.fm.txTone]/10.0f,
                      encdec_str);
        break;
        case OPMODE_DMR:
        // Print talkgroup on line 2 of 3
        gfx_printLine(2, 3, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, 
                      layout.horizontal_pad, layout.mode_font_small,
                      TEXT_ALIGN_LEFT, color_white, "TG:");
        // Print User ID on line 3 of 3
        gfx_printLine(3, 3, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, 
                      layout.horizontal_pad, layout.mode_font_small, 
                      TEXT_ALIGN_LEFT, color_white, "ID:");
        break;
        case OPMODE_M17:
        // Print M17 Source ID on line 2 of 3
        gfx_printLine(2, 3, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, 
                      layout.horizontal_pad, layout.mode_font_small, 
                      TEXT_ALIGN_LEFT, color_white, "Src ID: %s", cfg.source_address);
        // Print M17 Destination ID on line 3 of 3
        if(ui_state->edit_mode)
        {
            gfx_printLine(3, 3, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, 
                          layout.horizontal_pad, layout.mode_font_small, 
                          TEXT_ALIGN_LEFT, color_white, "Dst ID: %s_", ui_state->new_callsign);
        }
        else
        {
            gfx_printLine(3, 3, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, 
                          layout.horizontal_pad, layout.mode_font_small, 
                          TEXT_ALIGN_LEFT, color_white, "Dst ID: %s", cfg.destination_address);
        }
        break;
    }
}

void _ui_drawModeVFO(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop();
    _ui_drawModeVFOFreq();
    _ui_drawModeDetails(ui_state);
    _ui_drawMainBottom();
}

void _ui_drawModeMEM(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop();
    _ui_drawMEMChannel();
    _ui_drawModeDetails(ui_state);
    _ui_drawMainBottom();
}
