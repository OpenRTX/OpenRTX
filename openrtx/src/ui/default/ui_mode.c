/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ui/ui_default.h"
#include "ui_draw_private.h"
#include "ui_layout_config.h"
#include "core/graphics.h"
#include "core/cps.h"
#include "interfaces/delays.h"
#include "ui/ui_strings.h"
#include "ui/utils.h"
#include "rtx/rtx.h"
#include <stdio.h>
#include <string.h>

void _ui_drawModeInfo(ui_state_t *ui_state)
{
    char bw_str[8] = { 0 };

    switch (last_state.channel.mode) {
        case OPMODE_FM:

            // Get Bandwidth string
            if (last_state.channel.bandwidth == BW_12_5)
                sniprintf(bw_str, 8, "NFM");
            else if (last_state.channel.bandwidth == BW_25)
                sniprintf(bw_str, 8, "FM");

            // Get encdec string
            bool tone_tx_enable = last_state.channel.fm.txToneEn;
            bool tone_rx_enable = last_state.channel.fm.rxToneEn;
            // Print Bandwidth, Tone and encdec info
            if (tone_tx_enable || tone_rx_enable) {
                uint16_t tone = ctcss_tone[last_state.channel.fm.txTone];
                gfx_print(layout.line2_pos, layout.line2_font,
                          TEXT_ALIGN_CENTER, color_white, "%s %d.%d %s", bw_str,
                          (tone / 10), (tone % 10),
                          _ui_getToneEnabledString(tone_tx_enable,
                                                   tone_rx_enable, true));
            } else {
                gfx_print(layout.line2_pos, layout.line2_font,
                          TEXT_ALIGN_CENTER, color_white, "%s", bw_str);
            }
            break;

        case OPMODE_DMR:
            // Print talkgroup
            gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                      color_white, "DMR TG%s", "");
            break;

#ifdef CONFIG_M17
        case OPMODE_M17: {
            // Print M17 Destination ID on line 3 of 3
            rtxStatus_t rtxStatus = rtx_getCurrentStatus();

            if (rtxStatus.lsfOk) {
                // Destination address
                gfx_drawSymbol(layout.line2_pos, layout.line2_symbol_size,
                               TEXT_ALIGN_LEFT, color_white,
                               SYMBOL_CALL_RECEIVED);

                gfx_print(layout.line2_pos, layout.line2_font,
                          TEXT_ALIGN_CENTER, color_white, "%s",
                          rtxStatus.M17_dst);

                // Source address
                gfx_drawSymbol(layout.line1_pos, layout.line1_symbol_size,
                               TEXT_ALIGN_LEFT, color_white, SYMBOL_CALL_MADE);

                gfx_print(layout.line1_pos, layout.line2_font,
                          TEXT_ALIGN_CENTER, color_white, "%s",
                          rtxStatus.M17_src);

                // RF link (if present)
                if (rtxStatus.M17_link[0] != '\0') {
                    gfx_drawSymbol(layout.line4_pos, layout.line3_symbol_size,
                                   TEXT_ALIGN_LEFT, color_white,
                                   SYMBOL_ACCESS_POINT);

                    gfx_print(layout.line4_pos, layout.line2_font,
                              TEXT_ALIGN_CENTER, color_white, "%s",
                              rtxStatus.M17_link);
                }

                // Meta text (if present)
                if (rtxStatus.M17_meta_text[0] != '\0') {
                    char msg[14];

                    // Always display current position
                    scrollTextPeek(rtxStatus.M17_meta_text, msg, sizeof(msg),
                                   ui_state->m17_meta_text_scroll_position);
                    // Only advance scroll every 100ms
                    long long now = getTick();
                    if (now - ui_state->m17_meta_text_last_scroll_tick >= 100) {
                        scrollTextAdvance(
                            rtxStatus.M17_meta_text, sizeof(msg),
                            &ui_state->m17_meta_text_scroll_position);
                        ui_state->m17_meta_text_last_scroll_tick = now;
                    }
                    gfx_print(layout.line3_pos, layout.line2_font,
                              TEXT_ALIGN_CENTER, color_white, "%s", msg);
                }
                // Reflector (if present)
                else if (rtxStatus.M17_refl[0] != '\0') {
                    gfx_drawSymbol(layout.line3_pos, layout.line4_symbol_size,
                                   TEXT_ALIGN_LEFT, color_white,
                                   SYMBOL_NETWORK);

                    gfx_print(layout.line3_pos, layout.line2_font,
                              TEXT_ALIGN_CENTER, color_white, "%s",
                              rtxStatus.M17_refl);
                }

                // Reset scroll position when meta text becomes empty
                if (rtxStatus.M17_meta_text[0] == '\0') {
                    ui_state->m17_meta_text_scroll_position = 0;
                }
            } else {
                const char *dst = NULL;
                if (ui_state->edit_mode) {
                    dst = ui_state->new_callsign;
                } else {
                    if (strnlen(rtxStatus.destination_address, 10) == 0)
                        dst = currentLanguage->broadcast;
                    else
                        dst = rtxStatus.destination_address;
                }

                gfx_print(layout.line2_pos, layout.line2_font,
                          TEXT_ALIGN_CENTER, color_white, "M17 #%s", dst);
            }
            break;
        }
#endif
    }
}
