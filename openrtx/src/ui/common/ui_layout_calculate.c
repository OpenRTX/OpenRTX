/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ui/ui_layout.h"
#include "hwconfig.h"

#ifdef OPENRTX_UI_MODULE17

extern const layout_t _layout_config;

void _ui_calculateLayout(layout_t *layout)
{
    *layout = _layout_config;
}

#else  /* default handheld */

extern const layout_t _layout_config;

void _ui_calculateLayout(layout_t *layout)
{
    *layout = _layout_config;

    uint16_t h = layout->horizontal_pad;
    uint16_t o = layout->text_v_offset;
    uint16_t t = layout->top_h;
    uint16_t s = layout->status_v_pad;
    uint16_t p = layout->top_pad;
    uint16_t v = layout->small_line_v_pad;
    uint16_t b = layout->big_line_v_pad;

    layout->top_pos = (point_t){ h, t - s - o };
    layout->line1_pos = (point_t){ h, t + p + layout->line1_h - v - o };
    layout->line2_pos =
        (point_t){ h, t + p + layout->line1_h + layout->line2_h - v - o };
    layout->line3_pos = (point_t){ h, t + p + layout->line1_h + layout->line2_h
                                          + layout->line3_h - v - o };
    layout->line4_pos = (point_t){ h, t + p + layout->line1_h + layout->line2_h
                                          + layout->line3_h + layout->line4_h
                                          - v - o };
    layout->line3_large_pos =
        (point_t){ h, t + p + layout->line1_h + layout->line2_h
                          + layout->line3_large_h - b - o };
    layout->bottom_pos =
        (point_t){ h, CONFIG_SCREEN_HEIGHT - layout->bottom_pad - s - o };
}

#endif /* OPENRTX_UI_MODULE17 */
