/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <hwconfig.h>

#ifdef CONFIG_BROADCAST_FM

#include <stdio.h>
#include "core/graphics.h"
#include "core/bcastfm_tuner.h"
#include "ui/ui_default.h"

/* RSSI shown as a full signal bar, in dBuV */
#define FULL_SCALE_RSSI 60U

static const color_t color_signal = { 0, 200, 80, 255 };

extern layout_t layout;
extern void _ui_drawMainBackground(void);
extern void _ui_drawMainTop(ui_state_t *ui_state);

/**
 * Format the frequency being typed, with the missing digits as '_' and the
 * implied decimal point: "10_._" after typing 1 and 0.
 */
static void formatInput(const bcastfmTuner_t *tuner, char *buf, size_t size)
{
    uint8_t len = bcastfmTuner_inputLength(tuner);
    size_t pos = 0;

    for (uint8_t i = 0; (i < len) && (pos + 2 < size); i++) {
        if (i == len - 1)
            buf[pos++] = '.';
        buf[pos++] = (i < tuner->inputDigits) ? (char)('0' + tuner->input[i]) :
                                                '_';
    }

    buf[pos] = '\0';
}

/*
 * TODO: take the texts of this screen from the string table. They are not
 * there yet because the voice prompt packs would also need new recordings.
 */

/**
 * Draw the reception state line: seek progress, station or no signal.
 */
static void drawStatus(const bcastfmTuner_t *tuner)
{
    const char *text;

    if (!bcastfm_isEnabled())
        text = "Starting...";
    else if (tuner->seekDir > 0)
        text = "Seeking up...";
    else if (tuner->seekDir < 0)
        text = "Seeking down...";
    else if (tuner->status.station)
        text = tuner->status.stereo ? "Station  Stereo" : "Station";
    else
        text = "No signal";

    gfx_print(layout.line1_pos, layout.line1_font, TEXT_ALIGN_CENTER,
              color_white, "%s", text);

    gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
              color_grey, "%u dBuV   SNR %02u", tuner->status.rssi,
              tuner->status.snr);
}

/**
 * Draw the tuned frequency, or the one being typed.
 */
static void drawFrequency(const bcastfmTuner_t *tuner)
{
    char buf[12];
    color_t color = color_white;

    if (tuner->inputDigits != 0) {
        formatInput(tuner, buf, sizeof(buf));
        color = yellow_fab413;
    } else {
        sniprintf(buf, sizeof(buf), "%lu.%lu",
                  (unsigned long)(tuner->freq / 1000000U),
                  (unsigned long)((tuner->freq % 1000000U) / 100000U));
    }

    gfx_print(layout.line3_large_pos, layout.line3_large_font,
              TEXT_ALIGN_CENTER, color, "%s MHz", buf);
}

/**
 * Draw a signal strength bar in place of the S-meter.
 */
static void drawSignalBar(const bcastfmTuner_t *tuner)
{
    uint16_t width = CONFIG_SCREEN_WIDTH - 2 * layout.horizontal_pad;
    uint16_t height = layout.bottom_h / 2;
    point_t pos = { layout.horizontal_pad,
                    CONFIG_SCREEN_HEIGHT - layout.bottom_pad - height };

    uint8_t rssi = tuner->status.rssi;
    if (rssi > FULL_SCALE_RSSI)
        rssi = FULL_SCALE_RSSI;

    uint16_t fill = ((uint32_t)width * rssi) / FULL_SCALE_RSSI;
    color_t color = tuner->status.station ? color_signal : color_grey;

    gfx_drawRect(pos, width, height, color_grey, false);
    if (fill > 0)
        gfx_drawRect(pos, fill, height, color, true);
}

void _ui_drawBroadcastFm(ui_state_t *ui_state)
{
    const bcastfmTuner_t *tuner = &ui_state->bcastfm;

    gfx_clearScreen();
    _ui_drawMainBackground();
    _ui_drawMainTop(ui_state);

    drawStatus(tuner);
    drawFrequency(tuner);
    drawSignalBar(tuner);
}

#endif /* CONFIG_BROADCAST_FM */
