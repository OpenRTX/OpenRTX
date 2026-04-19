/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Radioddity RD-5R

#include "ui_layout_config.h"

const layout_t _layout_config = {
    .hline_h = 1,
    .text_v_offset = 1,
    .top_pad = 1,
    .top_h = 11,
    .line1_h = 0,
    .line2_h = 10,
    .line3_h = 10,
    .line3_large_h = 18,
    .line4_h = 10,
    .menu_h = 10,
    .bottom_h = 0,
    .bottom_pad = 0,
    .status_v_pad = 1,
    .small_line_v_pad = 1,
    .big_line_v_pad = 0,
    .horizontal_pad = 4,

    .top_font = FONT_SIZE_6PT,
    .top_symbol_size = SYMBOLS_SIZE_6PT,
    .line1_font = 0,
    .line1_symbol_size = 0,
    .line2_font = FONT_SIZE_6PT,
    .line2_symbol_size = 0,
    .line3_font = FONT_SIZE_6PT,
    .line3_symbol_size = 0,
    .line3_large_font = FONT_SIZE_12PT,
    .line4_font = FONT_SIZE_6PT,
    .line4_symbol_size = 0,
    .bottom_font = 0,
    .input_font = FONT_SIZE_8PT,
    .menu_font = FONT_SIZE_6PT,
    .message_font = FONT_SIZE_6PT,
};
