/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ST7735S display driver for the Ailunce HD2 (160x128 RGB565), driven
 * through the HR_C7000 hardware i8080 controller @0x12000000 (manual 5.3):
 * a write to INDEX (+0x00) emits one command cycle, a write to DATA (+0x04)
 * one 8-bit data cycle; the controller generates CS/RS/WR timing per WCFG.
 *
 * The controller owns PTC3-6 (CS/RS/WR/RD) and PTC7-14 (DB0-7) while the
 * PTC pad mux (DIPLEX2) is in i8080 mode; the panel RESET (PTC2) stays a
 * plain GPIO.  This driver establishes that mux at init and leaves it in
 * i8080 mode as the resting state (the keyboard scan borrows the pins and
 * restores i8080 mode when done).
 *
 * OpenRTX's graphics layer owns the single 40 KiB RGB565 framebuffer and
 * pushes it here via display_renderRows(); this driver keeps no copy.
 */

#include "interfaces/display.h"
#include "interfaces/delays.h"
#include "hwconfig.h"
#include "registers.h"
#include "pinmap.h"
#include "peripherals/gpio.h"
#include <stdint.h>
#include <stddef.h>
#include "drivers/backlight/backlight.h"

/* i8080 write-strobe timing (moved out of the SoC registers.h -- it belongs to
 * the ST7735S driver, not the HR_C7000 register map).  The panel RESET pin
 * (PTC2, GPIO, active-low) is driven via LCD_RST from pinmap.h. */
#define LCD_WCFG_DEFAULT 0x020202u /* 2/2/2 strobes ~143 ns */

/* ST7735S command opcodes used by the init sequence. */
enum {
    ST7735_SLPOUT = 0x11,
    ST7735_DISPON = 0x29,
    ST7735_CASET = 0x2a,
    ST7735_RASET = 0x2b,
    ST7735_RAMWR = 0x2c,
    ST7735_MADCTL = 0x36,
    ST7735_COLMOD = 0x3a,
};

static inline void send_cmd(uint8_t cmd)
{
    LCD_HW->INDEX = cmd;
}
static inline void send_data(uint8_t d)
{
    LCD_HW->DATA = d;
}

static void set_window(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1)
{
    send_cmd(ST7735_CASET);
    send_data(0);
    send_data(x0);
    send_data(0);
    send_data(x1);
    send_cmd(ST7735_RASET);
    send_data(0);
    send_data(y0);
    send_data(0);
    send_data(y1);
}

void display_init(void)
{
    /* Claim the PTC pads for the i8080 controller and make the panel-reset
     * pin (PTC2) a GPIO output.  DIPLEX2 upper bits are write-only -- always
     * write the whole-register constant, never read-modify-write.  GPIOC DDR
     * marks PTC2..14 (reset + LCD bus) as outputs: this is the resting state
     * the keyboard scan saves and restores around borrowing the shared pins. */
    GPIOC->DDR = 0x00007ffcu;
    SOCSYS->IO_DIPLEX2 = HD2_DIPLEX2_LCD_I80;
    gpio_setMode(LCD_RST, OUTPUT);
    LCD_HW->WCFG = LCD_WCFG_DEFAULT; /* 2/2/2 write strobes (~143 ns) */

    /* Reset pulse (active-low) to ST7735S datasheet timing. */
    LCD_HW->INDEX; /* (barrier: ensure mux write posted) */
    gpio_setPin(LCD_RST);
    delayMs(10);
    gpio_clearPin(LCD_RST);
    delayMs(10);
    gpio_setPin(LCD_RST);
    delayMs(120);

    send_cmd(ST7735_SLPOUT);
    delayMs(120);

    /* Frame rate (vendor V2.1.3 values). */
    send_cmd(0xb1);
    send_data(5);
    send_data(0x3c);
    send_data(0x3c);
    send_cmd(0xb2);
    send_data(5);
    send_data(0x3c);
    send_data(0x3c);
    send_cmd(0xb3);
    send_data(5);
    send_data(0x3c);
    send_data(0x3c);
    send_data(5);
    send_data(0x3c);
    send_data(0x3c);
    send_cmd(0xb4);
    send_data(3);

    /* Power control. */
    send_cmd(0xc0);
    send_data(0x28);
    send_data(8);
    send_data(4);
    send_cmd(0xc1);
    send_data(0xc0);
    send_cmd(0xc2);
    send_data(0x0d);
    send_data(0);
    send_cmd(0xc3);
    send_data(0x8d);
    send_data(0x2a);
    send_cmd(0xc4);
    send_data(0x8d);
    send_data(0xee);
    send_cmd(0xc5);
    send_data(0x1a);

    /* Gamma (positive / negative), 16 bytes each. */
    static const uint8_t gp[16] = {
        0x04, 0x22, 0x07, 0x0a, 0x2e, 0x30, 0x25, 0x2a,
        0x28, 0x26, 0x2e, 0x3a, 0x00, 0x01, 0x03, 0x13,
    };
    send_cmd(0xe0);
    for (unsigned i = 0; i < sizeof gp; i++)
        send_data(gp[i]);

    static const uint8_t gn[16] = {
        0x04, 0x16, 0x06, 0x0d, 0x2d, 0x26, 0x23, 0x27,
        0x27, 0x25, 0x2d, 0x3b, 0x00, 0x01, 0x04, 0x13,
    };
    send_cmd(0xe1);
    for (unsigned i = 0; i < sizeof gn; i++)
        send_data(gn[i]);

    send_cmd(ST7735_MADCTL);
    send_data(0xa0); /* landscape, RGB order */
    send_cmd(ST7735_COLMOD);
    send_data(0x05); /* RGB565 */

    /* Blank GRAM to black (full-window RAMWR + zeros). */
    set_window(0, CONFIG_SCREEN_WIDTH - 1, 0, CONFIG_SCREEN_HEIGHT - 1);
    send_cmd(ST7735_RAMWR);
    for (size_t i = 0; i < (size_t)CONFIG_SCREEN_WIDTH * CONFIG_SCREEN_HEIGHT;
         ++i) {
        LCD_HW->DATA = 0;
        LCD_HW->DATA = 0;
    }

    send_cmd(ST7735_DISPON);

    backlight_init(); /* explicit init; the core sets the level (openrtx.c) */
}

void display_terminate(void)
{
    /* Framebuffer is owned by the graphics layer; nothing to free. */
}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    if (endRow > CONFIG_SCREEN_HEIGHT)
        endRow = CONFIG_SCREEN_HEIGHT;
    if (startRow >= endRow)
        return;

    set_window(0, CONFIG_SCREEN_WIDTH - 1, startRow, endRow - 1);
    send_cmd(ST7735_RAMWR);

    const uint16_t *p = (const uint16_t *)fb
                      + (size_t)startRow * CONFIG_SCREEN_WIDTH;
    const size_t n = (size_t)(endRow - startRow) * CONFIG_SCREEN_WIDTH;

    /* Big-endian RGB565, one byte per DATA write. */
    for (size_t i = 0; i < n; ++i) {
        uint16_t v = p[i];
        LCD_HW->DATA = (uint32_t)(v >> 8);
        LCD_HW->DATA = (uint32_t)(v & 0xffu);
    }
}

void display_render(void *fb)
{
    display_renderRows(0, CONFIG_SCREEN_HEIGHT, fb);
}
