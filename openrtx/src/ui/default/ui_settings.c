/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ui/ui_default.h"
#include "ui_layout_config.h"
#include "ui_settings.h"
#include "ui_menu.h"
#include "ui/ui_menu_list.h"
#include "core/datetime.h"
#include "core/utils.h"
#include "interfaces/delays.h"
#include "ui/ui_strings.h"
#include <stdio.h>
#include <string.h>

void _ui_drawSettingsDisplay(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "Display" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->display);
    // Print display settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected,
                          _ui_getDisplayEntryName, _ui_getDisplayValueName);
}

#ifdef CONFIG_GPS
void _ui_drawSettingsGPS(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "GPS Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->gpsSettings);
    // Print display settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected,
                          _ui_getSettingsGPSEntryName,
                          _ui_getSettingsGPSValueName);
}
#endif

#ifdef CONFIG_RTC
void _ui_drawSettingsTimeDate(void)
{
    gfx_clearScreen();
    datetime_t local_time = utcToLocalTime(last_state.time,
                                           last_state.settings.utc_timezone);
    // Print "Time&Date" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->timeAndDate);
    // Print current time and date
    gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, "%02d/%02d/%02d", local_time.date, local_time.month,
              local_time.year);
    gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, "%02d:%02d:%02d", local_time.hour, local_time.minute,
              local_time.second);
}

void _ui_drawSettingsTimeDateSet(ui_state_t *ui_state)
{
    (void)last_state;

    gfx_clearScreen();
    // Print "Time&Date" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->timeAndDate);
    if (ui_state->input_position <= 0) {
        strcpy(ui_state->new_date_buf, "__/__/__");
        strcpy(ui_state->new_time_buf, "__:__:00");
    } else {
        char input_char = ui_state->input_number + '0';
        // Insert date digit
        if (ui_state->input_position <= 6) {
            uint8_t pos = ui_state->input_position - 1;
            // Skip "/"
            if (ui_state->input_position > 2)
                pos += 1;
            if (ui_state->input_position > 4)
                pos += 1;
            ui_state->new_date_buf[pos] = input_char;
        }
        // Insert time digit
        else {
            uint8_t pos = ui_state->input_position - 7;
            // Skip ":"
            if (ui_state->input_position > 8)
                pos += 1;
            ui_state->new_time_buf[pos] = input_char;
        }
    }
    gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, ui_state->new_date_buf);
    gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
              color_white, ui_state->new_time_buf);
}
#endif

#ifdef CONFIG_M17
void _ui_drawSettingsM17(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "M17 Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->m17settings);
    gfx_printLine(1, 4, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.menu_font, TEXT_ALIGN_LEFT,
                  color_white, currentLanguage->callsign);
    if ((ui_state->edit_mode) && (ui_state->menu_selected == M17_CALLSIGN)) {
        uint16_t rect_width = CONFIG_SCREEN_WIDTH - (layout.horizontal_pad * 2);
        uint16_t rect_height =
            (CONFIG_SCREEN_HEIGHT - (layout.top_h + layout.bottom_h)) / 2;
        point_t rect_origin = { (CONFIG_SCREEN_WIDTH - rect_width) / 2,
                                (CONFIG_SCREEN_HEIGHT - rect_height) / 2 };
        gfx_drawRect(rect_origin, rect_width, rect_height, color_white, false);
        // Print M17 callsign being typed
        gfx_printLine(1, 1, layout.top_h,
                      CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.input_font,
                      TEXT_ALIGN_CENTER, color_white, ui_state->new_callsign);
    } else if ((ui_state->edit_message)
               && (ui_state->menu_selected == M17_METATEXT)) {
        uint16_t rect_width = CONFIG_SCREEN_WIDTH - (layout.horizontal_pad * 2);
        uint16_t rect_height =
            (CONFIG_SCREEN_HEIGHT - (layout.top_h + layout.bottom_h)) / 2;
        point_t rect_origin = { (CONFIG_SCREEN_WIDTH - rect_width) / 2,
                                (CONFIG_SCREEN_HEIGHT - rect_height) / 2 };
        gfx_drawRect(rect_origin, rect_width, rect_height, color_white, false);
        // Print M17 message being typed
        gfx_printLine(1, 1, layout.top_h,
                      CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.message_font,
                      TEXT_ALIGN_CENTER, color_white, ui_state->new_message);
    } else {
        _ui_drawMenuListValue(ui_state, ui_state->menu_selected,
                              _ui_getM17EntryName, _ui_getM17ValueName);
    }
}
#endif

void _ui_drawSettingsFM(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "FM Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->fm);
    // Print FM settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected, _ui_getFMEntryName,
                          _ui_getFMValueName);
}

void _ui_drawSettingsAccessibility(ui_state_t *ui_state)
{
    gfx_clearScreen();
    // Print "Accessibility" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->accessibility);
    // Print accessibility settings entries
    _ui_drawMenuListValue(ui_state, ui_state->menu_selected,
                          _ui_getAccessibilityEntryName,
                          _ui_getAccessibilityValueName);
}

void _ui_drawSettingsReset2Defaults(ui_state_t *ui_state)
{
    (void)ui_state;

    static int drawcnt = 0;
    static long long lastDraw = 0;

    gfx_clearScreen();
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->resetToDefaults);

    // Make text flash yellow once every 1s
    color_t textcolor = drawcnt % 2 == 0 ? color_white : yellow_fab413;
    gfx_printLine(1, 4, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.top_font, TEXT_ALIGN_CENTER,
                  textcolor, currentLanguage->toReset);
    gfx_printLine(2, 4, layout.top_h, CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                  layout.horizontal_pad, layout.top_font, TEXT_ALIGN_CENTER,
                  textcolor, currentLanguage->pressEnterTwice);

    if ((getTick() - lastDraw) > 1000) {
        drawcnt++;
        lastDraw = getTick();
    }

    drawcnt++;
}

void _ui_drawSettingsRadio(ui_state_t *ui_state)
{
    gfx_clearScreen();

    // Print "Radio Settings" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->radioSettings);

    // Handle the special case where a frequency is being input
    if ((ui_state->menu_selected == R_OFFSET) && (ui_state->edit_mode)) {
        char buf[17] = { 0 };
        uint16_t rect_width = CONFIG_SCREEN_WIDTH - (layout.horizontal_pad * 2);
        uint16_t rect_height =
            (CONFIG_SCREEN_HEIGHT - (layout.top_h + layout.bottom_h)) / 2;
        point_t rect_origin = { (CONFIG_SCREEN_WIDTH - rect_width) / 2,
                                (CONFIG_SCREEN_HEIGHT - rect_height) / 2 };

        gfx_drawRect(rect_origin, rect_width, rect_height, color_white, false);

        // Print frequency with the most sensible unit
        char prefix = ' ';
        uint32_t div = 1;
        if (ui_state->new_offset >= 1000000) {
            prefix = 'M';
            div = 1000000;
        } else if (ui_state->new_offset >= 1000) {
            prefix = 'k';
            div = 1000;
        }

        // NOTE: casts are there only to squelch -Wformat warnings on the
        // sniprintf.
        sniprintf(buf, sizeof(buf), "%u.%u",
                  (unsigned int)(ui_state->new_offset / div),
                  (unsigned int)(ui_state->new_offset % div));
        stripTrailingZeroes(buf);

        gfx_printLine(1, 1, layout.top_h,
                      CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.input_font,
                      TEXT_ALIGN_CENTER, color_white, "%s%cHz", buf, prefix);
    } else {
        // Print radio settings entries
        _ui_drawMenuListValue(ui_state, ui_state->menu_selected,
                              _ui_getRadioEntryName, _ui_getRadioValueName);
    }
}
