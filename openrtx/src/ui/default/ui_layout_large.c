/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Tytera MD380, MD-UV380

#include "ui_layout.h"

const layout_t _layout_config = {
    .hline_h = 1,
    .text_v_offset = 1,
    .top_pad = 4,
    .top_h = 16,
    .line1_h = 20,
    .line2_h = 20,
    .line3_h = 20,
    .line3_large_h = 40,
    .line4_h = 20,
    .menu_h = 16,
    .bottom_h = 23,
    .bottom_pad = 4,
    .status_v_pad = 2,
    .small_line_v_pad = 2,
    .big_line_v_pad = 6,
    .horizontal_pad = 4,

    .top_font = FONT_SIZE_8PT,
    .top_symbol_size = SYMBOLS_SIZE_8PT,
    .line1_font = FONT_SIZE_8PT,
    .line1_symbol_size = SYMBOLS_SIZE_8PT,
    .line2_font = FONT_SIZE_8PT,
    .line2_symbol_size = SYMBOLS_SIZE_8PT,
    .line3_font = FONT_SIZE_8PT,
    .line3_symbol_size = SYMBOLS_SIZE_8PT,
    .line3_large_font = FONT_SIZE_16PT,
    .line4_font = FONT_SIZE_8PT,
    .line4_symbol_size = SYMBOLS_SIZE_8PT,
    .bottom_font = FONT_SIZE_8PT,
    .input_font = FONT_SIZE_12PT,
    .menu_font = FONT_SIZE_8PT,
    .message_font = FONT_SIZE_6PT,
};
