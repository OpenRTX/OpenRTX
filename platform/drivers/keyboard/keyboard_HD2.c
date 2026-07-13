/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 keypad driver -- first bring-up.  Implements OpenRTX's keyboard.h
 * (kbd_init / kbd_terminate / kbd_getKeys).  The matrix scan + side-key
 * + rotary reads are based on the vendor V2.1.3 extraction in
 * src/firmware/keypad/keypad.c plus the live-verified keycode map from
 * memory [[hd2-keypad]].
 *
 * What is known and verified live (V2.1.3 firmware + dbgshell, 2026-05):
 *   - Side-1 / Side-2  -> GPIOB bit 7 / bit 9  (vendor pin IDs 0x27/0x29)
 *   - Rotary push (Menu) is part of the matrix
 *   - PTT is via PWM ch1 ctrl bit-1 drop (handled by platform_getPttStatus,
 *     NOT this driver -- see hd2-keypad memory entry).
 *   - Vendor scancodes:
 *       0..9 -> ASCII 0x30..0x39
 *       *  # -> 0x2a, 0x23
 *       Menu  -> 0x0d   (KEY_ENTER)
 *       Up    -> 0x11   (KEY_UP)
 *       Down  -> 0x13   (KEY_DOWN)
 *       Exit  -> 0x1b   (KEY_ESC)
 *       Side1 -> 0x64   (KEY_F1)
 *       Side2 -> 0x65   (KEY_F2)
 *       Rotary CW  aliased to Up
 *       Rotary CCW aliased to Down
 *
 * Matrix bring-up (2026-05-30): the alt-func/IOMUX register turned out
 * to be a vendor-custom register at GPIO bank base + 0x78 (one selector
 * per bank).  Setting bit N puts pin N into "alt-func B" mode, which
 * routes external pin state to EXT_PORT regardless of DDR.  Live-
 * verified on V2.1.3 dbgshell: GPIOC+0x78 = 0x00000780 (bits 7-10) when
 * the vendor keypad task is scanning the matrix.  See memory
 * [[hd2-keypad-iomux-hunt]] for the discovery story.
 *
 * What is GUESSED and needs hardware verification:
 *   - Row pins: GPIOC bits 7..10 (vendor pin IDs 0x47..0x4a)
 *     The vendor's gpio_set_mode(0x47, 1, 2) writes into bank C (pin id
 *     >= 0x40 path).  Pin ID minus 0x40 = bit number, so 0x47 = bit 7.
 *   - Col pins: GPIOC bits 11..14 (vendor pin IDs 0x4b..0x4e)
 *   - Row<->col<->key layout:  Initial guess below is a standard MD-style
 *     4x4 layout.  The vendor's scan-localize stage is inline code at
 *     0x030529dc..0x03052a7f, not a static table, so we can't crib it
 *     directly.  Use the 'k' loader command (returns raw row reads per
 *     col-strobe) to confirm before committing.
 *   - Rotary direction:  The HD2 vendor reads rotary via GPIOA interrupts
 *     and aliases CW->Up / CCW->Down.  We poll GPIOA EXT_PORT bits here
 *     as a first cut; if it edge-detects unreliably we can revisit.
 *
 * Important interaction: GPIOC bits 2..14 are shared with the LCD bus
 * (see ST7735S_HD2.c).  kbd_getKeys() temporarily reconfigures GPIOC
 * DDR for the matrix scan and restores it before returning, so the LCD
 * bus state is preserved across calls.  This means a kbd_getKeys() in
 * the middle of a display_render() would still corrupt the LCD write
 * in progress -- caller must not interleave.  The vendor V2.1.3 firmware
 * runs the keypad as its own task gated on a 16 ms tick, so frames and
 * scans never overlap; OpenRTX should do similar once we have threads.
 */

#include "interfaces/keyboard.h"
#include "interfaces/delays.h"
#include "hwconfig.h"
#include "hd2_regs.h"   /* SOCSYS_IO_DIPLEX2 + HD2_DIPLEX2_* mux constants */

#include <stdint.h>


/* ---- MMIO ---------------------------------------------------------- *
 *
 * DesignWare-style GPIO bank layout:
 *   +0x00  DR        (output data register)
 *   +0x04  DDR       (direction: 1 = output)
 *   +0x50  EXT_PORT  (input mirror)
 *
 * Bank addresses verified from V2.1.3 mmio snapshot + platform_init:
 */
#define GPIOA_BASE      0x14020000u
#define GPIOB_BASE      0x14100000u
#define GPIOC_BASE      0x14110000u

#define GPIO_DR(base)         (*(volatile uint32_t *)((base) + 0x00u))
#define GPIO_DDR(base)        (*(volatile uint32_t *)((base) + 0x04u))
#define GPIO_EXT_PORT(base)   (*(volatile uint32_t *)((base) + 0x50u))

/* Vendor-custom alt-func B selector at GPIO_BASE + 0x78.  Setting a bit
 * puts the corresponding pin into "alt-func B" mode (keypad-row sense
 * path), which routes external pin state into the centralised sense
 * register at 0x140d0034 rather than into EXT_PORT.  Live-verified from
 * a V2.1.3 dbgshell snapshot 2026-05-30: GPIOC+0x78 reads 0x00000780
 * exactly when the vendor keypad task is scanning the matrix (bits
 * 7,8,9,10 = the four row pins on bank C). */
#define GPIO_ALT_B(base)      (*(volatile uint32_t *)((base) + 0x78u))

/* Keypad row sense register -- the central place where alt-func B pins
 * appear as inputs.  Live-verified 2026-05-30: with rows in alt-func B
 * and cols driven low one at a time, this register reads 0xff (all
 * "high"/no-key) when no key is held; bit N drops to 0 when row N is
 * pulled low by a key contact bridging row<->driven-low col.
 *
 * 0x140d0044 is a mirror (likely raw-vs-latched).  We use 0x34. */
#define KBD_ROW_SENSE         (*(volatile uint32_t *)0x140d0034u)
#define KBD_ROW_SENSE_MASK    0x0fu    /* low nibble = rows 0..3 */


/* ---- pin assignments (TENTATIVE -- see header comment) -------------- */

/* Rows: read-as-input during scan (with internal pullup).  When any col
 * is strobed low and a key in that column is pressed, the corresponding
 * row line is pulled low through the key contact. */
#define KBD_ROW_SHIFT       7u
#define KBD_ROW_MASK        (0xfu << KBD_ROW_SHIFT)       /* bits 7..10 */
#define KBD_ROW_COUNT       4u

/* Cols: driven low one at a time during matrix scan. */
#define KBD_COL_SHIFT       11u
#define KBD_COL_MASK        (0xfu << KBD_COL_SHIFT)       /* bits 11..14 */
#define KBD_COL_COUNT       4u

/* Direct GPIOB keys.  HARDWARE-VERIFIED 2026-06-01 via press_detect.py
 * (each pressed individually):
 *   bit 9  = SK1   (upper side button, vendor pin 0x29)
 *   bit 7  = EMER  (emergency button, vendor pin 0x27)  <-- NOT "Side-2";
 *            the earlier label was wrong, this drops for the EMERGENCY key
 *   bit 11 = PTT   (handled by platform_getPttStatus, listed for reference)
 * NB: the physical "SK2" button changes NO GPIO bit on any bank (A/B/C),
 * even with a column strobe -- it is read via a non-GPIO path (like PTT),
 * still TBD.  See memory [[hd2-keypad]]. */
#define KBD_SIDE1_BIT       (1u << 9)     /* SK1  (upper side button) */
#define KBD_EMER_BIT        (1u << 7)     /* EMER (emergency button)  */

/* SK2 is NOT a matrix cell or a GPIO bit.  HARDWARE-VERIFIED 2026-06-02:
 * it grounds the row0 sense line (GPIOC.7 / vendor pin 0x47) wholesale, so
 * a matrix scan reads row0 low in EVERY column at once -- raw bits
 * 0,4,8,12 all low (raw == 0xeeee at rest).  This is the "0xeeee phantom"
 * seen during early bring-up.  A real row0 key pulls only ONE column low
 * (e.g. '1' -> 0xfffe), so the all-columns pattern uniquely identifies SK2.
 * Reported as KEY_F3 (the third programmable function key). */
#define KBD_ROW0_ALL_COLS   0x1111u       /* bit (col*4 + row0) for all 4 cols */
#define KBD_SK2_KEY         KEY_F3

/* Rotary encoder lines on GPIOA (poll-mode for first cut). */
#define KBD_ROT_A_BIT       (1u << 0)     /* TODO: verify */
#define KBD_ROT_B_BIT       (1u << 1)     /* TODO: verify */


/* ---- keymap: matrix (row, col) -> KEY_* flag.
 *
 * HARDWARE-VERIFIED 2026-06-01 via scripts/press_detect.py: each physical
 * key was pressed one at a time and its raw matrix cell recorded.  The
 * earlier guess had the 3x3 number block TRANSPOSED -- the matrix COLUMN
 * is the physical key ROW (col0 = the {1,2,3} key column, col1 = {4,5,6},
 * col2 = {7,8,9}).  The col3 nav keys and the bottom {*,0,#} row were
 * already correct.
 *
 * Verified cell -> key:
 *   (r0,c0)=1 (r1,c0)=2 (r2,c0)=3 (r3,c0)=*
 *   (r0,c1)=4 (r1,c1)=5 (r2,c1)=6 (r3,c1)=0
 *   (r0,c2)=7 (r1,c2)=8 (r2,c2)=9 (r3,c2)=#
 *   (r0,c3)=MENU (r1,c3)=UP (r2,c3)=DOWN (r3,c3)=EXIT */
static const keyboard_t keymap[KBD_ROW_COUNT][KBD_COL_COUNT] =
{
    /* col0     col1     col2     col3   */
    { KEY_1,    KEY_4,   KEY_7,   KEY_ENTER },   /* row0 */
    { KEY_2,    KEY_5,   KEY_8,   KEY_UP    },   /* row1 */
    { KEY_3,    KEY_6,   KEY_9,   KEY_DOWN  },   /* row2 */
    { KEY_STAR, KEY_0,   KEY_HASH, KEY_ESC  },   /* row3 */
};


/* ---- rotary state -------------------------------------------------- */
static uint8_t rot_prev = 0;


/* ---- short delay used between col strobe + row sample -------------- *
 *
 * Vendor calls FUN_00030ab8(0x14) ~= 20 ticks of an unknown timer.  At
 * 192 MHz that's ~100 ns which is too short for capacitor settle on a
 * matrix.  Use the existing delayUs once it's calibrated; for now a
 * conservative spin works. */
static inline void scan_settle(void)
{
    for (volatile uint32_t i = 0; i < 200u; ++i) { }
}


/* ---- kbd_init / kbd_terminate -------------------------------------- *
 *
 * We deliberately DON'T permanently change GPIO state here.  The matrix
 * scan in kbd_getKeys() saves and restores DDR each call, so the LCD
 * bus on GPIOC bits 2..14 is left alone between scans.  Init's only
 * job is to seed the rotary baseline. */

void kbd_init()
{
    /* Put the row pins (GPIOC bits 7..10) into alt-func B mode by setting
     * the matching bits in the vendor-custom alt-func selector at
     * GPIOC+0x78.  This is the OR-equivalent of `gpio_set_mode(0x47..0x4a,
     * 1, 2)` in the vendor scan loop -- without it, GPIOC bits 7-10 read
     * 0 unconditionally because they're routed to the LCD-bus output
     * latch and DDR=output blocks any input sense.  Memory entry
     * [[hd2-keypad-iomux-hunt]] for the discovery story.
     *
     * Col pins (bits 11..14) stay normal GPIO outputs (driven low one at
     * a time during scan), so we don't touch their alt-func bits. */
    GPIO_ALT_B(GPIOC_BASE) |= KBD_ROW_MASK;

    /* Make sure side keys are inputs on GPIOB.  GPIOB bits 7/9 should
     * already be inputs in our IAP-default state but be defensive. */
    GPIO_DDR(GPIOB_BASE) &= ~(KBD_SIDE1_BIT | KBD_EMER_BIT);

    /* Rotary lines on GPIOA -- inputs, pull-up assumed. */
    GPIO_DDR(GPIOA_BASE) &= ~(KBD_ROT_A_BIT | KBD_ROT_B_BIT);

    /* Capture current rotary state so the first kbd_getKeys() doesn't
     * spuriously emit KNOB_LEFT/RIGHT against an uninitialised baseline. */
    uint32_t a = GPIO_EXT_PORT(GPIOA_BASE);
    rot_prev = (uint8_t)(((a & KBD_ROT_A_BIT) ? 1u : 0u) |
                         ((a & KBD_ROT_B_BIT) ? 2u : 0u));
}

void kbd_terminate()
{
    /* Nothing to release -- we restore DDR per-call. */
}


/* ---- raw matrix scan (used by both kbd_getKeys + the 'k' probe) ---- *
 *
 * Returns 4 packed nibbles (one per col), low 4 bits of each nibble =
 * the four row reads with that col strobed low.  Bit = 1 means "row is
 * high" (no key); bit = 0 means "key pressed" in that (row, col) cell.
 *
 * Layout: bits 0..3  = col0 row reads
 *         bits 4..7  = col1 row reads
 *         bits 8..11 = col2 row reads
 *         bits 12..15= col3 row reads
 */
uint16_t hd2_kbd_scan_raw(void)
{
    /* CORRECTED 2026-05-31: rows are read straight from GPIOC EXT_PORT
     * bits 7..10 (NOT the 0x140d0034 ADC -- that was a dead end).  The
     * vendor keypad_scan_once @ 0x0305290c does exactly this via gpio_read.
     * Rows must be INPUTS and cols OUTPUTS during the scan; both share the
     * LCD data bus (PTC7..14), so save + restore DR *and* DDR.  alt-func-B
     * on the rows is set once in kbd_init.  LIVE-VERIFIED: no-key rows=0xf;
     * '5' = col bit12 x row bit8. */
    uint32_t saved_dr  = GPIO_DR(GPIOC_BASE);
    uint32_t saved_ddr = GPIO_DDR(GPIOC_BASE);

#if !defined(HD2_LCD_BITBANG) || !(HD2_LCD_BITBANG)
    /* HW-i8080 LCD build: the matrix pins (rows PTC7-10, cols PTC11-14) are
     * muxed to the LCD controller; swap them to GPIO for the scan and back
     * after.  DIPLEX2 is write-only in its upper bits -- whole-register
     * constants, never RMW (hd2_regs.h).  Scan and LCD writes are serialized
     * by the same single-caller assumption the bit-bang latch sharing has
     * always relied on. */
    SOCSYS_IO_DIPLEX2 = HD2_DIPLEX2_PTC_GPIO;
#endif

    /* rows (7..10) -> input, cols (11..14) -> output */
    GPIO_DDR(GPIOC_BASE) = (saved_ddr & ~KBD_ROW_MASK) | KBD_COL_MASK;

    uint16_t out = 0;
    for (unsigned col = 0; col < KBD_COL_COUNT; ++col)
    {
        uint32_t col_bit = 1u << (KBD_COL_SHIFT + col);
        /* Drive only this col low, others high. */
        GPIO_DR(GPIOC_BASE) = (saved_dr | KBD_COL_MASK) & ~col_bit;

        scan_settle();

        /* Rows from EXT_PORT bits 7..10.  1 = row high (no key); 0 = key
         * pressed (row pulled low through the contact to the low col). */
        uint32_t rows = (GPIO_EXT_PORT(GPIOC_BASE) >> KBD_ROW_SHIFT) & 0xfu;
        out |= (uint16_t)((rows & 0xfu) << (col * 4));
    }

    /* Restore the LCD bus state (DR + DDR). */
    GPIO_DR(GPIOC_BASE)  = saved_dr;
    GPIO_DDR(GPIOC_BASE) = saved_ddr;

#if !defined(HD2_LCD_BITBANG) || !(HD2_LCD_BITBANG)
    /* Hand the pins back to the i8080 LCD controller. */
    SOCSYS_IO_DIPLEX2 = HD2_DIPLEX2_LCD_I80;
#endif

    return out;
}


/* ---- rotary decoding (quadrature, polled) -------------------------- *
 *
 * Standard quadrature: A leads B = CW (KNOB_RIGHT), B leads A = CCW
 * (KNOB_LEFT).  We sample on each kbd_getKeys call.  The vendor
 * firmware uses GPIOA edge interrupts -- we'll move to that once the
 * IRQ infra exists.  For now poll, which is fine at >= 100 Hz call
 * rates (most rotary turns are < 50 Hz). */
static keyboard_t rotary_step(void)
{
    uint32_t a = GPIO_EXT_PORT(GPIOA_BASE);
    uint8_t now = (uint8_t)(((a & KBD_ROT_A_BIT) ? 1u : 0u) |
                            ((a & KBD_ROT_B_BIT) ? 2u : 0u));
    uint8_t prev = rot_prev;
    rot_prev = now;

    /* Quadrature transition table.  Index = (prev << 2) | now.
     *   +1 -> CW, -1 -> CCW, 0 -> none. */
    static const int8_t qdec[16] = {
         0, +1, -1,  0,
        -1,  0,  0, +1,
        +1,  0,  0, -1,
         0, -1, +1,  0,
    };
    int8_t step = qdec[(prev << 2) | now];
    if (step > 0) return KNOB_RIGHT;
    if (step < 0) return KNOB_LEFT;
    return 0;
}


keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;

    /* Matrix scan.  Only honour the result if at least one row read as
     * "high" -- if the alt-func B sense register at 0x140d0034 reads 0
     * for every col strobe, the chip-level keypad controller (0x140d0000
     * peripheral) isn't fully waking up in our build, and the raw matrix
     * scan would (incorrectly) report all-keys-pressed.  Skip the matrix
     * keymap entirely in that case so the UI doesn't see phantom presses.
     * See memory [[hd2-keypad-iomux-hunt]] for what's still missing. */
    uint16_t raw = hd2_kbd_scan_raw();
    if (raw != 0 && (raw & KBD_ROW0_ALL_COLS) == 0u)
    {
        /* SK2: grounds the row0 sense line, so row0 reads low in ALL
         * columns.  Emit SK2 and skip the matrix decode so we don't also
         * report the four phantom row0 keys (1/4/7/MENU). */
        keys |= KBD_SK2_KEY;
    }
    else if (raw != 0)
    {
        for (unsigned col = 0; col < KBD_COL_COUNT; ++col)
        {
            uint8_t rows_high = (raw >> (col * 4)) & 0xfu;
            for (unsigned row = 0; row < KBD_ROW_COUNT; ++row)
            {
                /* Active-low: 0 bit = key pressed. */
                if ((rows_high & (1u << row)) == 0u)
                {
                    keys |= keymap[row][col];
                }
            }
        }
    }

    /* Side keys (GPIOB).  Active-low. */
    uint32_t b = GPIO_EXT_PORT(GPIOB_BASE);
    if ((b & KBD_SIDE1_BIT) == 0u) keys |= KEY_F1;   /* SK1  */
    if ((b & KBD_EMER_BIT)  == 0u) keys |= KEY_F2;   /* EMER */

    /* Rotary encoder. */
    keys |= rotary_step();

    return keys;
}
