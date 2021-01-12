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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <ui.h>
#include <string.h>

void _ui_drawMainBackground()
{
    // Print top bar line of hline_h pixel height
    gfx_drawHLine(layout.top_h, layout.hline_h, color_grey);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(SCREEN_HEIGHT - layout.bottom_h - 1, layout.hline_h, color_grey);
    // Print transparent OPNRTX on the background
    point_t splash_origin = {0, SCREEN_HEIGHT / 2 - 6};
    color_t yellow = yellow_fab413;
    yellow.alpha = 0.1f * 255; 
    gfx_print(splash_origin, "O P N\nR T X", FONT_SIZE_12PT, TEXT_ALIGN_CENTER,
              yellow);
}

void _ui_drawMainTop(state_t* last_state)
{
#ifdef HAS_RTC
    // Print clock on top bar
    char clock_buf[9] = "";
    snprintf(clock_buf, sizeof(clock_buf), "%02d:%02d:%02d", last_state->time.hour,
             last_state->time.minute, last_state->time.second);
    gfx_print(layout.top_left, clock_buf, layout.top_font, TEXT_ALIGN_CENTER,
              color_white);
#endif

    // Print battery icon on top bar, use 4 px padding
    uint16_t bat_width = SCREEN_WIDTH / 9;
    uint16_t bat_height = layout.top_h - (layout.status_v_pad * 2);
    point_t bat_pos = {SCREEN_WIDTH - bat_width - layout.horizontal_pad,
                       layout.status_v_pad};
    gfx_drawBattery(bat_pos, bat_width, bat_height, last_state->charge);

    // Print radio mode on top bar
    char mode[4] = "";
    switch(last_state->channel.mode)
    {
        case FM:
        strcpy(mode, "FM");
        break;
        case DMR:
        strcpy(mode, "DMR");
        break;
    }
    gfx_print(layout.top_left, mode, layout.top_font, TEXT_ALIGN_LEFT,
              color_white);
}

void _ui_drawVFOMiddle(state_t* last_state)
{
    // Print VFO frequencies
    char freq_buf[20] = "";
    snprintf(freq_buf, sizeof(freq_buf), " Rx:%03u.%05u",
             last_state->channel.rx_frequency/1000000,
             last_state->channel.rx_frequency%1000000/10);
    gfx_print(layout.line2_left, freq_buf, layout.line2_font, TEXT_ALIGN_CENTER,
              color_white);
    snprintf(freq_buf, sizeof(freq_buf), " Tx:%03u.%05u",
             last_state->channel.tx_frequency/1000000,
             last_state->channel.tx_frequency%1000000/10);
    gfx_print(layout.line3_left, freq_buf, layout.line3_font, TEXT_ALIGN_CENTER,
              color_white);
}

void _ui_drawMEMMiddle(state_t* last_state)
{
    // Print Channel name
    gfx_print(layout.line1_left, last_state->channel.name, layout.line1_font, TEXT_ALIGN_CENTER,
              color_white);
    // Print Channel frequencies
    char freq_buf[20] = "";
    snprintf(freq_buf, sizeof(freq_buf), " Rx:%03u.%05u",
             last_state->channel.rx_frequency/1000000,
             last_state->channel.rx_frequency%1000000/10);
    gfx_print(layout.line2_left, freq_buf, layout.line2_font, TEXT_ALIGN_CENTER,
              color_white);
    snprintf(freq_buf, sizeof(freq_buf), " Tx:%03u.%05u",
             last_state->channel.tx_frequency/1000000,
             last_state->channel.tx_frequency%1000000/10);
    gfx_print(layout.line3_left, freq_buf, layout.line3_font, TEXT_ALIGN_CENTER,
              color_white);
}

void _ui_drawVFOMiddleInput(state_t* last_state, ui_state_t* ui_state)
{
    // Add inserted number to string, skipping "Rx: "/"Tx: " and "."
    uint8_t insert_pos = ui_state->input_position + 3;
    if(ui_state->input_position > 3) insert_pos += 1;
    char input_char = ui_state->input_number + '0';
    char freq_buf[14] = "";

    if(ui_state->input_set == SET_RX)
    {
        if(ui_state->input_position == 0)
        {
            snprintf(freq_buf, sizeof(freq_buf), ">Rx:%03u.%05u",
                     ui_state->new_rx_frequency/1000000,
                     ui_state->new_rx_frequency%1000000/10);
            gfx_print(layout.line2_left, freq_buf, layout.line2_font, TEXT_ALIGN_CENTER,
                      color_white);
        }
        else
        {
            // Replace Rx frequency with underscorses
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_rx_freq_buf, ">Rx:___._____");
            ui_state->new_rx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line2_left, ui_state->new_rx_freq_buf, layout.line2_font, TEXT_ALIGN_CENTER,
                      color_white);
        }
        snprintf(freq_buf, sizeof(freq_buf), " Tx:%03u.%05u",
                 last_state->channel.tx_frequency/1000000,
                 last_state->channel.tx_frequency%1000000/10);
        gfx_print(layout.line3_left, freq_buf, layout.line3_font, TEXT_ALIGN_CENTER,
                  color_white);
    }
    else if(ui_state->input_set == SET_TX)
    {
        snprintf(freq_buf, sizeof(freq_buf), " Rx:%03u.%05u",
                 ui_state->new_rx_frequency/1000000,
                 ui_state->new_rx_frequency%1000000/10);
        gfx_print(layout.line2_left, freq_buf, layout.line2_font, TEXT_ALIGN_CENTER,
                  color_white);
        // Replace Rx frequency with underscorses
        if(ui_state->input_position == 0)
        {
            snprintf(freq_buf, sizeof(freq_buf), ">Tx:%03u.%05u",
                     ui_state->new_rx_frequency/1000000,
                     ui_state->new_rx_frequency%1000000/10);
            gfx_print(layout.line3_left, freq_buf, layout.line3_font, TEXT_ALIGN_CENTER,
                      color_white);
        }
        else
        {
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_tx_freq_buf, ">Tx:___._____");
            ui_state->new_tx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line3_left, ui_state->new_tx_freq_buf, layout.line3_font, TEXT_ALIGN_CENTER,
                      color_white);
        }
    }
}

void _ui_drawVFOBottom()
{
    gfx_print(layout.bottom_left, "VFO", layout.bottom_font,
              TEXT_ALIGN_LEFT, color_white);
    gfx_print(layout.bottom_left, "OpenRTX", layout.bottom_font,
              TEXT_ALIGN_CENTER, color_white);
}

void _ui_drawMEMBottom()
{
    gfx_print(layout.bottom_left, "MEM", layout.bottom_font,
              TEXT_ALIGN_LEFT, color_white);
    gfx_print(layout.bottom_left, "OpenRTX", layout.bottom_font,
              TEXT_ALIGN_CENTER, color_white);
}

void _ui_drawMainVFO(state_t* last_state)
{
    gfx_clearScreen();
    _ui_drawMainBackground();
    _ui_drawMainTop(last_state);
    _ui_drawVFOMiddle(last_state);
    _ui_drawVFOBottom();
}

void _ui_drawMainVFOInput(state_t* last_state, ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainBackground();
    _ui_drawMainTop(last_state);
    _ui_drawVFOMiddleInput(last_state, ui_state);
    _ui_drawVFOBottom();
}

void _ui_drawMainMEM(state_t* last_state)
{
    gfx_clearScreen();
    _ui_drawMainBackground();
    _ui_drawMainTop(last_state);
    _ui_drawMEMMiddle(last_state);
    _ui_drawMEMBottom();
}
