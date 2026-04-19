/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ui_macro_menu.h"
#include "ui_layout_config.h"
#include "ui_draw_private.h"
#include "core/cps.h"
#include "ui/ui_strings.h"
#include <stdio.h>

static void draw_macro_top(void)
{
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->macroMenu);
    if (macro_latched) {
        gfx_drawSymbol(layout.top_pos, layout.top_symbol_size, TEXT_ALIGN_LEFT,
                       color_white, SYMBOL_ALPHA_M_BOX_OUTLINE);
    }
    if (last_state.settings.gps_enabled) {
        if (last_state.gps_data.fix_quality > 0) {
            gfx_drawSymbol(layout.top_pos, layout.top_symbol_size,
                           TEXT_ALIGN_RIGHT, color_white,
                           SYMBOL_CROSSHAIRS_GPS);
        } else {
            gfx_drawSymbol(layout.top_pos, layout.top_symbol_size,
                           TEXT_ALIGN_RIGHT, color_white, SYMBOL_CROSSHAIRS);
        }
    }
}

bool _ui_drawMacroMenu(ui_state_t *ui_state)
{
    // Header
    draw_macro_top();
    // First row
    if (last_state.channel.mode == OPMODE_FM) {
/*
 * If we have a keyboard installed draw all numbers, otherwise draw only the
 * currently selected number.
 */
#if defined(CONFIG_UI_NO_KEYBOARD)
        if (ui_state->macro_menu_selected == 0)
#endif // CONFIG_UI_NO_KEYBOARD
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      yellow_fab413, "1");
        if (last_state.channel.mode == OPMODE_FM) {
            char encdec_str[9] = { 0 };
            sniprintf(encdec_str, 9, "  %s",
                      _ui_getToneEnabledString(last_state.channel.fm.txToneEn,
                                               last_state.channel.fm.rxToneEn,
                                               true));

            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, encdec_str);
            uint16_t tone = ctcss_tone[last_state.channel.fm.txTone];
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, "       %d.%d", (tone / 10), (tone % 10));
#if defined(CONFIG_UI_NO_KEYBOARD)
            if (ui_state->macro_menu_selected == 1)
#endif // CONFIG_UI_NO_KEYBOARD
                gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                          yellow_fab413, "2");
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                      color_white, "       T-");
        }
#ifdef CONFIG_M17
        else if (last_state.channel.mode == OPMODE_M17) {
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      yellow_fab413, "1");
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_LEFT,
                      color_white, "          ");
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                      yellow_fab413, "2");
        }
#endif
#if defined(CONFIG_UI_NO_KEYBOARD)
        if (ui_state->macro_menu_selected == 2)
#endif // CONFIG_UI_NO_KEYBOARD
            gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                      yellow_fab413, "3        ");
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, " T+");
    }
#ifdef CONFIG_M17
    else if (last_state.channel.mode == OPMODE_M17) {
        char encdec_str[9] = "        ";
        gfx_print(layout.line1_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, encdec_str);
    }
#endif

    // Second row
    // Calculate symmetric second row position, line2_pos is asymmetric like main screen
    point_t pos_2 = {
        layout.line1_pos.x,
        layout.line1_pos.y + (layout.line3_large_pos.y - layout.line1_pos.y) / 2
    };

#if defined(CONFIG_UI_NO_KEYBOARD)
    if (ui_state->macro_menu_selected == 3)
#endif // CONFIG_UI_NO_KEYBOARD
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT, yellow_fab413, "4");

    if (last_state.channel.mode == OPMODE_FM) {
        char bw_str[12] = { 0 };
        switch (last_state.channel.bandwidth) {
            case BW_12_5:
                sniprintf(bw_str, 12, "  BW12.5");
                break;
            case BW_25:
                sniprintf(bw_str, 12, "  BW   25");
                break;
        }

        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT, color_white, bw_str);
    }
#ifdef CONFIG_M17
    else if (last_state.channel.mode == OPMODE_M17) {
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_LEFT, color_white,
                  "       ");
    }
#endif

#if defined(CONFIG_UI_NO_KEYBOARD)
    if (ui_state->macro_menu_selected == 4)
#endif // CONFIG_UI_NO_KEYBOARD
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER, yellow_fab413,
                  "5");

    char mode_str[12] = "";
    switch (last_state.channel.mode) {
        case OPMODE_FM:
            gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER, color_white,
                      "         FM");
            break;
        case OPMODE_DMR:
            gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER, color_white,
                      "          DMR");
            break;
#ifdef CONFIG_M17
        case OPMODE_M17:
            gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER, color_white,
                      "          M17");
            break;
#endif
    }

    gfx_print(pos_2, layout.top_font, TEXT_ALIGN_CENTER, color_white, mode_str);

#if defined(CONFIG_UI_NO_KEYBOARD)
    if (ui_state->macro_menu_selected == 5)
#endif // CONFIG_UI_NO_KEYBOARD
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_RIGHT, yellow_fab413,
                  "6        ");

    // Compute x.y format for TX power avoiding to pull in floating point math.
    // Remember that power is expressed in mW!
    unsigned int power_int = (last_state.channel.power / 1000);
    unsigned int power_dec = (last_state.channel.power % 1000) / 100;

    if (power_dec == 0) {
        // Print power output as an integer without a decimal (i.e "5W")
        gfx_print(pos_2, layout.top_font, TEXT_ALIGN_RIGHT, color_white, "%dW",
                  power_int);
    } else {
        // Smaller font to fit larger strings on screen (i.e "2.5W")
        gfx_print(pos_2, layout.top_font - 1, TEXT_ALIGN_RIGHT, color_white,
                  "%d.%dW", power_int, power_dec);
    }

    // Third row
#if defined(CONFIG_UI_NO_KEYBOARD)
    if (ui_state->macro_menu_selected == 6)
#endif // CONFIG_UI_NO_KEYBOARD
        gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_LEFT,
                  yellow_fab413, "7");
#ifdef CONFIG_SCREEN_BRIGHTNESS
    gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_LEFT,
              color_white, "  B-");
    gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_LEFT,
              color_white, "       %5d", state.settings.brightness);
#endif

#if defined(CONFIG_UI_NO_KEYBOARD)
    if (ui_state->macro_menu_selected == 7)
#endif // CONFIG_UI_NO_KEYBOARD
        gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  yellow_fab413, "8");
#ifdef CONFIG_SCREEN_BRIGHTNESS
    gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "        B+");
#endif

#if defined(CONFIG_UI_NO_KEYBOARD)
    if (ui_state->macro_menu_selected == 8)
#endif // CONFIG_UI_NO_KEYBOARD
        gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  yellow_fab413, "9        ");
    if (ui_state->input_locked == true)
        gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "Unlk");
    else
        gfx_print(layout.line3_large_pos, layout.top_font, TEXT_ALIGN_RIGHT,
                  color_white, "Lck");

    // Draw S-meter bar
    _ui_drawMainBottom();
    return true;
}
