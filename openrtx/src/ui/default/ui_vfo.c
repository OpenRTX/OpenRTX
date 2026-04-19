/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ui/ui_default.h"
#include "ui_draw_private.h"
#include "ui_vfo.h"
#include "ui_menubar.h"
#include "ui_mode.h"
#include "ui_layout_config.h"
#include "core/graphics.h"
#include "rtx/rtx.h"
#include <stdint.h>
#include <string.h>

void _ui_drawMainVFO(ui_state_t *ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop(ui_state);
    _ui_drawModeInfo(ui_state);

#ifdef CONFIG_M17
    // Show VFO frequency if the OpMode is not M17 or there is no valid LSF data
    rtxStatus_t status = rtx_getCurrentStatus();
    if ((status.opMode != OPMODE_M17) || (status.lsfOk == false))
#endif
        _ui_drawFrequency();

    _ui_drawMainBottom();
}

void _ui_drawVFOMiddleInput(ui_state_t *ui_state)
{
    // Add inserted number to string, skipping "Rx: "/"Tx: " and "."
    uint8_t insert_pos = ui_state->input_position + 3;
    if (ui_state->input_position > 3)
        insert_pos += 1;
    char input_char = ui_state->input_number + '0';

    if (ui_state->input_set == SET_RX) {
        if (ui_state->input_position == 0) {
            gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ">Rx:%03lu.%04lu",
                      (unsigned long)ui_state->new_rx_frequency / 1000000,
                      (unsigned long)(ui_state->new_rx_frequency % 1000000)
                          / 100);
        } else {
            // Replace Rx frequency with underscorses
            if (ui_state->input_position == 1)
                strcpy(ui_state->new_rx_freq_buf, ">Rx:___.____");
            ui_state->new_rx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ui_state->new_rx_freq_buf);
        }
        gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
                  color_white, " Tx:%03lu.%04lu",
                  (unsigned long)last_state.channel.tx_frequency / 1000000,
                  (unsigned long)(last_state.channel.tx_frequency % 1000000)
                      / 100);
    } else if (ui_state->input_set == SET_TX) {
        gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                  color_white, " Rx:%03lu.%04lu",
                  (unsigned long)ui_state->new_rx_frequency / 1000000,
                  (unsigned long)(ui_state->new_rx_frequency % 1000000) / 100);
        // Replace Rx frequency with underscorses
        if (ui_state->input_position == 0) {
            gfx_print(layout.line3_large_pos, layout.input_font,
                      TEXT_ALIGN_CENTER, color_white, ">Tx:%03lu.%04lu",
                      (unsigned long)ui_state->new_rx_frequency / 1000000,
                      (unsigned long)(ui_state->new_rx_frequency % 1000000)
                          / 100);
        } else {
            if (ui_state->input_position == 1)
                strcpy(ui_state->new_tx_freq_buf, ">Tx:___.____");
            ui_state->new_tx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line3_large_pos, layout.input_font,
                      TEXT_ALIGN_CENTER, color_white,
                      ui_state->new_tx_freq_buf);
        }
    }
}

void _ui_drawMainVFOInput(ui_state_t *ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop(ui_state);
    _ui_drawVFOMiddleInput(ui_state);
    _ui_drawMainBottom();
}
