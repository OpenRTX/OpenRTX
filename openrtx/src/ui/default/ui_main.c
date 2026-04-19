/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "interfaces/cps_io.h"
#include "interfaces/delays.h"
#include <stdio.h>
#include <stdint.h>
#include "ui/ui_default.h"
#include <string.h>
#include "ui_draw_private.h"
#include "ui_menubar.h"
#include "ui_mode.h"
#include "ui/ui_strings.h"
#include "core/utils.h"
#include "ui/utils.h"

void _ui_drawMainBackground()
{
    // Print top bar line of hline_h pixel height
    gfx_drawHLine(layout.top_h, layout.hline_h, color_grey);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(CONFIG_SCREEN_HEIGHT - layout.bottom_h - 1, layout.hline_h,
                  color_grey);
}

void _ui_drawBankChannel()
{
    // Print Bank number, channel number and Channel name
    uint16_t b = (last_state.bank_enabled) ? last_state.bank : 0;
    gfx_print(layout.line1_pos, layout.line1_font, TEXT_ALIGN_CENTER,
              color_white, "%01d-%03d: %.12s", b, last_state.channel_index + 1,
              last_state.channel.name);
}

const char *_ui_getToneEnabledString(bool tone_tx_enable, bool tone_rx_enable,
                                     bool use_abbreviation)
{
    const char *strings[2][4] = { {
                                      currentLanguage->None,
                                      currentLanguage->Encode,
                                      currentLanguage->Decode,
                                      currentLanguage->Both,
                                  },
                                  { "N", "T", "R", "B" } };

    uint8_t index = (tone_rx_enable << 1) | (tone_tx_enable);
    return strings[use_abbreviation][index];
}

void _ui_drawFrequency()
{
    freq_t freq = platform_getPttStatus() ? last_state.channel.tx_frequency :
                                            last_state.channel.rx_frequency;

    // Print big numbers frequency
    char freq_str[16] = { 0 };
    sniprintf(freq_str, sizeof(freq_str), "%lu.%06lu", (freq / 1000000lu),
              (freq % 1000000lu));
    stripTrailingZeroes(freq_str);

    gfx_print(layout.line3_large_pos, layout.line3_large_font,
              TEXT_ALIGN_CENTER, color_white, "%s", freq_str);
}

void _ui_drawMainBottom()
{
    // Squelch bar
    rssi_t rssi = last_state.rssi;
    uint8_t squelch = last_state.settings.sqlLevel;
    uint8_t volume = last_state.volume;
    uint16_t meter_width = CONFIG_SCREEN_WIDTH - 2 * layout.horizontal_pad;
    uint16_t meter_height = layout.bottom_h;
    point_t meter_pos = { layout.horizontal_pad, CONFIG_SCREEN_HEIGHT
                                                     - meter_height
                                                     - layout.bottom_pad };
    uint8_t mic_level = platform_getMicLevel();
    switch (last_state.channel.mode) {
        case OPMODE_FM:
            gfx_drawSmeter(meter_pos, meter_width, meter_height, rssi, squelch,
                           volume, true, yellow_fab413);
            break;
        case OPMODE_DMR:
            gfx_drawSmeterLevel(meter_pos, meter_width, meter_height, rssi,
                                mic_level, volume, true);
            break;
#ifdef CONFIG_M17
        case OPMODE_M17:
            gfx_drawSmeterLevel(meter_pos, meter_width, meter_height, rssi,
                                mic_level, volume, true);
            break;
#endif
    }
}

void _ui_drawMainMEM(ui_state_t *ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop(ui_state);
    _ui_drawModeInfo(ui_state);

#ifdef CONFIG_M17
    // Show channel data if the OpMode is not M17 or there is no valid LSF data
    rtxStatus_t status = rtx_getCurrentStatus();
    if ((status.opMode != OPMODE_M17) || (status.lsfOk == false))
#endif
    {
        _ui_drawBankChannel();
        _ui_drawFrequency();
    }

    _ui_drawMainBottom();
}
