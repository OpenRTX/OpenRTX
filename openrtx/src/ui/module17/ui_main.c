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
#include <ui/ui_mod17.h>
#include <string.h>

void _ui_Draw_MainBackground()
{
    // Print top bar line of hline_h pixel height
    gfx_drawHLine(layout.lines[ GUI_LINE_TOP ].height, layout.hline_h, color_gg);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(SCREEN_HEIGHT - layout.lines[ GUI_LINE_BOTTOM ].height - 1, layout.hline_h, color_gg);
}

void _ui_Draw_MainTop()
{
#ifdef RTC_PRESENT
    // Print clock on top bar
    datetime_t localTime = utcToLocalTime(last_state.time,
                                           last_state.settings.utc_timezone);
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, GFX_ALIGN_CENTER,
              color_fg, "%02d:%02d:%02d", localTime.hour,
              localTime.minute, localTime.second);
#endif

    // Print the source callsign on top bar
    gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, GFX_ALIGN_LEFT,
                  color_fg, state.settings.callsign);
}

void _ui_Draw_BankChannel()
{
    // Print Bank number, channel number and Channel name
    uint16_t b = (last_state.bank_enabled) ? last_state.bank : 0;
    gfx_print(layout.lines[ GUI_LINE_1 ].pos, layout.lines[ GUI_LINE_1 ].font, GFX_ALIGN_CENTER,
              color_fg, "%01d-%03d: %.12s",
              b, last_state.channel_index + 1, last_state.channel.name);
}

void _ui_Draw_ModeInfo(UI_State_st* ui_state)
{
    char bw_str[8] = { 0 };
    char encdec_str[9] = { 0 };

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
        gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.lines[ GUI_LINE_2 ].font, GFX_ALIGN_CENTER,
              color_fg, "B:%s T:%4.1f S:%s",
              bw_str, ctcss_tone[last_state.channel.fm.txTone]/10.0f,
              encdec_str);
        break;
        case OPMODE_DMR:
        // Print talkgroup
        gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.lines[ GUI_LINE_2 ].font, GFX_ALIGN_CENTER,
              color_fg, "TG:%s",
              "");
        break;
        case OPMODE_M17:
        {
            rtxStatus_t rtxStatus = rtx_getCurrentStatus();

            if(rtxStatus.lsfOk)
            {
                gfx_drawSymbol(layout.lines[ GUI_LINE_2 ].pos, layout.lines[ GUI_LINE_2 ].symbol_font, GFX_ALIGN_LEFT,
                               color_fg, SYMBOL_CALL_RECEIVED);
                gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.lines[ GUI_LINE_2 ].font, GFX_ALIGN_CENTER,
                          color_fg, "%s", rtxStatus.M17_dst);
                gfx_drawSymbol(layout.lines[ GUI_LINE_1 ].pos, layout.lines[ GUI_LINE_1 ].symbol_font, GFX_ALIGN_LEFT,
                               color_fg, SYMBOL_CALL_MADE);
                gfx_print(layout.lines[ GUI_LINE_1 ].pos, layout.lines[ GUI_LINE_2 ].font, GFX_ALIGN_CENTER,
                          color_fg, "%s", rtxStatus.M17_src);

                if(rtxStatus.M17_link[0] != '\0')
                {
                    gfx_drawSymbol(layout.lines[ GUI_LINE_4 ].pos, layout.lines[ GUI_LINE_3 ]_symbol_font, GFX_ALIGN_LEFT,
                                color_fg, SYMBOL_ACCESS_POINT);
                    gfx_print(layout.lines[ GUI_LINE_4 ].pos, layout.lines[ GUI_LINE_2 ].font, GFX_ALIGN_CENTER,
                            color_fg, "%s", rtxStatus.M17_link);
                }

                if(rtxStatus.M17_refl[0] != '\0')
                {
                    gfx_drawSymbol(layout.lines[ GUI_LINE_3 ].pos, layout.lines[ GUI_LINE_4 ]_symbol_font, GFX_ALIGN_LEFT,
                                   color_fg, SYMBOL_NETWORK);
                    gfx_print(layout.lines[ GUI_LINE_3 ].pos, layout.lines[ GUI_LINE_2 ].font, GFX_ALIGN_CENTER,
                              color_fg, "%s", rtxStatus.M17_refl);
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
                gfx_print(layout.lines[ GUI_LINE_TOP ].pos, layout.lines[ GUI_LINE_TOP ].font, GFX_ALIGN_RIGHT,
                          color_fg, "CAN %02d", state.settings.m17_can);
                gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.lines[ GUI_LINE_2 ].font, GFX_ALIGN_CENTER,
                          color_fg, last);
                // Print M17 Destination ID on line 2
                gfx_print(layout.lines[ GUI_LINE_3 ].pos, layout.lines[ GUI_LINE_3 ]_font, GFX_ALIGN_CENTER,
                          color_fg, "%s", dst);
                // Menu
                gfx_print(layout.line5_pos, layout.line5_font, GFX_ALIGN_RIGHT,
                          color_fg, "Menu");
                break;
            }
        }
    }
}

void _ui_Draw_Frequency()
{
  unsigned long frequency = platform_getPttStatus() ?
       frequency = last_state.channel.tx_frequency : last_state.channel.rx_frequency;

    // Print big numbers frequency
    gfx_print(layout.lines[ GUI_LINE_3 ].pos, layout.lines[ GUI_LINE_3 ]_font, GFX_ALIGN_CENTER,
              color_fg, "%03lu.%05lu",
              (unsigned long)frequency/1000000,
              (unsigned long)frequency%1000000/10);
}

void _ui_Draw_VFOMiddleInput(UI_State_st* ui_state)
{
    // Add inserted number to string, skipping "Rx: "/"Tx: " and "."
    uint8_t insert_pos = ui_state->input_position + 3;
    if(ui_state->input_position > 3) insert_pos += 1;
    char input_char = ui_state->input_number + '0';

    if(ui_state->input_set == SET_RX)
    {
        if(ui_state->input_position == 0)
        {
            gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.input_font.size, GFX_ALIGN_CENTER,
                      color_fg, ">Rx:%03lu.%04lu",
                      (unsigned long)ui_state->new_rx_frequency/1000000,
                      (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        }
        else
        {
            // Replace Rx frequency with underscorses
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_rx_freq_buf, ">Rx:___.____");
            ui_state->new_rx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.input_font.size, GFX_ALIGN_CENTER,
                      color_fg, ui_state->new_rx_freq_buf);
        }
        gfx_print(layout.lines[ GUI_LINE_3 ].pos, layout.input_font.size, GFX_ALIGN_CENTER,
                  color_fg, " Tx:%03lu.%04lu",
                  (unsigned long)last_state.channel.tx_frequency/1000000,
                  (unsigned long)(last_state.channel.tx_frequency%1000000)/100);
    }
    else if(ui_state->input_set == SET_TX)
    {
        gfx_print(layout.lines[ GUI_LINE_2 ].pos, layout.input_font.size, GFX_ALIGN_CENTER,
                  color_fg, " Rx:%03lu.%04lu",
                  (unsigned long)ui_state->new_rx_frequency/1000000,
                  (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        // Replace Rx frequency with underscorses
        if(ui_state->input_position == 0)
        {
            gfx_print(layout.lines[ GUI_LINE_3 ].pos, layout.input_font.size, GFX_ALIGN_CENTER,
                      color_fg, ">Tx:%03lu.%04lu",
                      (unsigned long)ui_state->new_rx_frequency/1000000,
                      (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        }
        else
        {
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_tx_freq_buf, ">Tx:___.____");
            ui_state->new_tx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.lines[ GUI_LINE_3 ].pos, layout.input_font.size, GFX_ALIGN_CENTER,
                      color_fg, ui_state->new_tx_freq_buf);
        }
    }
}

void _ui_Draw_MainBottom()
{
    // Squelch bar
    float rssi = last_state.rssi;
    float squelch = last_state.settings.sqlLevel / 16.0f;
    uint16_t meter_width = SCREEN_WIDTH - 2 * layout.horizontal_pad;
    uint16_t meter_height = layout.lines[ GUI_LINE_BOTTOM ].height;
    Pos_st meter_pos = { layout.horizontal_pad,
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
            /*gfx_drawSmeterLevel(meter_pos,
                                meter_width,
                                meter_height,
                                rssi,
                                mic_level);*/
            break;
    }
}

void _ui_Draw_MainVFO(UI_State_st* ui_state)
{
    gfx_clearScreen();
    _ui_Draw_MainTop();
    _ui_Draw_ModeInfo(ui_state);
    //_ui_Draw_Frequency(); //has to be replaced with Line 1 and Line 2
    _ui_Draw_MainBottom();
}

void _ui_Draw_MainVFOInput(UI_State_st* ui_state)
{
    gfx_clearScreen();
    _ui_Draw_MainTop();
    _ui_Draw_VFOMiddleInput(ui_state);
    _ui_Draw_MainBottom();
}

void _ui_Draw_MainMEM(UI_State_st* ui_state)
{
    gfx_clearScreen();
    _ui_Draw_MainTop();
    _ui_Draw_BankChannel();
    _ui_Draw_ModeInfo(ui_state);
    //_ui_Draw_Frequency(); //has to be replaced with Line 1 and Line 2
    _ui_Draw_MainBottom();
}
