/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 keypad driver -- implements OpenRTX keyboard.h (kbd_init /
 * kbd_terminate / kbd_getKeys).
 *
 * The 4x4 matrix shares the LCD data pins on GPIOC (rows PTC7-10, cols
 * PTC11-14).  A scan momentarily swaps the PTC pad mux (DIPLEX2) from the
 * i8080 LCD controller to GPIO and back, and saves/restores GPIOC DR+DDR,
 * so the LCD bus is preserved across calls.  Rows sense through the vendor
 * alternate-function-B selector (gpio_setAltFuncB), set once in kbd_init.
 *
 * Because the matrix and the LCD share pins, a scan must NOT interleave
 * with a display_render(); OpenRTX runs the UI on a single thread that
 * scans then renders, so they never overlap.
 *
 * Matrix cell map, side keys (GPIOB.9 = SK1, GPIOB.7 = EMER) and the SK2
 * "row0 in every column" phantom are live-verified on the V2.1.3 hardware.
 * The rotary-encoder lines (GPIOA.0/1) are a first-cut poll and may need
 * revisiting.
 */

#include "interfaces/keyboard.h"
#include "interfaces/delays.h"
#include "registers.h"
#include "pinmap.h"
#include "peripherals/gpio.h"
#include <stdint.h>

/* SK2 grounds the row0 sense line, so a scan reads row0 low in EVERY
 * column (bit col*4+row0 for all 4 cols).  Unique -> report KEY_F3.
 * (Matrix geometry and pin labels live in pinmap.h.) */
#define KBD_ROW0_ALL_COLS 0x1111u

/* Matrix (row, col) -> key.  Live-verified: the matrix column is the
 * physical key column {1,2,3}/{4,5,6}/{7,8,9}, col3 = nav keys. */
static const keyboard_t keymap[KBD_ROW_COUNT][KBD_COL_COUNT] = {
    { KEY_1, KEY_4, KEY_7, KEY_ENTER },     /* row0 */
    { KEY_2, KEY_5, KEY_8, KEY_UP },        /* row1 */
    { KEY_3, KEY_6, KEY_9, KEY_DOWN },      /* row2 */
    { KEY_STAR, KEY_0, KEY_HASH, KEY_ESC }, /* row3 */
};

static uint8_t rot_prev = 0;

void kbd_init()
{
    /* Route the row pins (GPIOC 7..10) to the alt-func-B keypad sense path;
     * without this they read 0 (they are LCD-bus outputs otherwise). */
    for (uint8_t row = 0; row < KBD_ROW_COUNT; ++row)
        gpio_setAltFuncB(KBD_ROW(row));

    /* Side keys + rotary are inputs. */
    gpio_setMode(SK1_SW, INPUT);
    gpio_setMode(EMER_SW, INPUT);
    gpio_setMode(ROT_A, INPUT);
    gpio_setMode(ROT_B, INPUT);

    rot_prev = (uint8_t)(gpio_readPin(ROT_A) | (gpio_readPin(ROT_B) << 1));
}

void kbd_terminate()
{
    /* DR/DDR are restored per scan; nothing to release. */
}

/* Raw matrix scan: 4 nibbles (one per col), each bit = one row read.
 * bit=1 -> row high (no key); bit=0 -> key pressed in that (row, col). */
static uint16_t scan_matrix(void)
{
    /* Whole-register save/restore of the shared LCD bus: no per-pin API can
     * snapshot the entire port, so these stay direct register accesses. */
    uint32_t saved_dr = GPIOC->DR;
    uint32_t saved_ddr = GPIOC->DDR;

    /* Borrow the shared PTC pads from the i8080 LCD controller. */
    SOCSYS->IO_DIPLEX2 = HD2_DIPLEX2_PTC_GPIO;

    /* rows (7..10) input, cols (11..14) output, cols idle high */
    for (unsigned row = 0; row < KBD_ROW_COUNT; ++row)
        gpio_setMode(KBD_ROW(row), INPUT);
    for (unsigned col = 0; col < KBD_COL_COUNT; ++col) {
        gpio_setMode(KBD_COL(col), OUTPUT);
        gpio_setPin(KBD_COL(col));
    }

    uint16_t out = 0;
    for (unsigned col = 0; col < KBD_COL_COUNT; ++col) {
        gpio_clearPin(KBD_COL(col)); /* drive this col low */
        delayUs(10);                 /* let the driven column + rows settle */
        uint32_t rows = 0;
        for (unsigned row = 0; row < KBD_ROW_COUNT; ++row)
            rows |= (uint32_t)gpio_readPin(KBD_ROW(row)) << row;
        out |= (uint16_t)(rows << (col * 4));
        gpio_setPin(KBD_COL(col)); /* restore high before next */
    }

    GPIOC->DR = saved_dr;
    GPIOC->DDR = saved_ddr;
    SOCSYS->IO_DIPLEX2 = HD2_DIPLEX2_LCD_I80; /* hand pads back to the LCD */
    return out;
}

static keyboard_t rotary_step(void)
{
    uint8_t now = (uint8_t)(gpio_readPin(ROT_A) | (gpio_readPin(ROT_B) << 1));
    uint8_t prev = rot_prev;
    rot_prev = now;

    static const int8_t qdec[16] = {
        0, +1, -1, 0, -1, 0, 0, +1, +1, 0, 0, -1, 0, -1, +1, 0,
    };
    int8_t step = qdec[(prev << 2) | now];
    if (step > 0)
        return KNOB_RIGHT;
    if (step < 0)
        return KNOB_LEFT;
    return 0;
}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;

    uint16_t raw = scan_matrix();
    if (raw != 0 && (raw & KBD_ROW0_ALL_COLS) == 0u) {
        keys |= KEY_F3; /* SK2: row0 low in every column */
    } else if (raw != 0) {
        for (unsigned col = 0; col < KBD_COL_COUNT; ++col) {
            uint8_t rows_high = (raw >> (col * 4)) & 0xfu;
            for (unsigned row = 0; row < KBD_ROW_COUNT; ++row)
                if ((rows_high & (1u << row)) == 0u) /* active-low */
                    keys |= keymap[row][col];
        }
    }

    if (gpio_readPin(SK1_SW) == 0u)
        keys |= KEY_F1;
    if (gpio_readPin(EMER_SW) == 0u)
        keys |= KEY_F2;

    keys |= rotary_step();
    return keys;
}
