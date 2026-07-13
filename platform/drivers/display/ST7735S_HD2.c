/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Active HD2 LCD bring-up.  display_init() runs the vendor init
 * sequence but as of 2026-05-29 the LCD chip isn't accepting commands
 * -- backlight lights (white screen) but no actual pixel data shows.
 * Likely missing IOMUX / pin-routing step still being investigated.
 * See memory project-hd2-v213-mmio-snapshot.  Other agents: do not
 * refactor or extend this code as if it's a settled implementation.
 *
 * ST7735S driver for the Ailunce HD2 (160x128 RGB565).
 *
 * Cousin of ST7735R_CS7000.c (same panel family, different host wiring):
 *
 *   - CS7000 drives the panel through real GPIO pins + an STM32 8-bit
 *     parallel bus.
 *   - HD2 drives it through a single 32-bit MMIO latch at 0x14110000.
 *     One write to that register sets data/cmd, write strobe, drive
 *     enable, reset, and the 8 data bits all at once.
 *
 * Latch layout (see src/firmware/include/lcd.h):
 *   bit  2 (0x0004)  LCD_RST    active-low reset
 *   bit  3 (0x0008)  LCD_WR     write strobe (idle high)
 *   bit  4 (0x0010)  LCD_DCX    0 = command, 1 = data
 *   bit  5 (0x0020)  LCD_CSB    drive enable (asserted during write)
 *   bit  6 (0x0040)  LCD_CLK2   secondary clock-domain bit
 *   bits 7..14 (0xFF80)         D0..D7
 *
 * Init sequence is the HD2 vendor V2.1.3 sequence verbatim (see
 * src/firmware/lcd/lcd.c lcd_init).  ST7735S timings + gamma values
 * differ from the CS7000 ST7735R.
 *
 * Framebuffer:
 *   static 40 KiB block in .bss (CONFIG_SCREEN_WIDTH * CONFIG_SCREEN_HEIGHT *
 *   sizeof(uint16_t)).  No malloc -- avoids needing newlib heap during
 *   first bring-up.  display.h says display_init should "allocate
 *   framebuffer on the heap" but the real contract is "return a stable
 *   pointer", which the static array satisfies.
 *
 * Backlight:
 *   bring-up here uses PWM channel 0 on the HR_C7000 PWM block at
 *   0x140c0000.  Vendor sequence comes from pwm_channel_start @ 0x03059d20
 *   (see src/firmware/audio/audio.c).  Period = 0x1068 (~4200 ticks @
 *   the IAP-default timer source), duty = 50%.  TODO: factor out into
 *   platform/drivers/backlight/backlight_HD2.c once we have brightness
 *   level support.
 */

#include "interfaces/display.h"
#include "interfaces/delays.h"
#include "drivers/backlight/backlight.h"
extern void delayMs(unsigned int mseconds);
#include "hwconfig.h"
#include "hd2_regs.h"
#include <stdint.h>
#include <string.h>

/* ---- MMIO ---------------------------------------------------------- *
 * The LCD bus is a single 32-bit latch on GPIOC's data register
 * (0x14110000); PWM_CH0_BASE (backlight) comes from hd2_regs.h. */
#define LCD_LATCH      GPIOC_DR

/* ---- latch bits ---------------------------------------------------- *
 * CORRECTED 2026-05-30 from the ST7735S datasheet (8080 write cycle) + the
 * HR_C7000 manual pin map (PTC2-6) + live verification.  The previous map
 * had WR and CS SWAPPED (WR=bit3, CSB=bit5), so it strobed CS and held WR
 * high -> the panel never latched -> permanent white.  Correct map:
 *   bit2 RST  (active-low reset)
 *   bit3 CS   (active-low chip-select; held LOW for the whole transaction)
 *   bit4 DCX  (0 = command, 1 = data)
 *   bit5 WR   (write strobe; the LCD latches on the WR RISING edge)
 *   bit6 RD   (read strobe; held HIGH/inactive for writes)
 *   bits7..14 D0..D7
 */
#define LCD_RST        0x004u   /* bit2 */
#define LCD_CS         0x008u   /* bit3, active low */
#define LCD_DCX        0x010u   /* bit4 */
#define LCD_WR         0x020u   /* bit5, strobe (idle high) */
#define LCD_RD         0x040u   /* bit6, held high for writes */
#define LCD_DATA_SHIFT 7
#define LCD_DATA_MASK  0xff80u

/* ---- ST7735S commands ---------------------------------------------- */
enum
{
    ST7735_SLPOUT  = 0x11,
    ST7735_INVOFF  = 0x20,
    ST7735_DISPON  = 0x29,
    ST7735_CASET   = 0x2a,
    ST7735_RASET   = 0x2b,
    ST7735_RAMWR   = 0x2c,
    ST7735_MADCTL  = 0x36,
    ST7735_COLMOD  = 0x3a,
};

/* ---- framebuffer ----------------------------------------------------
 * This driver owns NO framebuffer: the OpenRTX graphics layer keeps the
 * single 40 KiB .bss.fb in graphics.c and pushes it here via
 * display_renderRows().  The boot clear below blanks the panel by streaming
 * zeros directly to GRAM, so no driver-side copy is needed (reclaims 40 KiB
 * of SRAM — see hd2_dmr / AMBE-codec RAM budget). */

/* ---- low-level bus primitives -------------------------------------- */

#if defined(HD2_LCD_BITBANG) && (HD2_LCD_BITBANG)
/*
 * GPIO-latch bit-bang (fallback transport).
 *
 * ST7735S 8080 write cycle: CS low, present byte on D0..D7 with DCX set
 * for cmd/data and WR low, then drive WR high -- the panel latches on the
 * WR rising edge (RD held high throughout).  bus_write_byte() performs the
 * full WR low->high strobe; the caller sets CS/DCX/RD first.
 */
static inline void bus_write_byte(uint8_t b)
{
    /* Preserve CS/DCX/RD; clear WR + data, present byte (WR low). */
    uint32_t v = (LCD_LATCH & ~(LCD_WR | LCD_DATA_MASK))
               | ((uint32_t)b << LCD_DATA_SHIFT);
    LCD_LATCH = v;              /* WR low, data driven */
    LCD_LATCH = v | LCD_WR;     /* WR rising edge -> latch */
}

static inline void send_cmd(uint8_t cmd)
{
    /* CS low (assert), DCX=0 (command), RD high. */
    LCD_LATCH = (LCD_LATCH & ~(LCD_CS | LCD_DCX)) | LCD_RD;
    bus_write_byte(cmd);
}

static inline void send_data(uint8_t d)
{
    /* CS low (assert), DCX=1 (data), RD high. */
    LCD_LATCH = (LCD_LATCH & ~LCD_CS) | LCD_DCX | LCD_RD;
    bus_write_byte(d);
}

static inline void bus_init(void) { }

#else
/*
 * HW i8080 controller @ 0x1200_0000 (manual 5.3; default transport,
 * LIVE-VERIFIED 2026-06-13: host-paced INDEX/DATA writes drew pixels on the
 * panel with SCFG/AC_MODE at reset defaults).  A write to INDEX emits one
 * command cycle (RS=0), a write to DATA one data cycle (RS=1); the
 * controller generates CS/WR timing itself per WCFG.  One register write =
 * one 8-bit bus cycle -- ~6 AHB cycles/byte at the 2/2/2 strobe timing vs
 * hundreds for the GPIO latch.  The pad mux (DIPLEX2 bits 3..18 = func 0)
 * is set by platform_init and swapped transiently around keypad scans
 * (keyboard_HD2.c); the panel RESET pin (PTC2) is GPIO-only and stays on
 * the latch path below.
 */
#define I80_INDEX  (*(volatile uint32_t *)0x12000000u)
#define I80_DATA   (*(volatile uint32_t *)0x12000004u)
#define I80_WCFG   (*(volatile uint32_t *)0x12000010u)

static inline void send_cmd(uint8_t cmd) { I80_INDEX = cmd; }
static inline void send_data(uint8_t d)  { I80_DATA  = d;   }

static inline void bus_init(void)
{
    /* Conservative write strobes: 2 leading-inactive / 2 active / 2 trailing
     * cycles (the live proof's timing).  ST7735S min write cycle is 66 ns;
     * this gives ~143 ns at 42 MHz with margin to spare. */
    I80_WCFG = 0x00020202u;
}
#endif /* HD2_LCD_BITBANG */

static void set_window(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1)
{
    send_cmd(ST7735_CASET);
    send_data(0); send_data(x0);
    send_data(0); send_data(x1);
    send_cmd(ST7735_RASET);
    send_data(0); send_data(y0);
    send_data(0); send_data(y1);
}

/* ---- display.h interface ------------------------------------------- */

void display_init(void)
{
    bus_init();                 /* HW i8080: program write-strobe timing */

    /* Reset pulse to ST7735S datasheet (not vendor's tight timings, which
     * worked on their specific panel + boot state but ours apparently
     * needs spec-compliant timing).  10 ms RST low, 120 ms post-reset
     * before any command, 120 ms post-SLPOUT before the next command.
     * RST is PTC2 = GPIO-only (never muxed to the i8080), so the latch
     * path drives it in both transports. */
    LCD_LATCH |=  LCD_RST;
    delayMs(10);
    LCD_LATCH &= ~LCD_RST;
    delayMs(10);
    LCD_LATCH |=  LCD_RST;
    delayMs(120);

    send_cmd(ST7735_SLPOUT);
    delayMs(120);

    /* Frame rate (B1, B2, B3) -- vendor V2.1.3 values */
    send_cmd(0xb1); send_data(5); send_data(0x3c); send_data(0x3c);
    send_cmd(0xb2); send_data(5); send_data(0x3c); send_data(0x3c);
    send_cmd(0xb3); send_data(5); send_data(0x3c); send_data(0x3c);
                    send_data(5); send_data(0x3c); send_data(0x3c);
    send_cmd(0xb4); send_data(3);

    /* Power control */
    send_cmd(0xc0); send_data(0x28); send_data(8); send_data(4);
    send_cmd(0xc1); send_data(0xc0);
    send_cmd(0xc2); send_data(0x0d); send_data(0);
    send_cmd(0xc3); send_data(0x8d); send_data(0x2a);
    send_cmd(0xc4); send_data(0x8d); send_data(0xee);
    send_cmd(0xc5); send_data(0x1a);

    /* Positive gamma (E0) -- 16 bytes */
    static const uint8_t gp[16] = {
        0x04, 0x22, 0x07, 0x0a, 0x2e, 0x30, 0x25, 0x2a,
        0x28, 0x26, 0x2e, 0x3a, 0x00, 0x01, 0x03, 0x13,
    };
    send_cmd(0xe0);
    for (unsigned i = 0; i < sizeof gp; i++) send_data(gp[i]);

    /* Negative gamma (E1) -- 16 bytes */
    static const uint8_t gn[16] = {
        0x04, 0x16, 0x06, 0x0d, 0x2d, 0x26, 0x23, 0x27,
        0x27, 0x25, 0x2d, 0x3b, 0x00, 0x01, 0x04, 0x13,
    };
    send_cmd(0xe1);
    for (unsigned i = 0; i < sizeof gn; i++) send_data(gn[i]);

    send_cmd(ST7735_MADCTL); send_data(0xa0);   /* landscape, RGB order */
    send_cmd(ST7735_COLMOD); send_data(0x05);   /* RGB565 */

    set_window(0, 0x9f, 0, 0x7f);

    /* Blank GRAM to black without a framebuffer: full-window RAMWR + zeros. */
    set_window(0, CONFIG_SCREEN_WIDTH - 1, 0, CONFIG_SCREEN_HEIGHT - 1);
    send_cmd(ST7735_RAMWR);
    for (size_t i = 0; i < (size_t)CONFIG_SCREEN_WIDTH * CONFIG_SCREEN_HEIGHT; ++i)
    {
#if defined(HD2_LCD_BITBANG) && (HD2_LCD_BITBANG)
        LCD_LATCH = (LCD_LATCH & ~LCD_CS) | LCD_DCX | LCD_RD;
        bus_write_byte(0); bus_write_byte(0);
#else
        I80_DATA = 0; I80_DATA = 0;
#endif
    }

    send_cmd(ST7735_DISPON);

    /* Wire the real PWM backlight driver. Without backlight_init() (the
     * threaded build links main.cpp, not the loader's main.c which used to
     * call it), bl_initialised stays 0 and display_setBacklightLevel() — used
     * by Settings brightness AND by UI standby (level 0) — is a silent no-op.
     * Drive the initial level through the driver instead of poking PWM here. */
    backlight_init();
    display_setBacklightLevel(50);
}

void display_terminate(void)
{
    /* nothing to free -- framebuffer is static */
}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    if (endRow > CONFIG_SCREEN_HEIGHT) endRow = CONFIG_SCREEN_HEIGHT;
    if (startRow >= endRow) return;

    set_window(0, CONFIG_SCREEN_WIDTH - 1, startRow, endRow - 1);
    send_cmd(ST7735_RAMWR);

    const uint16_t *p = (const uint16_t *)fb + (size_t)startRow * CONFIG_SCREEN_WIDTH;
    const size_t   n = (size_t)(endRow - startRow) * CONFIG_SCREEN_WIDTH;

#if defined(HD2_LCD_BITBANG) && (HD2_LCD_BITBANG)
    /* Bulk pixel write: assert CS low, DCX=data, RD high ONCE; then
     * bus_write_byte() strobes WR per byte (CS/DCX/RD stay put). */
    LCD_LATCH = (LCD_LATCH & ~LCD_CS) | LCD_DCX | LCD_RD;

    for (size_t i = 0; i < n; ++i) {
        uint16_t v = p[i];
        bus_write_byte((uint8_t)(v >> 8));
        bus_write_byte((uint8_t)v);
    }
#else
    /* HW i8080: each DATA write is one byte cycle; the controller handles
     * CS/RS/WR.  Big-endian RGB565 like the panel expects. */
    for (size_t i = 0; i < n; ++i) {
        uint16_t v = p[i];
        I80_DATA = (uint32_t)(v >> 8);
        I80_DATA = (uint32_t)(v & 0xffu);
    }
#endif
}

void display_render(void *fb)
{
    display_renderRows(0, CONFIG_SCREEN_HEIGHT, fb);
}

void display_setContrast(uint8_t contrast)
{
    (void)contrast;                              /* ST7735S has no contrast reg */
}

/* display_setBacklightLevel lives in platform/drivers/backlight/backlight_HD2.c
 * now (matches the OpenRTX pattern: backlight driver owns the PWM, display
 * driver only owns the bus). */
