/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * \file ui_layout.h
 * \brief Screen layout metrics and derived positions for OpenRTX UIs.
 *
 * Handheld (default) and Module17 use different layout_t shapes; this header
 * selects the correct definition via OPENRTX_UI_MODULE17 (see Meson).
 */

#ifndef OPENRTX_UI_LAYOUT_H
#define OPENRTX_UI_LAYOUT_H

#include <stdint.h>
#include "core/graphics.h"

#if defined(OPENRTX_UI_MODULE17)

typedef struct layout_t {
    uint16_t hline_h;
    uint16_t top_h;
    uint16_t line1_h;
    uint16_t line2_h;
    uint16_t line3_h;
    uint16_t line4_h;
    uint16_t line5_h;
    uint16_t menu_h;
    uint16_t bottom_h;
    uint16_t bottom_pad;
    uint16_t status_v_pad;
    uint16_t horizontal_pad;
    uint16_t text_v_offset;
    point_t top_pos;
    point_t line1_pos;
    point_t line2_pos;
    point_t line3_pos;
    point_t line4_pos;
    point_t line5_pos;
    point_t bottom_pos;
    fontSize_t top_font;
    symbolSize_t top_symbol_font;
    fontSize_t line1_font;
    symbolSize_t line1_symbol_font;
    fontSize_t line2_font;
    symbolSize_t line2_symbol_font;
    fontSize_t line3_font;
    symbolSize_t line3_symbol_font;
    fontSize_t line4_font;
    symbolSize_t line4_symbol_font;
    fontSize_t line5_font;
    symbolSize_t line5_symbol_font;
    fontSize_t bottom_font;
    fontSize_t input_font;
    fontSize_t menu_font;
    fontSize_t message_font;
    fontSize_t mode_font_big;
    fontSize_t mode_font_small;
} layout_t;

#else  /* default handheld UI */

typedef struct layout_t {
    uint16_t hline_h;
    uint16_t top_h;
    uint16_t line1_h;
    uint16_t line2_h;
    uint16_t line3_h;
    uint16_t line3_large_h;
    uint16_t line4_h;
    uint16_t menu_h;
    uint16_t bottom_h;
    uint16_t bottom_pad;
    uint16_t status_v_pad;
    uint16_t horizontal_pad;
    uint16_t text_v_offset;
    uint16_t top_pad;
    uint16_t small_line_v_pad;
    uint16_t big_line_v_pad;
    point_t top_pos;
    point_t line1_pos;
    point_t line2_pos;
    point_t line3_pos;
    point_t line3_large_pos;
    point_t line4_pos;
    point_t bottom_pos;
    fontSize_t top_font;
    symbolSize_t top_symbol_size;
    fontSize_t line1_font;
    symbolSize_t line1_symbol_size;
    fontSize_t line2_font;
    symbolSize_t line2_symbol_size;
    fontSize_t line3_font;
    symbolSize_t line3_symbol_size;
    fontSize_t line3_large_font;
    fontSize_t line4_font;
    symbolSize_t line4_symbol_size;
    fontSize_t bottom_font;
    fontSize_t input_font;
    fontSize_t menu_font;
    fontSize_t message_font;
} layout_t;

#endif /* OPENRTX_UI_MODULE17 */

/**
 * \brief Fill \p layout from target constants and derived row positions.
 */
void _ui_calculateLayout(layout_t *layout);

#endif /* OPENRTX_UI_LAYOUT_H */
