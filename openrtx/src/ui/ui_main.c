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

#include <interfaces/platform.h>
#include <interfaces/cps_io.h>
#include <stdio.h>
#include <stdint.h>
#include <ui.h>
#include <string.h>
#include "ui/ui_strings.h"

void _ui_drawMainBackground()
{
    // Print top bar line of hline_h pixel height
    gfx_drawHLine(layout.top_h, layout.hline_h, color_grey);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(SCREEN_HEIGHT - layout.bottom_h - 1, layout.hline_h, color_grey);
}

void _ui_drawMainTop()
{
#ifdef RTC_PRESENT
    // Print clock on top bar
    datetime_t local_time = utcToLocalTime(last_state.time,
                                           last_state.settings.utc_timezone);
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "%02d:%02d:%02d", local_time.hour,
              local_time.minute, local_time.second);
#endif
    // If the radio has no built-in battery, print input voltage
#ifdef BAT_NONE
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_RIGHT,
              color_white,"%.1fV", last_state.v_bat);
#else
    // Otherwise print battery icon on top bar, use 4 px padding
    uint16_t bat_width = SCREEN_WIDTH / 9;
    uint16_t bat_height = layout.top_h - (layout.status_v_pad * 2);
    point_t bat_pos = {SCREEN_WIDTH - bat_width - layout.horizontal_pad,
                       layout.status_v_pad};
    gfx_drawBattery(bat_pos, bat_width, bat_height, last_state.charge);
#endif
    // Print radio mode on top bar
    switch(last_state.channel.mode)
    {
        case OPMODE_FM:
        gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, currentLanguage->fm);
        break;
        case OPMODE_DMR:
        gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, currentLanguage->dmr);
        break;
        case OPMODE_M17:
        gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  color_white, currentLanguage->m17);
        break;
    }
}

void _ui_drawBankChannel()
{
    // Print Bank number, channel number and Channel name
    uint16_t b = (last_state.bank_enabled) ? last_state.bank : 0;
    gfx_print(layout.line1_pos, layout.line1_font, TEXT_ALIGN_CENTER,
              color_white, "%01d-%03d: %.12s",
              b, last_state.channel_index + 1, last_state.channel.name);
}

void _ui_drawModeInfo(ui_state_t* ui_state)
{
    char bw_str[8] = { 0 };
    char encdec_str[9] = { 0 };

    rtxStatus_t cfg = rtx_getCurrentStatus();

    switch(last_state.channel.mode)
    {
        case OPMODE_FM:
        // Get Bandwidth string
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

        // Print Bandwidth, Tone and encdec info
        gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
              color_white, "B:%s T:%4.1f S:%s",
              bw_str, ctcss_tone[last_state.channel.fm.txTone]/10.0f,
              encdec_str);
        break;
        case OPMODE_DMR:
        // Print talkgroup
        gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
              color_white, "TG:%s",
              "");
        break;
        case OPMODE_M17:
        {
            // Print M17 Destination ID on line 3 of 3
            const char *dst = NULL;
            if(ui_state->edit_mode)
                dst = ui_state->new_callsign;
            else
                dst = (!strnlen(cfg.destination_address, 10)) ?
                    currentLanguage->broadcast : cfg.destination_address;
            gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                  color_white, "#%s", dst);
            break;
        }
    }
}

void _ui_drawFrequency()
{
  unsigned long frequency = platform_getPttStatus() ?
       frequency = last_state.channel.tx_frequency : last_state.channel.rx_frequency;

    // Print big numbers frequency
    gfx_print(layout.line3_pos, layout.line3_font, TEXT_ALIGN_CENTER,
              color_white, "%03lu.%05lu",
              (unsigned long)frequency/1000000,
              (unsigned long)frequency%1000000/10);
}

void _ui_drawVFOMiddleInput(ui_state_t* ui_state)
{
    // Add inserted number to string, skipping "Rx: "/"Tx: " and "."
    uint8_t insert_pos = ui_state->input_position + 3;
    if(ui_state->input_position > 3) insert_pos += 1;
    char input_char = ui_state->input_number + '0';

    if(ui_state->input_set == SET_RX)
    {
        if(ui_state->input_position == 0)
        {
            gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ">Rx:%03lu.%04lu",
                      (unsigned long)ui_state->new_rx_frequency/1000000,
                      (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        }
        else
        {
            // Replace Rx frequency with underscorses
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_rx_freq_buf, ">Rx:___.____");
            ui_state->new_rx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ui_state->new_rx_freq_buf);
        }
        gfx_print(layout.line3_pos, layout.input_font, TEXT_ALIGN_CENTER,
                  color_white, " Tx:%03lu.%04lu",
                  (unsigned long)last_state.channel.tx_frequency/1000000,
                  (unsigned long)(last_state.channel.tx_frequency%1000000)/100);
    }
    else if(ui_state->input_set == SET_TX)
    {
        gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                  color_white, " Rx:%03lu.%04lu",
                  (unsigned long)ui_state->new_rx_frequency/1000000,
                  (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        // Replace Rx frequency with underscorses
        if(ui_state->input_position == 0)
        {
            gfx_print(layout.line3_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ">Tx:%03lu.%04lu",
                      (unsigned long)ui_state->new_rx_frequency/1000000,
                      (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        }
        else
        {
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_tx_freq_buf, ">Tx:___.____");
            ui_state->new_tx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line3_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ui_state->new_tx_freq_buf);
        }
    }
}

void _ui_drawMainBottom()
{
    // Squelch bar
    float rssi = last_state.rssi;
    float squelch = last_state.settings.sqlLevel / 16.0f;
    uint16_t meter_width = SCREEN_WIDTH - 2 * layout.horizontal_pad;
    uint16_t meter_height = layout.bottom_h;
    point_t meter_pos = { layout.horizontal_pad,
                          SCREEN_HEIGHT - meter_height - layout.bottom_pad};
    uint8_t mic_level = platform_getMicLevel();
    switch(last_state.channel.mode)
    {
        case OPMODE_FM:
            gfx_drawSmeter(meter_pos,
                           meter_width,
                           meter_height,
                           rssi,
                           squelch,
                           yellow_fab413);
            break;
        case OPMODE_DMR:
            gfx_drawSmeterLevel(meter_pos,
                                meter_width,
                                meter_height,
                                rssi,
                                mic_level);
            break;
        case OPMODE_M17:
            gfx_drawSmeterLevel(meter_pos,
                                meter_width,
                                meter_height,
                                rssi,
                                mic_level);
            break;
    }
}

void _ui_drawMainVFO(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop();
    _ui_drawModeInfo(ui_state);
    _ui_drawFrequency();
    _ui_drawMainBottom();
}

void _ui_drawMainVFOInput(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop();
    _ui_drawVFOMiddleInput(ui_state);
    _ui_drawMainBottom();
}

void _ui_drawMainMEM(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop();
    _ui_drawBankChannel();
    _ui_drawModeInfo(ui_state);
    _ui_drawFrequency();
    _ui_drawMainBottom();
}
